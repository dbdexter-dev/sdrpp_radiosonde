#pragma once

#include <module.h>
#include <dsp/demodulator.h>
#include "decode/common.hpp"
#include "demod/gardner.hpp"

class RadiosondeDecoderModule : public ModuleManager::Instance {
public:
	RadiosondeDecoderModule(std::string name);
	~RadiosondeDecoderModule();

	/* Overrides for virtual methods in ModuleManager::Instance */
	void postInit() override;
	void enable() override;
	void disable() override;
	bool isEnabled() override;

private:
	std::string name;
	bool enabled = true;
	float symrate, bw;
	VFOManager::VFO *vfo;
	dsp::FloatFMDemod fmDemod;
	dsp::GardnerResampler resampler;
	dsp::HandlerSink<float> sink;
	SondeInfoStruct sondeInfo;

	static void menuHandler(void *ctx);
	static void fmStreamHandler(float *data, int count, void *ctx);
};
