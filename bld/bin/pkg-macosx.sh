#!/bin/sh
#	\file pkg-macosx.sh
#	\brief MacOSX package generator.
#	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

#set -x

czAppName="CUDA-Z"
czAppDir="bin/$czAppName.app"
czAppPlistPath="$czAppDir/Contents/Info.plist"
czAppBinPath="$czAppDir/Contents/MacOS"
czAppResPath="$czAppDir/Contents/Resources"
czAppLibs= #libcudart.dylib # check this with otool!
czLibPath="/usr/local/cuda/lib"
czImgDir="res/img"
czVolumeIcon="$czImgDir/VolumeIcon.icns"
czMakefile="Makefile"
czQtMenuNib="qt_menu.nib"
czUpx="bld/bin/upx-macosx"

czQmake=`grep "QMAKE *=" $czMakefile | sed -e "s/^.*=//"`
czQtPath=`$czQmake --version | tail -n1 | sed -e "s/^Using .* in //"`
#czQtGuiResPath="$czQtPath/QtGui.framework/Resources"
czQtGuiResPath="$czQtPath/../src/gui/mac"

czQtDocPath="$czQtPath/../doc"
if [ -h "$czQtDocPath" ]; then
	czQtRealDocPath=`readlink "$czQtDocPath"`
	if [ -d "$czQtRealDocPath" ]; then
		czQtGuiResPath="$czQtRealDocPath/../src/gui/mac"
	else
		echo "Can't find Qt Mac resources directory!"
		exit 1
	fi
fi

czVerFile="src/version.h"
czBldFile="src/build.h"

if [ -z "$czAppLibs" ]; then
	czAppLibs=`LANG=C otool -L "$czAppBinPath/$czAppName" | grep @rpath | sed -e "s/ *(.*$//" -e "s/^.*@rpath\///"`

	for lib in $czAppLibs; do
		if [ -f "$czLibPath/$lib" ]; then
			czAppLibsTemp=`LANG=C otool -L "$czLibPath/$lib" | grep @rpath | sed -e "s/ *(.*$//" -e "s/^.*@rpath\///"`
			czAppLibs="$czAppLibs $czAppLibsTemp"
		else
			echo "Can't find $lib library!"
			exit 1
		fi
	done
fi

czAppLibs=`echo $czAppLibs | tr " " "\n" | sort | uniq`

for lib in $czAppLibs; do
		echo "Warning: library $lib is required and will be added to image!"
	if [ -f "$czLibPath/$lib" ]; then
		cp "$czLibPath/$lib" "$czAppBinPath"
	else
		echo "Can't find $lib library!"
		exit 1
	fi
done

for lib in $czAppLibs; do
	for lib2 in $czAppLibs; do
		install_name_tool -change @rpath/$lib2 @executable_path/$lib2 "$czAppBinPath/$lib"
	done
	install_name_tool -change @rpath/$lib @executable_path/$lib "$czAppBinPath/$czAppName"
	strip "$czAppBinPath/$lib"
done
strip "$czAppBinPath/$czAppName"

$czUpx --ultra-brute "$czAppBinPath/$czAppName"

#Add copy of qt_menu.nib to Resource subdirectory!
#if [ -d "$czQtGuiResPath/$czQtMenuNib" ]; then
#	cp -R "$czQtGuiResPath/$czQtMenuNib" "$czAppResPath"
#else
#	echo "Can't find Qt Mac resources file $czQtGuiResPath/$czQtMenuNib!"
#	exit 1
#fi

czVerMajor=`cat "$czVerFile" | grep "^#define CZ_VER_MAJOR" | tr "\t" " "| sed -e "s/^.*MAJOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMajor"; then
	echo "Can't get \$czVerMajor!"
	exit 1
fi

czVerMinor=`cat "$czVerFile" | grep "^#define CZ_VER_MINOR" | tr "\t" " "| sed -e "s/^.*MINOR//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMinor"; then
	echo "Can't get \$czVerMinor!"
	exit 1
fi

czBldNum=`cat "$czBldFile" | grep "^#define CZ_VER_BUILD[^_]" | tr "\t" " "| sed -e "s/^.*BUILD//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czBldNum"; then
	echo "Can't get \$czBldNum! Assume 0!"
	czBldNum=0
fi

czVersion="$czVerMajor.$czVerMinor.$czBldNum"

czNameShort=`cat "$czVerFile" | grep "^#define CZ_NAME_SHORT" | tr "\t" " " | sed -e "s/^.*SHORT//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if test -z "$czNameShort"; then
	echo "Can't get \$czNameShort!"
	exit 1
fi

czBldState=`cat "$czVerFile" | grep "^#define CZ_VER_STATE" | tr "\t" " " | sed -e "s/^.*STATE//" -e "s,//.*$,," -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if [ -z "$czBldState" ]; then
	echo "Can't get \$czBldState! Assume empty!"
	czBldState=""
fi

if [ ! -z "$czBldState" ]; then
	outFile="$czNameShort-$czVersion-$czBldState.dmg"
	outVol="$czNameShort-$czVersion-$czBldState"
	czVersion="$czVersion $czBldState"
else
	outFile="$czNameShort-$czVersion.dmg"
	outVol="$czNameShort-$czVersion"
fi

tmpPlistPath="Info.plist"

sed -e "s/\$czVersion/$czVersion/g" -e "s/\$czNameShort/$czNameShort/g" $czAppPlistPath > $tmpPlistPath
mv -f "$tmpPlistPath" "$czAppPlistPath"

tmpDmg="tmp.dmg"

sudo -v

dmgSize=`du -sk "$czAppDir" | tr "\t" " " | sed -e 's/ .*$//'`
dmgSize=$((${dmgSize}/1000+1))
hdiutil create "$tmpDmg" -megabytes ${dmgSize} -ov -type UDIF
dmgDisk=`hdid -nomount "$tmpDmg" | awk '/scheme/ {print substr ($1, 6, length)}'`
newfs_hfs -v "$outVol" /dev/r${dmgDisk}s1
hdiutil eject "$dmgDisk"

hdid "$tmpDmg"
cp $czVolumeIcon "/Volumes/$outVol/.VolumeIcon.icns"
SetFile -a C "/Volumes/$outVol/"
mkdir "/Volumes/$outVol/$czAppName.app"
sudo ditto -rsrcFork -v "$czAppDir" "/Volumes/$outVol/$czAppName.app"
sudo chflags -R nouchg,noschg "/Volumes/$outVol/$czAppName.app"
ls -l "/Volumes/$outVol"
hdiutil eject "$dmgDisk"

rm -f "$outFile"
hdiutil convert "$tmpDmg" -format UDBZ -imagekey bzip2-level=9 -o "$outFile"
#hdiutil convert "$tmpDmg" -format UDZO -imagekey zlib-level=9 -o "$outFile"
rm -f "$tmpDmg"

exit 0
