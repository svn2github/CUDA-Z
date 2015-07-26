@echo off
::	\file pkg-win32.cmd
::	\brief Windows package generator.
::	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
::	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
::	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

::set czAppName=CUDA-Z
set czExeName=cuda-z.exe
set czExeDir=bin
set czUpx=bld\bin\upx-win32.exe

set czSourceDir=.
set czBuildDir=.

set czVerFile=src\version.h
set czBldFile=src\build.h

set czBldSuffix=

if not "%~1" == "" set czSourceDir="%~1"
if not "%~2" == "" set czBuildDir="%~2"

if not exist %czSourceDir%\%czVerFile% ( echo Can't find %czSourceDir%\%czVerFile% && goto END )
if not exist %czBuildDir%\%czBldFile% ( echo Can't find %czBuildDir%\%czBldFile% && goto END )

cl.exe 2> %czBuildDir%\cl.tst

findstr "^Microsoft.(R).*for.80x86\>" %czBuildDir%\cl.tst > NUL
if %ERRORLEVEL% equ 0 set czBldSuffix=32bit
findstr "^Microsoft.(R).*for.x86\>" %czBuildDir%\cl.tst > NUL
if %ERRORLEVEL% equ 0 set czBldSuffix=32bit
findstr "^Microsoft.(R).*for.x64\>" %czBuildDir%\cl.tst > NUL
if %ERRORLEVEL% equ 0 set czBldSuffix=64bit

del /F %czBuildDir%\cl.tst

set czVerMajor=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_MAJOR\>" %czSourceDir%\%czVerFile%' ) do set czVerMajor=%%i
set czVerMinor=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_MINOR\>" %czSourceDir%\%czVerFile%' ) do set czVerMinor=%%i
set czBldNum=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_BUILD\>" %czBuildDir%\%czBldFile%' ) do set czBldNum=%%i
set czVersion="%czVerMajor%.%czVerMinor%.%czBldNum%"

set czNameShort=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_NAME_SHORT\>" %czSourceDir%\%czVerFile%' ) do set czNameShort=%%i
set czBldState=
for /f "tokens=3" %%i in ( 'findstr "^#define.CZ_VER_STATE\>" %czSourceDir%\%czVerFile%' ) do set czBldState=%%i

if "%czBldState%"=="" (
	set czOutName=%czNameShort%-%czVersion%
) else (
	set czOutName=%czNameShort%-%czVersion%-%czBldState%
)

if not "%czBldSuffix%"=="" set czOutName=%czOutName%-%czBldSuffix%

set czOutName=%czOutName%.exe

cd %czBuildDir%
del /F %czOutName%

%czSourceDir%\%czUpx% --ultra-brute -o%czOutName% %czExeDir%\%czExeName%

:END
	