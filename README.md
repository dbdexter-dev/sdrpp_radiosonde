Radiosonde decoder plugin for SDR++
===================================


Build instructions
------------------

1. Download the SDR++ source code: `git clone https://github.com/AlexandreRouma/SDRPlusPlus`
2. Open the top-level `CMakeLists.txt` file, and add the following line in the
   `# Decoders` section at the top:
```
option(OPT_BUILD_RADIOSONDE_DECODER "Build the radiosonde decoder module (no dependencies required)" ON)
```
3. In that same file, search for the second `# Decoders` section, and add the
   following lines:
```
if (OPT_BUILD_RADIOSONDE_DECODER)
add_subdirectory("decoder_modules/radiosonde_decoder")
endif(OPT_BUILD_RADIOSONDE_DECODER)
```
4. Navigate to the `decoder_modules` folder, then clone this repository in the `radiosonde_decoder` folder: `git clone https://github.com/dbdexter-dev/sdrpp_radiosonde radiosonde_decoder`
5. Build SDR++ as usual: `cd .. && mkdir build && cd build && cmake .. && make && sudo make install`
6. Enable the module by adding it via the module manager

