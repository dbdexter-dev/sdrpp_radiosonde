#include <config.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <imgui.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include <time.h>
#include <options.h>
#include "main.hpp"
#include "utils.hpp"

#define CONCAT(a, b)    ((std::string(a) + b).c_str())

#define SNAP_INTERVAL 5000
#define GARDNER_DAMP 0.707
#define UNCAL_COLOR IM_COL32(255,234,0,255)
#define OUT_SAMPLE_RATE 48000

SDRPP_MOD_INFO {
    /* Name:            */ "radiosonde_decoder",
    /* Description:     */ "Radiosonde decoder for SDR++",
    /* Author:          */ "dbdexter-dev",
    /* Version:         */ 0, 6, 0,
    /* Max instances    */ -1
};

ConfigManager config;

RadiosondeDecoderModule::RadiosondeDecoderModule(std::string name)
{
	float bw;
	bool created = false;
	int typeToSelect;
	std::string gpxPath, ptuPath;

	this->name = name;
	selectedType = -1;
	activeDecoder = NULL;

	config.acquire();
	if (!config.conf.contains(name)) {
		config.conf[name]["gpxPath"] = getTempFile("radiosonde.gpx");
		config.conf[name]["ptuPath"] = getTempFile("radiosonde_ptu.csv");
		config.conf[name]["sondeType"] = 0;
		created = true;
	}
	gpxPath = config.conf[name]["gpxPath"];
	ptuPath = config.conf[name]["ptuPath"];
	typeToSelect = config.conf[name]["sondeType"];
	config.release(created);

	strncpy(gpxFilename, gpxPath.c_str(), sizeof(gpxFilename)-1);
	strncpy(ptuFilename, ptuPath.c_str(), sizeof(ptuFilename)-1);

	bw = std::get<1>(supportedTypes[typeToSelect]);
	vfo = sigpath::vfoManager.createVFO(name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	vfo->setSnapInterval(SNAP_INTERVAL);
	fmDemod.init(vfo->output, bw, bw/2.0f);

	/* Resampler to 48kHz */
	window.init(bw, 1000, OUT_SAMPLE_RATE);
	resampler.init(&fmDemod.out, &window, bw, OUT_SAMPLE_RATE);
	window.setSampleRate(bw * resampler.getInterpolation());
	resampler.updateWindow(&window);

	rs41decoder.init(&resampler.out, OUT_SAMPLE_RATE, sondeDataHandler, this);
	dfm09decoder.init(&resampler.out, OUT_SAMPLE_RATE, sondeDataHandler, this);
	ims100decoder.init(&resampler.out, OUT_SAMPLE_RATE, sondeDataHandler, this);
	m10decoder.init(&resampler.out, OUT_SAMPLE_RATE, sondeDataHandler, this);

	fmDemod.start();
	resampler.start();
	onTypeSelected(this, typeToSelect);
	enabled = true;

	gui::menu.registerEntry(name, menuHandler, this, this);
}

RadiosondeDecoderModule::~RadiosondeDecoderModule()
{
	if (isEnabled()) disable();
	if (vfo) {
		sigpath::vfoManager.deleteVFO(vfo);
		vfo = NULL;
	}
	gui::menu.removeEntry(name);
}

void
RadiosondeDecoderModule::enable() {
	/* Make a new VFO, wire it into the DSP path, then start the appropriate decoder */
	onTypeSelected(this, selectedType);

	fmDemod.start();
	resampler.start();
	enabled = true;
}

void
RadiosondeDecoderModule::disable() {
	if (activeDecoder) activeDecoder->stop();
	activeDecoder = NULL;

	fmDemod.stop();
	resampler.stop();

	if (vfo) sigpath::vfoManager.deleteVFO(vfo);
	vfo = NULL;

	gpxWriter.stopTrack();
	lastData.init();
	enabled = false;
}

bool
RadiosondeDecoderModule::isEnabled() {
	return enabled;
}

void
RadiosondeDecoderModule::postInit() {
}

/* Private methods {{{*/
void
RadiosondeDecoderModule::menuHandler(void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	const float width = ImGui::GetContentRegionAvailWidth();
	char time[64];
	bool gpxStatusChanged, ptuStatusChanged;

	if (!_this->enabled) style::beginDisabled();

	/* Type combobox {{{ */
	ImGui::LeftLabel("Type");
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	if (ImGui::BeginCombo(CONCAT("##_radiosonde_type_", _this->name), std::get<0>(_this->supportedTypes[_this->selectedType]))) {
		for (int i=0; i<IM_ARRAYSIZE(_this->supportedTypes); i++) {
			const char *curItem = std::get<0>(_this->supportedTypes[i]);
			bool selected = _this->selectedType == i;

			if (ImGui::Selectable(curItem, selected)) {
				onTypeSelected(ctx, i);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	/* }}} */
	/* Sonde data display {{{ */
	ImGui::SetNextItemWidth(width);
	if (ImGui::BeginTable(CONCAT("##radiosonde_data_", _this->name), 2, ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableNextColumn();
		ImGui::Text("Serial no.");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text(_this->lastData.serial.c_str());
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Frame no.");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%d", _this->lastData.seq);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Onboard time");
		if (_this->enabled) {
			if (strftime(time, sizeof(time), "%a %b %d %Y %H:%M:%S", gmtime(&_this->lastData.time))) {
				ImGui::TableNextColumn();
				ImGui::Text("%s", time);
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text(" ");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Latitude");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%8.5f%c", fabs(_this->lastData.lat), (_this->lastData.lat >= 0 ? 'N' : 'S'));
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Longitude");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%8.5f%c", fabs(_this->lastData.lon), (_this->lastData.lon >= 0 ? 'E' : 'W'));
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Altitude");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%.1fm", _this->lastData.alt);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Speed");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%.1fm/s", _this->lastData.spd);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Heading");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%.0f°", _this->lastData.hdg);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Climb");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%.1fm/s", _this->lastData.climb);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text(" ");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Temperature");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1f°C", _this->lastData.temp);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
			if (!_this->lastData.calibrated && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Calibration data not yet available.");
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Humidity");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1f%%", _this->lastData.rh);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
			if (!_this->lastData.calibrated && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Calibration data not yet available.");
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Dew point");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1f°C", _this->lastData.dewpt);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
			if (!_this->lastData.calibrated && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Calibration data not yet available.");
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Pressure");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1fhPa", _this->lastData.pressure);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
			if (!_this->lastData.calibrated && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Calibration data not yet available.");
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Aux. data");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			ImGui::Text("%s", _this->lastData.auxData.c_str());
		}

		ImGui::EndTable();
	}
	/* }}} */
	/* GPX output file {{{ */
	gpxStatusChanged = ImGui::Checkbox(CONCAT("GPX track##_gpx_track_", _this->name), &_this->gpxOutput);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	gpxStatusChanged |= ImGui::InputText(CONCAT("##_gpx_fname_", _this->name), _this->gpxFilename, sizeof(gpxFilename)-1,
	                                     ImGuiInputTextFlags_EnterReturnsTrue);
	if (gpxStatusChanged) onGPXOutputChanged(ctx);
	/* }}} */
	/* Log output file {{{ */
	ptuStatusChanged = ImGui::Checkbox(CONCAT("Log data##_ptu_log_", _this->name), &_this->ptuOutput);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	ptuStatusChanged |= ImGui::InputText(CONCAT("##_ptu_fname_", _this->name), _this->ptuFilename, sizeof(ptuFilename)-1,
	                                     ImGuiInputTextFlags_EnterReturnsTrue);
	if (ptuStatusChanged) onPTUOutputChanged(ctx);
	/* }}} */

	if (!_this->enabled) style::endDisabled();
}

