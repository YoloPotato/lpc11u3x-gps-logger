echo off
rem make.bat
rem
rem Create firmware binary file for LPC11U35 on windows operating system.
rem Requires https://launchpad.net/gcc-arm-embedded/ “Win32 Installer”
rem
set TARGET=usb_test
rem Change the following path if required:
set GNUARMPATH="C:\Program Files (x86)\GNU Tools ARM Embedded\5.4 2016q2"

set GCC=%GNUARMPATH%\bin\arm-none-eabi-gcc.exe
set OBJCOPY=%GNUARMPATH%\bin\arm-none-eabi-objcopy.exe
set CCFLAGS=-DCORE_M0 -Os -flto -mthumb -mcpu=cortex-m0 -Wall -ffunction-sections -fdata-sections -std=gnu99 
set LDFLAGS=-Wl,--gc-sections  -Wl,--undefined=arm_stack_area -Wl,--undefined=__isr_vector
set LIBS=--specs=nano.specs -lc -lc -lnosys -L. -T lpc11u35.ld
set GCCINC=-I. -I..\lpc_chip_11uxx_lib\inc
set SRC=
for %%f in (..\lpc_chip_11uxx_lib\src\*.c) do call set SRC=%%SRC%% %%f
for %%f in (*.c) do call set SRC=%%SRC%% %%f
echo on
%GCC% %GCCINC% %CCFLAGS%%SRC% %LDFLAGS% %LIBS% -o %TARGET%.elf
%OBJCOPY% -O binary --gap-fill 255 --pad-to 65536 %TARGET%.elf firmware.bin
