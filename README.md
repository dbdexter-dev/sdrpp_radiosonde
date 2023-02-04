Radiosonde decoder plugin for SDR++
===================================

![radiosondeGPX](https://user-images.githubusercontent.com/17110004/144872708-2a578c62-5493-4845-9098-9328c4e914bf.png)

Compatibility:
--------------

| Manufacturer | Model       | GPS                | Temperature        | Humidity           | XDATA              |
|--------------|-------------|--------------------|--------------------|--------------------|--------------------|
| Vaisala      | RS41-SG     | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |
| Meteomodem   | M10         | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |                    |
| Meteomodem   | M20         | :heavy_check_mark: | :heavy_check_mark: |                    |                    |
| GRAW         | DFM06/09/17 | :heavy_check_mark: | :heavy_check_mark: |                    |                    |
| Meisei       | iMS-100     | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |                    |
| Meisei       | RS-11G      | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |                    |
| InterMet     | iMet-1/4    | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: |
| Meteolabor   | SRS-C50     | :heavy_check_mark: | :heavy_check_mark: |                    |                    |
| Meteo-Radiy  | MRZ-N1      | :heavy_check_mark: | :heavy_check_mark: |                    |                    |

Installing
----------

Binary releases for Windows and Linux (both x86-64 only, built for [SDR++
nightlies](https://github.com/AlexandreRouma/SDRPlusPlus/actions)) are available from
[the Releases page](https://github.com/dbdexter-dev/sdrpp_radiosonde/releases).

- **Windows**: download the `.dll` file from the latest release, and place it in
  the `modules` directory within your SDR++ installation.
- **Linux**: download the `.so` file from the latest release, and place it in
  the `/usr/lib/sdrpp/plugins` folder.

The plugin can then be enabled from the module manager in SDR++, under the name
*radiosonde\_decoder*.


Building from source
--------------------

If no binary is available for your platform, you can build this plugin from
source:

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
add_subdirectory("decoder_modules/sdrpp_radiosonde")
endif (OPT_BUILD_RADIOSONDE_DECODER)
```
4. Navigate to the `decoder_modules` folder, then clone this repository: `git clone https://github.com/dbdexter-dev/sdrpp_radiosonde --recurse-submodules`
5. Build and install SDR++ following the guide in the original repository
6. Enable the module by adding it via the module manager

