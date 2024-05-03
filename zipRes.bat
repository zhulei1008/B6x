@Echo off & setlocal EnableDelayedExpansion

REM Usage: zipRes.bat

SET ZipExe="C:\Program Files\7-Zip\7z.exe"

Echo ************Release SDK************
Echo.
Echo Pelease Input SDK Name, eg.SDK6_v1.0
SET /p SdkName=^[SDK Name^] 

Echo.
Echo Choice Release Mode(1:Full with docs 2:Lite without docs)
CHOICE /C 12  
if %errorlevel%==1 SET SdkMode=1
if %errorlevel%==2 SET SdkMode=2

Echo.
Echo Release(Name: "%SdkName%.zip" Mode: %SdkMode%)
CHOICE /C YN 
if %errorlevel%==2 goto Quit

REM Add exclude files or dirs to temp file(ex_fl.txt)
(
Echo usb/mdk
Echo usb/src
Echo usb/usbd
Echo drivers/gnu
Echo drivers/iar
Echo drivers/mdk
Echo drivers/api/svn_ver.h
Echo drivers/src/build.c
Echo drivers/src/RTT
Echo drivers/src/core.c
Echo findmy
Echo Testing
Echo Doxyfile_SDK_API
Echo core/B6x.SFR
Echo projects/QKIE_Keybd
Echo projects/QKIE_Keybd_2G4_OK
Echo projects/QKIE_Mouse
Echo projects/QKIE_Mouse_SOP16
Echo projects/QKIE_USB_Receive_KB_Mouse
Echo projects/QKIE_USB_Receive_Mouse
Echo tools/*.py
Echo tools/*.bat
Echo *.txt
Echo *.bat
Echo *.zip
) > ex_fl.txt

if %SdkMode%==2 (
	(
	Echo doc
	Echo tools\UartTool
	) >> ex_fl.txt
)

%ZipExe% a -tzip "%SdkName%.zip" -x"@ex_fl.txt"
DEL /q ex_fl.txt

IF %errorlevel% equ 0 GOTO End

:Err
ECHO "--FAIL(errCode:%errorlevel%)--"
pause
EXIT /b

:Quit
EXIT /b

:End
ECHO "Successful!"

choice /T 2 /C ync /CS /D y /n 