void
RadiosondeDecoderModule::sondeDataHandler(SondeFullData *data, void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	_this->lastData = *data;

	if (data->serial != "") {
		_this->gpxWriter.startTrack(data->serial.c_str());
	}
	_this->gpxWriter.addTrackPoint(data->time, data->lat, data->lon, data->alt, data->spd, data->hdg);
	_this->ptuWriter.addPoint(data);
}

void
RadiosondeDecoderModule::onGPXOutputChanged(void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	if (_this->gpxOutput) {
		_this->gpxOutput = _this->gpxWriter.init(_this->gpxFilename);
	} else {
		_this->gpxWriter.deinit();
	}

	if (_this->gpxOutput) {
		config.acquire();
		config.conf[_this->name]["gpxPath"] = _this->gpxFilename;
		config.release(true);
	}
}

void
RadiosondeDecoderModule::onPTUOutputChanged(void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	if (_this->ptuOutput) {
		_this->ptuOutput = _this->ptuWriter.init(_this->ptuFilename);
	} else {
		_this->ptuWriter.deinit();
	}
	if (_this->ptuOutput) {
		config.acquire();
		config.conf[_this->name]["ptuPath"] = _this->ptuFilename;
		config.release(true);
	}
}

void
RadiosondeDecoderModule::onTypeSelected(void *ctx, int selection)
{
	float bw;
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;

	/* Ensure that the selection is within bounds */
	if (selection > sizeof(_this->supportedTypes)/sizeof(_this->supportedTypes[0])) return;

	/* Spin down the currently active decoder */
	_this->lastData.init();
	if (_this->activeDecoder) _this->activeDecoder->stop();
	_this->activeDecoder = NULL;

	/* If selection is negative, just stop here */
	if (selection < 0) return;
	_this->selectedType = selection;

	/* Save selection to config */
	config.acquire();
	config.conf[_this->name]["sondeType"] = selection;
	config.release(true);


	/* Get new bandwidth */
	bw = std::get<1>(_this->supportedTypes[selection]);

	/* Update VFO */
	_this->fmDemod.stop();
	if (_this->vfo) sigpath::vfoManager.deleteVFO(_this->vfo);
	_this->vfo = sigpath::vfoManager.createVFO(_this->name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	_this->fmDemod.setInput(_this->vfo->output);
	_this->fmDemod.start();

	_this->resampler.setInSampleRate(bw);
	_this->window.setSampleRate(bw * _this->resampler.getInterpolation());
	_this->resampler.updateWindow(&_this->window);

	/* Spin up the appropriate decoder */
	_this->activeDecoder = std::get<2>(_this->supportedTypes[selection]);
	_this->activeDecoder->start();
}
/* }}} */

/* Module exports {{{ */
MOD_EXPORT void _INIT_() {
    json def = json({});
    config.setPath(options::opts.root + "/radiosonde_decoder_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
	return new RadiosondeDecoderModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void *instance) {
	delete (RadiosondeDecoderModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}

/* }}} */
