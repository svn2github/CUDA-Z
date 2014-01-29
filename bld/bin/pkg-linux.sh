#!/bin/sh
#	\file pkg-linux.sh
#	\brief linux package generator.
#	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://ag.embedded.org.ru/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

#set -x

czExeName="cuda-z"
czExeDir="bin"
czUpx="bld/bin/upx-linux"

czVerFile="src/version.h"
czBldFile="src/build.h"

strip "$czExeDir/$czExeName"

czVerMajor=`cat "$czVerFile" | grep "^#define CZ_VER_MAJOR" | sed -e "s/^.*MAJOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czVerMajor" ]; then
	echo "Can't get \$czVerMajor!"
	exit 1
fi

czVerMinor=`cat "$czVerFile" | grep "^#define CZ_VER_MINOR" | sed -e "s/^.*MINOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czVerMinor" ]; then
	echo "Can't get \$czVerMinor!"
	exit 1
fi

czBldNum=`cat "$czBldFile" | grep "^#define CZ_VER_BUILD[^_]" | sed -e "s/^.*BUILD//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if [ -z "$czBldNum" ]; then
	echo "Can't get \$czBldNum! Assume 0!"
	czBldNum=0
fi

czVersion="$czVerMajor.$czVerMinor.$czBldNum"

czNameShort=`cat "$czVerFile" | grep "^#define CZ_NAME_SHORT" | sed -e "s/^.*SHORT//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if [ -z "$czNameShort" ]; then
	echo "Can't get \$czNameShort!"
	exit 1
fi

czBldState=`cat "$czVerFile" | grep "^#define CZ_VER_STATE" | sed -e "s/^.*STATE//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
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

rm -f "$outFile"

$czUpx --ultra-brute -o"$outFile" "$czExeDir/$czExeName"
chmod +x "$outFile"

exit 0
