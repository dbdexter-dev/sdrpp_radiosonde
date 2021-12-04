#include <iostream>
#include <imgui.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include "main.hpp"
#include "demod/gardner.hpp"

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
	sink.init(&resampler.out, fmStreamHandler, this);

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
	sink.setInput(&resampler.out);

	fmDemod.start();
	resampler.start();
	sink.start();
	enabled = true;
}
void
RadiosondeDecoderModule::disable() {
	sink.stop();
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
RadiosondeDecoderModule::fmStreamHandler(float *data, int count, void *ctx)
{
	static char tmp = 0x00;
	static int offset = 0;
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;

	for (; count>0; count--) {
		tmp = (tmp << 1) | (*data++ > 0 ? 1 : 0);
		offset++;

		if (offset >= 8) {
			offset = 0;
			std::cout << tmp;
		}
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
