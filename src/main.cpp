#include <iostream>
#include <imgui.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include "main.hpp"

#define SYMRATE 4800.0
#define DEFAULT_BANDWIDTH 10000

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
	fmDemod.init(vfo->output, bw, bw/2.0f);
	resampler.init(&fmDemod.out, symrate, 0.707, symrate/250, symrate/1e4);
	slicer.init(&resampler.out);
	packetizer.init(&slicer.out, RS41_SYNCWORD, RS41_SYNCLEN, RS41_FRAME_LEN);
	sink.init(&packetizer.out, bitstreamHandler, this);

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
	fmDemod.setInput(vfo->output);
	resampler.setInput(&fmDemod.out);
	slicer.setInput(&resampler.out);
	packetizer.setInput(&slicer.out);
	sink.setInput(&packetizer.out);

	fmDemod.start();
	resampler.start();
	slicer.start();
	packetizer.start();
	sink.start();
	enabled = true;
}

void
RadiosondeDecoderModule::disable() {
	sink.stop();
	packetizer.stop();
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
		ImGui::TableNextColumn();
		ImGui::Text("S2093283");

		ImGui::EndTable();
	}

	if (!_this->enabled) style::endDisabled();
}

void
RadiosondeDecoderModule::bitstreamHandler(uint8_t *data, int count, void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;

	for (; count>0; count--) {
		std::cout << *data++;
	}
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
