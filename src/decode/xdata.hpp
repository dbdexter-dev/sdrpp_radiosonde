#pragma once

#define XDATA_ENSCI_OZONE 0x05

#include <iostream>
#include "common.hpp"

std::string decodeXDATA(const SondeData *data, const char *asciiData, int len);

