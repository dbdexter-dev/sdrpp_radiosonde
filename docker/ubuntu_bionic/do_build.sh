#!/bin/bash
set -e
cd /root

# Update repos to get a more recent cmake version
apt update
apt install -y gpg wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
apt update

# Install dependencies and tools
apt install -y build-essential cmake git libfftw3-dev libglfw3-dev libvolk1-dev libzstd-dev libsoapysdr-dev libairspy-dev \
            libiio-dev libad9361-dev librtaudio-dev libhackrf-dev librtlsdr-dev libbladerf-dev liblimesuite-dev p7zip-full wget portaudio19-dev \
            libcodec2-dev

# Install a more recent libairspyhf version
git clone https://github.com/airspy/airspyhf
cd airspyhf
mkdir build
cd build
cmake .. -DINSTALL_UDEV_RULES=ON
make -j2
make install
ldconfig
cd ../../

# Fix missing .pc file for codec2
echo 'prefix=/usr/' >> /usr/share/pkgconfig/codec2.pc
echo 'libdir=/usr/include/x86_64-linux-gnu/' >> /usr/share/pkgconfig/codec2.pc
echo 'includedir=/usr/include/codec2' >> /usr/share/pkgconfig/codec2.pc
echo 'Name: codec2' >> /usr/share/pkgconfig/codec2.pc
echo 'Description: A speech codec for 2400 bit/s and below' >> /usr/share/pkgconfig/codec2.pc
echo 'Requires:' >> /usr/share/pkgconfig/codec2.pc
echo 'Version: 0.7' >> /usr/share/pkgconfig/codec2.pc
echo 'Libs: -L/usr/include/x86_64-linux-gnu/ -lcodec2' >> /usr/share/pkgconfig/codec2.pc
echo 'Cflags: -I/usr/include/codec2' >> /usr/share/pkgconfig/codec2.pc

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
