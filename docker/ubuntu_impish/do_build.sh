#!/bin/bash
set -e
cd /root

# Install dependencies and tools
apt update
apt install -y build-essential cmake git libfftw3-dev libglfw3-dev libvolk2-dev libzstd-dev libsoapysdr-dev libairspyhf-dev libairspy-dev \
            libiio-dev libad9361-dev librtaudio-dev libhackrf-dev librtlsdr-dev libbladerf-dev liblimesuite-dev p7zip-full wget portaudio19-dev \
            libcodec2-dev

# Download SDR++ source code
git clone https://github.com/AlexandreRouma/SDRPlusPlus

# Copy module code into SDR++ tree
cp -r /github/workspace SDRPlusPlus/decoder_modules/sdrpp_radiosonde

# Add module to CMakeLists
echo "add_subdirectory(\"decoder_modules/sdrpp_radiosonde\")" >> SDRPlusPlus/CMakeLists.txt

# Build
cd SDRPlusPlus
cmake -B build -DOPT_BUILD_AIRSPY_SOURCE=OFF -DOPT_BUILD_AIRSPYHF_SOURCE=OFF -DOPT_BUILD_HACKRF_SOURCE=OFF -DOPT_BUILD_RFSPACE_SOURCE=OFF -DOPT_BUILD_RTL_SDR_SOURCE=OFF -DOPT_BUILD_RTL_TCP_SOURCE=OFF -DOPT_BUILD_SPYSERVER_SOURCE=OFF -DOPT_BUILD_PLUTOSDR_SOURCE=OFF
cd build
make -j2 radiosonde_decoder
