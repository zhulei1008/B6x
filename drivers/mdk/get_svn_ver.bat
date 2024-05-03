@echo off

set src_dir=%~dp0
SET API_PATH_H=..\api\svn_ver.h
echo %src_dir%

svn update ..

::获取sdk6/drivers目录下的版本号
for /f "delims=" %%i in ('svn info .. ^| findstr "Rev:"') do set last_rev=%%i

echo %last_rev%

set last_rev=%last_rev:~18%

echo #ifndef _SVN_VER_H_ > %API_PATH_H%
echo #define _SVN_VER_H_ >> %API_PATH_H%
echo= >> %API_PATH_H%
echo #define SVN_VER %last_rev% >> %API_PATH_H%

echo= >> %API_PATH_H%
echo #endif // _SVN_VER_H_ >> %API_PATH_H%
::pause