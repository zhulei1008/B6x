@echo off & setlocal EnableDelayedExpansion

REM Usage: conv.bat <[--bin] [--asm] [--mif] [--rom] [--otp]> <file.axf or file.hex> [--output <out_path>] [--addr <otp_addr>]
REM Keil User: ..\tools\conv.bat --bin --mif  "#L" --output "$P.\out"
:: Keil directory of fromelf.exe
set fromelf="C:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe"

set hex2mif="%~dp0%hex2mif.py"
set hex2hex="%~dp0%hex2hex.py"
set mif_rom="%~dp0%mif_rom.py"
set mif_otp="%~dp0%mif_otp.py"

set /a t_arg=0
set /a t_bin=0
set /a t_asm=0
set /a t_mif=0
set /a t_rom=0
set /a t_otp=0
set /a t_out=0
set /a t_adr=0
set p_out=
set p_adr=
set f_src=
set f_name=

:: parse argv
call:argv %1
call:argv %2
call:argv %3
call:argv %4
call:argv %5
call:argv %6
call:argv %7
call:argv %8
call:argv %9

:: judge argv
if "%f_src%"==""  goto help

::if %t_bin% == 0 ( if %t_mif% == 0 goto help )
if %t_bin%+%t_asm%+%t_mif%+%t_rom%+%t_otp% == 0 goto help

if not exist "%f_src%" (
	echo error: "%f_src%" file not exist
	goto end
)

if "%p_out%" NEQ "" (
	if not exist "%p_out%" (
		md "%p_out%"
	)
    
	if "%p_out:~-1%" == "\" (
        set p_out=%p_out:~0,-1%
    )
)

::echo t_bin=%t_bin% t_asm=%t_asm% t_mif=%t_mif% t_out=%t_out% p_out=%p_out% f_src=%f_src% f_name=%f_name% p_adr=%p_adr%

:: conv *.axf to *.hex, used '--mif'
set ftype=%f_src:~-4%
if "%ftype%"==".axf" (
	set f_hex=%f_src:~0,-4%.hex
	set f_axf=%f_src%

	if not exist "!f_hex!" (
		if (%t_mif%+%t_rom%+%t_otp% NEQ 0 (
			echo ^>  fromelf --i32combined --output "!f_hex!" "!f_axf!"
			%fromelf% --i32combined --output "!f_hex!" "%f_src%"
		)
	)
) else if "%ftype%"==".hex" (
	set f_hex=%f_src%
) else (
	goto help
)

:: conv *.axf to *.bin, used '--bin'
if %t_bin% == 1 (
	if exist "%f_axf%" (
		if exist "%p_out%" (
			set f_bin=%p_out%^\%f_name:~0,-4%.bin
		) else (
			set f_bin=%f_axf:~0,-4%.bin
		)
		
		echo ^>  fromelf --bin --output "!f_bin!" "%f_axf%"
		%fromelf% --bin --output "!f_bin!" "%f_src%"

	) else (
		echo error: "%f_src%" isn't *.axf, used '--bin'
		goto end
	)
)

:: conv *.axf to *.asm, used '--asm'
if %t_asm% == 1 (
	if exist "%f_axf%" (
		if exist "%p_out%" (
			set f_asm=%p_out%^\%f_name:~0,-4%.asm
		) else (
			set f_asm=%f_axf:~0,-4%.asm
		)
		
		echo ^>  fromelf --text -a -c --output "!f_asm!" "%f_axf%"
		%fromelf% --text -a -c --output "!f_asm!" "%f_src%"

	) else (
		echo error: "%f_src%" isn't *.axf, used '--asm'
		goto end
	)
)

:: conv *.hex to *.mif and _SIM.hex, used '--mif'
if %t_mif% == 1 (
	if exist "%f_hex%" (
		echo ^>  python hex2mif.py
		python %hex2mif% "%f_hex%" "%p_out%"
		
		echo ^>  python hex2hex.py
		python %hex2hex% "%f_hex%" "%p_out%"
	) else (
		echo error: "%f_hex%" file not exist, used '--mif'
		goto end
	)
)

if %t_rom% == 1 (
	if exist "%f_hex%" (
		echo ^>  python mif_rom.py
		python %mif_rom% "%f_hex%" "%p_out%"
	) else (
		echo error: "%f_hex%" file not exist, used '--rom'
		goto end
	)
)

if %t_otp% == 1 (
	if exist "%f_hex%" (
		echo ^>  python mif_otp.py
		python %mif_otp% "%f_hex%" "%p_out%"  "%p_adr%"
	) else (
		echo error: "%f_hex%" file not exist, used '--otp'
		goto end
	)
)

goto end

::help info
:help
echo Cmd Format:
echo 	conv.bat ^<[--bin] [--mif] [--rom] [--otp]^> ^<file.axf or file.hex^> [--output ^<out_path^>] [--base <otp_addr>]
echo Example:
echo 	conv.bat --bin ".\output\hybxx.axf
echo 	conv.bat --bin --mif "#L" --output "$P.\bin"
goto end

::arg parse func
:argv
if "%1"=="" ( 
	set /a t_arg=0 
) else if "%1"=="--bin" (
	set /a t_arg=1
	set /a t_bin=1
) else if "%1"=="--asm" (
	set /a t_arg=10
	set /a t_asm=1
) else if "%1"=="--mif" (
	set /a t_arg=2
	set /a t_mif=1
) else if "%1"=="--rom" (
	set /a t_arg=6
	set /a t_rom=1
) else if "%1"=="--otp" (
	set /a t_arg=7
	set /a t_otp=1
) else if "%1"=="--output" (
	set /a t_arg=3
	set /a t_out=1
) else if "%1"=="--addr" (
	set /a t_arg=8
	set /a t_adr=1
) else (
	if %t_arg%==3 (
		set /a t_arg=4
		set p_out=%~1
		REM if not exist "%p_out%" ( md "%p_out%" )
	) else if %t_arg%==8 (
		set /a t_arg=9
		set p_adr=%~1
		REM if not exist "%p_out%" ( md "%p_out%" )
	) else (
		set /a t_arg=5
		if "%f_src%"=="" (
			set f_src=%~1
			set f_name=%~nx1
		)
	)
)
goto:eof

:end
