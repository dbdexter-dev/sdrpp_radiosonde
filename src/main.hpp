#pragma once

#include "dsp/block.h"
#include <module.h>
#include <dsp/demodulator.h>
#include <dsp/deframing.h>
#include <signal_path/signal_path.h>
#include "decode/decoder.hpp"
#include "gpx.hpp"
#include "ptu.hpp"

/* Display name, symbol rate, bandwidth, syncword, syncword length (bits), frame length (bits), decoder */
typedef std::tuple<const char*, dsp::generic_unnamed_block*> sondespec_t;

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

	radiosonde::Decoder<RS41Decoder, rs41_decoder_init, rs41_decoder_deinit, rs41_decode> rs41decoder;
	radiosonde::Decoder<DFM09Decoder, dfm09_decoder_init, dfm09_decoder_deinit, dfm09_decode> dfm09decoder;
	radiosonde::Decoder<IMS100Decoder, ims100_decoder_init, ims100_decoder_deinit, ims100_decode> ims100decoder;
	radiosonde::Decoder<M10Decoder, m10_decoder_init, m10_decoder_deinit, m10_decode> m10decoder;

	const sondespec_t supportedTypes[4] = {
		sondespec_t("RS41", &rs41decoder),
		sondespec_t("DFM06/09", &dfm09decoder),
		sondespec_t("IMS100", &dfm09decoder),
		sondespec_t("M10", &m10decoder),
	};
	int selectedType = -1;
	dsp::generic_unnamed_block *activeDecoder;

	SondeFullData lastData;
	GPXWriter gpxWriter;
	PTUWriter ptuWriter;

	static void menuHandler(void *ctx);
	static void sondeDataHandler(SondeFullData *data, void *ctx);
	static void onTypeSelected(void *ctx, int selection);
	static void onGPXOutputChanged(void *ctx);
	static void onPTUOutputChanged(void *ctx);
};
