# Fynx
ELF/DWARF Visualizer and off-line calibration tool

This application is able to process .elf files containing both ELF and DWARF information. It is aimed at cross-compiled binaries for microcontrollers.

The tool allows to offline calibrate the .hex binaries. In a nutshell, off-line calibration allows for changes in behaviour of the code by adjusting symbol values without the need to recompile.

<div align="center">
    <img src="https://github.com/deve-newbe/Fynix/blob/main/Res/flow.png" alt="alt text" width="500">
</div>

## Configure project (STM32CubeIDE)

Generate ELF with DWARF 4 debug information

<div align="center">
  <img src="https://github.com/deve-newbe/Fynix/blob/main/Res/config_debug.png" alt="alt text" width="500">
</div>

Generate HEX file

<div align="center">
  <img src="https://github.com/deve-newbe/Fynix/blob/main/Res/config_hex.png" alt="alt text" width="500">
</div>
