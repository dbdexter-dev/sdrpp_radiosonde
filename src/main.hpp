#pragma once

#include <module.h>
#include <dsp/demodulator.h>
#include "decode/common.hpp"
#include "decode/rs41/decoder.hpp"
#include "demod/gardner.hpp"
#include "demod/slicer.hpp"

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
	dsp::Slicer slicer;
	RS41Decoder rs41Decoder;
	dsp::HandlerSink<SondeData> sink;
	SondeData lastData;

	static void menuHandler(void *ctx);
	static void sondeDataHandler(SondeData *data, int count, void *ctx);
};
