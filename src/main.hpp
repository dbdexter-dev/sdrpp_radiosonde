#pragma once

#include <module.h>
#include <dsp/demodulator.h>
#include "decode/common.hpp"
#include "decode/framer.hpp"
#include "decode/null/decoder.hpp"
#include "decode/rs41/decoder.hpp"
#include "demod/gardner.hpp"
#include "demod/slicer.hpp"
#include "gpx.hpp"
#include "ptu.hpp"

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
	bool gpxOutput = false, ptuOutput = false;
	char gpxFilename[2048];
	char ptuFilename[2048];
	int selectedType;
	const std::tuple<const char*, float, float> supportedTypes[1] = {
		std::tuple<const char*, float, float>("RS41", 4800.0, 1e4),
	};

	float symRate, bw;
	VFOManager::VFO *vfo;
	dsp::FloatFMDemod fmDemod;
	dsp::GardnerResampler resampler;
	dsp::Slicer slicer;
	dsp::Framer framer;
	RS41Decoder rs41Decoder;
	NullDecoder nullDecoder;
	SondeData lastData;
	GPXWriter gpxWriter;
	PTUWriter ptuWriter;

	static void menuHandler(void *ctx);
	static void sondeDataHandler(SondeData *data, void *ctx);
	static void onTypeSelected(void *ctx, int selection);
	static void onGPXOutputChanged(void *ctx);
	static void onPTUOutputChanged(void *ctx);
};
