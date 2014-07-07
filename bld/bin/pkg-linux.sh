#!/bin/sh
#	\file pkg-linux.sh
#	\brief Linux package generator.
#	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

#set -x

czExeName="cuda-z"
czExeDir="bin"
czUpx="bld/bin/upx-linux"

czSourceDir=.
czBuildDir=.

czVerFile="src/version.h"
czBldFile="src/build.h"

if [ $# -gt 0 ]; then
	czSourceDir="$1"
fi

if [ $# -gt 1 ]; then
	czBuildDir="$2"
fi

if [ ! -r "$czSourceDir/$czVerFile" ]; then
	echo Can\'t find $czSourceDir/$czVerFile
	exit 1
fi

if [ ! -r "$czBuildDir/$czBldFile" ]; then
	echo Can\'t find $czBuildDir/$czBldFile
	exit 1
fi

strip "$czBuildDir/$czExeDir/$czExeName"

czVerMajor=`cat "$czSourceDir/$czVerFile" | grep "^#define CZ_VER_MAJOR" | sed -e "s/^.*MAJOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czVerMajor" ]; then
	echo "Can't get \$czVerMajor!"
	exit 1
fi

czVerMinor=`cat "$czSourceDir/$czVerFile" | grep "^#define CZ_VER_MINOR" | sed -e "s/^.*MINOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czVerMinor" ]; then
	echo "Can't get \$czVerMinor!"
	exit 1
fi

czBldNum=`cat "$czBuildDir/$czBldFile" | grep "^#define CZ_VER_BUILD[^_]" | sed -e "s/^.*BUILD//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czBldNum" ]; then
	echo "Can't get \$czBldNum! Assume 0!"
	czBldNum=0
fi

czVersion="$czVerMajor.$czVerMinor.$czBldNum"

czNameShort=`cat "$czSourceDir/$czVerFile" | grep "^#define CZ_NAME_SHORT" | sed -e "s/^.*SHORT//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if [ -z "$czNameShort" ]; then
	echo "Can't get \$czNameShort!"
	exit 1
fi

czBldState=`cat "$czSourceDir/$czVerFile" | grep "^#define CZ_VER_STATE" | sed -e "s/^.*STATE//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if [ -z "$czBldState" ]; then
	echo "Can't get \$czBldState! Assume empty!"
	czBldState=""
fi

if [ ! -z "$czBldState" ]; then
	outFile="$czNameShort-$czVersion-$czBldState.run"
	czVersion="$czVersion $czBldState"
else
	outFile="$czNameShort-$czVersion.run"
fi

rm -f "$czBuildDir/$outFile"

$czSourceDir/$czUpx --ultra-brute -o"$czBuildDir/$outFile" "$czBuildDir/$czExeDir/$czExeName"
chmod +x "$outFile"

exit 0
