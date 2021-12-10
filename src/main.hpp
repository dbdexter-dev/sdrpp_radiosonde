#pragma once

#include <module.h>
#include <dsp/demodulator.h>
#include "decode/common.hpp"
#include "decode/framer.hpp"
#include "decode/null/decoder.hpp"
#include "decode/rs41/decoder.hpp"
#include "decode/dfm09/decoder.hpp"
#include "demod/gardner.hpp"
#include "demod/slicer.hpp"
#include "gpx.hpp"
#include "ptu.hpp"

typedef std::tuple<const char*, float, float, uint64_t, int, int, dsp::generic_unnamed_block*> sondespec_t;

class RadiosondeDecoderModule : public ModuleManager::Instance {
public:
	RadiosondeDecoderModule(std::string name);
	~RadiosondeDecoderModule();

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
	VFOManager::VFO *vfo;
	dsp::FloatFMDemod fmDemod;
	dsp::GardnerResampler resampler;
	dsp::Slicer slicer;
	dsp::Framer framer;

	RS41Decoder rs41Decoder;
	DFM09Decoder dfm09Decoder;
	NullDecoder nullDecoder;

	const sondespec_t supportedTypes[2] = {
		sondespec_t("RS41", RS41_BAUDRATE, 1e4, RS41_SYNCWORD, RS41_SYNC_LEN, RS41_FRAME_LEN, &rs41Decoder),
		sondespec_t("DFM09", DFM09_BAUDRATE, 1e4, DFM09_SYNCWORD, DFM09_SYNC_LEN, DFM09_FRAME_LEN, &dfm09Decoder),
	};
	int selectedType = -1;
	dsp::generic_unnamed_block *activeDecoder;

	SondeData lastData;
	GPXWriter gpxWriter;
	PTUWriter ptuWriter;

	static void menuHandler(void *ctx);
	static void sondeDataHandler(SondeData *data, void *ctx);
	static void onTypeSelected(void *ctx, int selection);
	static void onGPXOutputChanged(void *ctx);
	static void onPTUOutputChanged(void *ctx);
};
