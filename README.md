# Fynx
ELF/DWARF Visualizer and off-line calibration tool

This application is able to process .elf files containing both ELF and DWARF information. It is aimed at cross-compiled binaries for microcontrollers.

The tool allows to offline calibrate the .hex binaries. In a nutshell, off-line calibration allows for changes in behaviour of the code by adjusting symbol values without the need to recompile.

![alt text](https://github.com/deve-newbe/Fynix/blob/main/Res/flow.png "Logo Title Text 1")

## Configure project (STM32CubeIDE)

Generate ELF with DWARF 4 debug information

![alt text](https://github.com/deve-newbe/Fynix/blob/main/Res/config_debug.png "Logo Title Text 1"){ width=400px }

Generate HEX file

![alt text](https://github.com/deve-newbe/Fynix/blob/main/Res/config_hex.png "Logo Title Text 1"){ width=400px }
