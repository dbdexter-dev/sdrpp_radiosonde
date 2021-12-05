#include <iostream>
#include <imgui.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include <sstream>
#include "main.hpp"
#include "decode/rs41/rs41.h"

#define SYMRATE 4800.0
#define DEFAULT_BANDWIDTH 10000
#define SNAP_INTERVAL 5000

SDRPP_MOD_INFO {
    /* Name:            */ "radiosonde_decoder",
    /* Description:     */ "Radiosonde decoder for SDR++",
    /* Author:          */ "dbdexter-dev",
    /* Version:         */ 0, 0, 1,
    /* Max instances    */ -1
};

RadiosondeDecoderModule::RadiosondeDecoderModule(std::string name)
{
	this->name = name;
	bw = DEFAULT_BANDWIDTH;
	symrate = SYMRATE/bw;

	vfo = sigpath::vfoManager.createVFO(name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	vfo->setSnapInterval(SNAP_INTERVAL);
	fmDemod.init(vfo->output, bw, bw/2.0f);
	resampler.init(&fmDemod.out, symrate, 0.707, symrate/250, symrate/1e4);
	slicer.init(&resampler.out);
	framer.init(&slicer.out, RS41_SYNCWORD, RS41_SYNC_LEN, RS41_FRAME_LEN);
	rs41Decoder.init(&framer.out, sondeDataHandler, this);

	lastData.lat = lastData.lon = lastData.alt = 0;

	fmDemod.start();
	resampler.start();
	slicer.start();
	framer.start();
	rs41Decoder.start();
	enabled = true;

	gui::menu.registerEntry(name, menuHandler, this, this);
}

RadiosondeDecoderModule::~RadiosondeDecoderModule()
{
	if (isEnabled()) disable();
	sigpath::vfoManager.deleteVFO(vfo);
	gui::menu.removeEntry(name);
}

void
RadiosondeDecoderModule::enable() {
	vfo = sigpath::vfoManager.createVFO(name, ImGui::WaterfallVFO::REF_CENTER, 0, bw, bw, bw, bw, true);
	vfo->setSnapInterval(SNAP_INTERVAL);
	fmDemod.setInput(vfo->output);
	resampler.setInput(&fmDemod.out);
	slicer.setInput(&resampler.out);
	framer.setInput(&slicer.out);
	rs41Decoder.setInput(&framer.out);

	fmDemod.start();
	resampler.start();
	slicer.start();
	framer.start();
	rs41Decoder.start();
	enabled = true;
}

void
RadiosondeDecoderModule::disable() {
	rs41Decoder.stop();
	framer.stop();
	slicer.stop();
	resampler.stop();
	fmDemod.stop();
	sigpath::vfoManager.deleteVFO(vfo);
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

	float width = ImGui::GetContentRegionAvailWidth();

	if (!_this->enabled) style::beginDisabled();


	ImGui::SetNextItemWidth(width);
	if (ImGui::BeginTable("split", 2)) {
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
			ImGui::Text("%.0f'", _this->lastData.hdg);
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
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
			ImGui::Text("%.1f'C", _this->lastData.temp);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Relative Humidity");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
			ImGui::Text("%.1f%%", _this->lastData.rh);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Dew point");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
			ImGui::Text("%.1f'C", _this->lastData.dewpt);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Pressure");
		if (_this->enabled) {
			ImGui::TableNextColumn();
			if (!_this->lastData.calibrated) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
			ImGui::Text("%.1fhPa", _this->lastData.pressure);
			if (!_this->lastData.calibrated) ImGui::PopStyleColor();
		}

		ImGui::EndTable();
	}

	if (!_this->enabled) style::endDisabled();
}

void
RadiosondeDecoderModule::sondeDataHandler(SondeData *data, void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;
	_this->lastData = *data;
}
/* }}} */


/* Module exports {{{ */
MOD_EXPORT void _INIT_() {
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
	return new RadiosondeDecoderModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void *instance) {
	delete (RadiosondeDecoderModule*)instance;
}

MOD_EXPORT void _END_() {
}

/* }}} */
