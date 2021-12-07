#include <iomanip>
#include <sstream>
#include "xdata.hpp"

#define FLOWRATE 30     /* Time taken for the pump to force 100mL of air through the sensor, in seconds */

/* https://www.en-sci.com/wp-content/uploads/2020/02/Ozonesonde-Flight-Preparation-Manual.pdf */
/* https://www.vaisala.com/sites/default/files/documents/Ozone%20Sounding%20with%20Vaisala%20Radiosonde%20RS41%20User%27s%20Guide%20M211486EN-C.pdf */

const static float _cfPressure[] = {3, 5, 7, 10, 15, 20, 30, 50, 70, 100, 150, 200};
const static float _cfFactor[]   = {1.24, 1.124, 1.087, 1.066, 1.048, 1.041, 1.029, 1.018, 1.013, 1.007, 1.002, 1};
static float o3CorrectionFactor(float pressure);

std::string
decodeXDATA(const SondeData *curData, const char *asciiData, int len)
{
	unsigned int instrumentID, instrumentNum;
	std::ostringstream ssDecoded;

	while (len > 0) {
		sscanf(asciiData, "%02X%02X", &instrumentID, &instrumentNum);
		asciiData += 4;
		len -= 4;

		switch (instrumentID) {
			case XDATA_ENSCI_OZONE:
				unsigned int r_pumpTemp, r_o3Current, r_battVoltage, r_pumpCurrent, r_extVoltage;
				float pumpTemp, o3Current, o3Pressure, o3PPB;

				if (sscanf(asciiData, "%04X%05X%02X%03X%02X", &r_pumpTemp, &r_o3Current, &r_battVoltage, &r_pumpCurrent, &r_extVoltage) == 5) {
					asciiData += 16;
					pumpTemp = (r_pumpTemp & 0x8000 ? -1 : 1) * 0.001 * (r_pumpTemp & 0x7FFF) + 273.15;
					o3Current = r_o3Current * 1e-5;

					o3Pressure = 4.307e-3 * o3Current * pumpTemp * FLOWRATE * o3CorrectionFactor(curData->pressure);
					o3PPB = o3Pressure * 1000.0 / curData->pressure;

					ssDecoded << "O3=" << std::fixed << std::setprecision(2) << o3PPB << "ppb";
				} else {
					/* Diagnostic data */
					asciiData += 17;
				}
				break;
			default:
				break;
		}
	}


	return ssDecoded.str();
}

static float
o3CorrectionFactor(float pressure)
{
	for (int i=0; i<sizeof(_cfFactor)/sizeof(_cfFactor[0]); i++) {
		if (pressure < _cfPressure[i]) return _cfFactor[i];
	}
	return 1;
}
