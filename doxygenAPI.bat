@Echo off & setlocal EnableDelayedExpansion

REM Usage: doxygenAPI.bat
:: 代码注释规范：https://www.cnblogs.com/silencehuan/p/11169084.html

SET DoxygenExe="C:\Program Files\Doxygen\doxygen\bin\doxygen.exe"
SET Prj_Output_Directory="docs\APIs\html"
::SET Prj_Output_CHI="docs\APIs\html\*.chi"
::SET Prj_Output_CHM="docs\APIs\html\*.chm"

SET Prj_Name="SDK API"
SET Prj_NUMBER="V1.4.0"
SET Prj_BRIEF=%date:~0,4%/%date:~5,2%/%date:~8,2%

Echo ************SDK API************
Echo.

IF EXIST %Prj_Output_Directory% (
	Echo 正在删除旧文件%Prj_Output_Directory%
	rmdir /s/q %Prj_Output_Directory%
) 

(type Doxyfile_SDK_API & ^
echo PROJECT_NAME=%Prj_Name% & ^
echo PROJECT_NUMBER=%Prj_NUMBER% & ^
echo PROJECT_BRIEF=%Prj_BRIEF%) | %DoxygenExe% -

IF %errorlevel% equ 0 GOTO End

:Err
ECHO "--FAIL(errCode:%errorlevel%)--"
pause
EXIT /b

:End
ECHO "Successful!"

choice /T 2 /C ync /CS /D y /n 
