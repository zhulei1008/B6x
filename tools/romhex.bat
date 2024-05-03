@echo off & setlocal EnableDelayedExpansion
set outDir=..\hexmif
set binDir=%outDir%\hyROM.bin
set hexDir=%outDir%\hyROM_hex

echo ****generate HEX for Simulation****
set romHex=%outDir%\hyROM.hex
set hex_rom=hex_rom.py

echo python %hex_rom% %romHex% %hexDir%
python %hex_rom% %romHex% %hexDir%
echo Finish
echo.


::echo ****generate HEX for Chip GDS****
::srec_cat.exe BinaryFile.bin -Binary -crop 0x000000 0x003FFF -offset 0x08010000 -o HexFile.hex -Intel -address-length=4
::set botBin="%binDir%\ER_IROM1"
::set bleBin="%binDir%\ER_IROM2"
::set botHex="%hexDir%\hyROM_BOT_intel.hex"
::set bleHex="%hexDir%\hyROM_BLE_intel.hex"
::set srec_cat=".\srec_cat.exe"

if not exist %botBin% (
	echo error: %botBin% file not exist, please rebuild by user "--bin"!
	goto end
)
if not exist "%hexDir%" (
	md "%hexDir%"
)

echo %srec_cat% %botBin% -Binary -fill 0x00 0x000000 0x008000 -o %botHex% -Intel -Output_Block_Size=16 
%srec_cat% %botBin% -Binary -fill 0x00 0x000000 0x008000 -o %botHex% -Intel -Output_Block_Size=16 
echo %srec_cat% %bleBin% -Binary -fill 0x00 0x000000 0x028000 -o %bleHex% -Intel -Output_Block_Size=16 
%srec_cat% %bleBin% -Binary -fill 0x00 0x000000 0x028000 -o %bleHex% -Intel -Output_Block_Size=16 
echo Finish

pause

:end