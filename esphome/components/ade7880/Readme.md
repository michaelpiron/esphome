# Status of this ESPHome component ADE7880

This component is work in progress.
Discussions are also ongoing here:
* https://github.com/esphome/feature-requests/issues/1579
* https://github.com/arendst/Tasmota/discussions/13515
* https://github.com/yaourdt/mgos-to-tasmota/issues/48

## Ongoing work

1/ Which are the connections between ESP and ADE chip? I have to know which GPIO pins of the ESP are connected to the IRQ’s of the ADE. I can’t see this visually on the PCBs without desoldering the top and bottom PCB, and even then I’m not sure if it can be deduced from the PCB tracks.

> **Solution:** Pin connections between ESP and ADE:
> * ADE Pin 29 (/IRQ0) -> ESP8266 Pin 12
> * ADE Pin 32 (/IRQ1) -> ESP8266 Pin 24
> * ADE Pin 36 (SCLK/SCL) > ESP Pin 9, via R36 to VDD
> * ADE Pin 37 (MISO/HSD) > NC
> * ADE Pin 38 (MOSI/SDA) > ESP Pin 10, via R35 to VDD
> * ADE Pin 39 (/SS/HSA) > via R46 to VDD

> This is not yet implemented in the code in this repository.

2/ There are quite some calibration constants to figure out. Anyone who is familiar with this or who could reverse engineer it from the Shelly 3PM firmware binary?

> **Solution:** It seems that you need to download the calibration constants from your 3EM device **before** loading any new firmware on it! Calibration constants are specific to a device, if you don't download them first, the device basically becomes worthless. Need to figure out how to download it.

# Below text to be reused in ESPhome documentation

## ADE77880 Three-phase Power Sensor for ESPHome
The ade7880 sensor platform allows you to use ADE7880 three phase energy metering ICs [(datasheet)](https://www.analog.com/media/en/technical-documentation/data-sheets/ADE7880.pdf) with ESPHome. These are commonly found in Shelly 3EM devices.

The ADE7880 is suitable for measuring active, reactive, and apparent energy in various 3-phase configurations, such as wye  or delta services with, both, three and four wires. The ADE7880 provides system calibration features for each phase, that is, rms offset correction, phase calibration, and gain calibration. The CF1, CF2, and CF3 logic outputs provide a wide choice of  power information: total active powers, apparent powers, or the sum of the current rms values, and fundamental active and reactive powers.

### Types of three-phases
The ADE7880 is capable of measuring different types of 3-phase configurations: 3-wire delta, 4-wire wye. More on this in the [application note AN-639 (section METER CONFIGURATIONS)](https://www.analog.com/media/en/technical-documentation/application-notes/AN-639.pdf?doc=ADE7880.pdf).
