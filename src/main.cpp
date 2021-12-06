#include <config.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <imgui.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include <time.h>
#include <options.h>
#include "main.hpp"

#define SNAP_INTERVAL 5000
#define GARDNER_DAMP 0.707
#define UNCAL_COLOR IM_COL32(255,255,0,255)

SDRPP_MOD_INFO {
    /* Name:            */ "radiosonde_decoder",
    /* Description:     */ "Radiosonde decoder for SDR++",
    /* Author:          */ "dbdexter-dev",
    /* Version:         */ 0, 3, 0,
    /* Max instances    */ -1
};

ConfigManager config;


RadiosondeDecoderModule::RadiosondeDecoderModule(std::string name)
{
	bool created = false;
	int typeToSelect;
	std::string gpxPath, ptuPath;

	this->name = name;
	selectedType = -1;

	config.acquire();
	if (!config.conf.contains(name)) {
		config.conf[name]["gpxPath"] = "/tmp/radiosonde.gpx";
		config.conf[name]["ptuPath"] = "/tmp/radiosonde_ptu.csv";
		config.conf[name]["sondeType"] = 0;
		created = true;
	}
	gpxPath = config.conf[name]["gpxPath"];
	ptuPath = config.conf[name]["ptuPath"];
	typeToSelect = config.conf[name]["sondeType"];
	config.release(created);

	symRate = std::get<1>(supportedTypes[typeToSelect]);
	bw = std::get<2>(supportedTypes[typeToSelect]);

	strncpy(gpxFilename, gpxPath.c_str(), sizeof(gpxFilename)-1);
	strncpy(ptuFilename, ptuPath.c_str(), sizeof(ptuFilename)-1);

	vfo = sigpath::vfoManager.createVFO(name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	vfo->setSnapInterval(SNAP_INTERVAL);
	fmDemod.init(vfo->output, bw, bw/2.0f);
	resampler.init(&fmDemod.out, symRate, GARDNER_DAMP, symRate/250, symRate/1e4);
	slicer.init(&resampler.out);
	framer.init(&slicer.out, RS41_SYNCWORD, RS41_SYNC_LEN, RS41_FRAME_LEN);

	rs41Decoder.init(&framer.out, sondeDataHandler, this);
	nullDecoder.init(&framer.out, sondeDataHandler, this);

	fmDemod.start();
	resampler.start();
	slicer.start();
	framer.start();
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
	slicer.start();
	framer.start();
	enabled = true;
}

void
RadiosondeDecoderModule::disable() {
	nullDecoder.stop();
	rs41Decoder.stop();

	framer.stop();
	slicer.stop();
	resampler.stop();
	fmDemod.stop();

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
	bool gpxStatusChanged, ptuStatusChanged, gpxStatus, ptuStatus;

	if (!_this->enabled) style::beginDisabled();

	/* Type combobox {{{ */
	ImGui::LeftLabel("Type");
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	if (ImGui::BeginCombo("##_radiosonde_type_", std::get<0>(_this->supportedTypes[_this->selectedType]))) {
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
	if (ImGui::BeginTable("split", 2, ImGuiTableFlags_SizingFixedFit)) {
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
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Humidity");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1f%%", _this->lastData.rh);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Dew point");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1f°C", _this->lastData.dewpt);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Pressure");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, UNCAL_COLOR);
			ImGui::Text("%.1fhPa", _this->lastData.pressure);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::EndTable();
	}
	/* }}} */
	/* GPX output file {{{ */
	gpxStatusChanged = ImGui::Checkbox("GPX track", &_this->gpxOutput);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	gpxStatusChanged |= ImGui::InputText("##_gpx_fname", _this->gpxFilename, sizeof(gpxFilename)-1,
	                                     ImGuiInputTextFlags_EnterReturnsTrue);
	if (gpxStatusChanged) onGPXOutputChanged(ctx);
	/* }}} */
	/* Log output file {{{ */
	ptuStatusChanged = ImGui::Checkbox("Log data", &_this->ptuOutput);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(width - ImGui::GetCursorPosX());
	ptuStatusChanged |= ImGui::InputText("##_ptu_fname", _this->ptuFilename, sizeof(ptuFilename)-1,
	                                     ImGuiInputTextFlags_EnterReturnsTrue);
	if (ptuStatusChanged) onPTUOutputChanged(ctx);
	/* }}} */

	if (!_this->enabled) style::endDisabled();
}

void
RadiosondeDecoderModule::sondeDataHandler(SondeData *data, void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	_this->lastData = *data;

	if (data->serial != "") {
		_this->gpxWriter.startTrack(data->serial.c_str());
	}
	_this->gpxWriter.addTrackPoint(data->time, data->lat, data->lon, data->alt);
	_this->ptuWriter.addPoint(data->time,
	                          data->temp, data->rh, data->dewpt, data->pressure,
	                          data->lat, data->lon, data->alt,
	                          data->spd, data->hdg, data->climb);
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
	float symRate, bw;
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;

	switch (_this->selectedType) {
		case 0: /* RS41 */
			_this->rs41Decoder.stop();
			break;
		default:
			_this->nullDecoder.stop();
			break;
	}

	symRate = std::get<1>(_this->supportedTypes[selection]);
	bw = std::get<2>(_this->supportedTypes[selection]);
	_this->bw = bw;
	_this->symRate = symRate/bw;

	/* Update VFO */
	_this->fmDemod.stop();
	if (_this->vfo) sigpath::vfoManager.deleteVFO(_this->vfo);
	_this->vfo = sigpath::vfoManager.createVFO(_this->name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	_this->fmDemod.setInput(_this->vfo->output);
	_this->fmDemod.start();

	/* Update resampler parameters */
	_this->resampler.setLoopParams(_this->symRate, GARDNER_DAMP, _this->symRate/250, _this->symRate/1e4);


	switch (selection) {
		case 0: /* RS41 */
			_this->rs41Decoder.setInput(&_this->framer.out);
			_this->rs41Decoder.start();
			break;
		default:
			_this->nullDecoder.setInput(&_this->framer.out);
			_this->nullDecoder.start();
			break;
	}

	_this->selectedType = selection;
	if (selection >= 0) {
		config.acquire();
		config.conf[_this->name]["sondeType"] = selection;
		config.release(true);
	}
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
