#include <imgui.h>
#include <gui/gui.h>
#include <gui/style.h>
#include <module.h>
#include "main.hpp"

SDRPP_MOD_INFO {
    /* Name:            */ "radiosonde_decoder",
    /* Description:     */ "Radiosonde decoder for SDR++",
    /* Author:          */ "dbdexter-dev",
    /* Version:         */ 0, 0, 1,
    /* Max instances    */ -1
};

/* Public methods {{{ */
RadiosondeDecoderModule::RadiosondeDecoderModule(std::string name) 
{
	this->name = name;

	/* Register with main GUI */
	gui::menu.registerEntry(name, menuHandler, this, this);
}

RadiosondeDecoderModule::~RadiosondeDecoderModule() 
{
	if (isEnabled()) disable();
	gui::menu.removeEntry(name);
}

void
RadiosondeDecoderModule::enable() {
	this->enabled = true;
}
void
RadiosondeDecoderModule::disable() {
	this->enabled = false;
}
bool
RadiosondeDecoderModule::isEnabled() {
	return this->enabled;
}
void
RadiosondeDecoderModule::postInit() {
}
/* }}} */

/* Private methods {{{*/
void
RadiosondeDecoderModule::menuHandler(void *ctx)
{
	RadiosondeDecoderModule *_this = (RadiosondeDecoderModule*)ctx;

	float width = ImGui::GetContentRegionAvailWidth();

	if (!_this->enabled) style::beginDisabled();

	ImGui::SetNextItemWidth(width);
	ImGui::Button("Test", ImVec2(width, 20));

	if (!_this->enabled) style::endDisabled();
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
