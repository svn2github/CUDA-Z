@echo off
::	\file pkg-win32.cmd
::	\brief Windows package generator.
::	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://ag.embedded.org.ru/
::	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
::	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

::set czAppName=CUDA-Z
set czExeName=cuda-z.exe
set czExeDir=bin
set czUpx=bld\bin\upx-win32.exe

set czVerFile=src\version.h
set czBldFile=src\build.h

if not exist %czVerFile% ( echo Can't find %czVerFile% && goto END )
if not exist %czBldFile% ( echo Can't find %czBldFile% && goto END )

set czVerMajor=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_MAJOR\>" %czVerFile%' ) do set czVerMajor=%%i
set czVerMinor=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_MINOR\>" %czVerFile%' ) do set czVerMinor=%%i
set czBldNum=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_BUILD\>" %czBldFile%' ) do set czBldNum=%%i
set czVersion="%czVerMajor%.%czVerMinor%.%czBldNum%"

set czNameShort=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_NAME_SHORT\>" %czVerFile%' ) do set czNameShort=%%i
set czBldState=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_STATE\>" %czVerFile%' ) do set czBldState=%%i

if "%czBldState%"=="" (
	set czOutName=%czNameShort%-%czVersion%.exe
) else (
	set czOutName=%czNameShort%-%czVersion%-%czBldState%.exe
)

del /F %czOutName%

%czUpx% --ultra-brute -o%czOutName% %czExeDir%\%czExeName%

:END
	