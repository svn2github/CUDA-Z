#!/bin/sh
#	\file pkg-macosx.sh
#	\brief MacOSX package generator.
#	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html

#set -x

czAppName=CUDA-Z
czAppDir=bin/$czAppName.app
czAppPlistPath=$czAppDir/Contents/Info.plist
czAppBinPath=$czAppDir/Contents/MacOS
czAppResPath=$czAppDir/Contents/Resources
czAppLibs="libcudart.dylib libtlshook.dylib"
czLibPath=/usr/local/cuda/lib
czImgDir=res/img
czVolumeIcon=$czImgDir/VolumeIcon.icns
czMakefile=Makefile

czQmake=`grep "QMAKE *=" $czMakefile | sed -e "s/^.*=//"`
czQtPath=`$czQmake --version | tail -n1 | sed -e "s/^Using .* in //"`
#czQtGuiResPath=$czQtPath/QtGui.framework/Resources
czQtGuiResPath=$czQtPath/../src/gui/mac

czVerFile=src/version.h
czBldFile=src/build.h

for lib in $czAppLibs
do
	cp $czLibPath/$lib $czAppBinPath
	install_name_tool -change @rpath/$lib @executable_path/$lib $czAppBinPath/$lib
	install_name_tool -change @rpath/$lib @executable_path/$lib $czAppBinPath/$czAppName
	strip $czAppBinPath/$lib
done
strip $czAppBinPath/$czAppName

cp $czLibPath/libcuda.dylib $czAppBinPath
install_name_tool -change $czLibPath/libcuda.dylib @executable_path/libcuda.dylib $czAppBinPath/libcuda.dylib
strip $czAppBinPath/libcuda.dylib

#Add copy of qt_menu.nib to Resource subdirectory!
cp -R $czQtGuiResPath/qt_menu.nib $czAppResPath

czVerMajor=`cat $czVerFile | grep "CZ_VER_MAJOR" | tr "\t" " "| sed -e "s/^.*MAJOR//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMajor"; then
	echo "Can't get \$czVerMajor!"; exit 1
fi

czVerMinor=`cat $czVerFile | grep "CZ_VER_MINOR" | tr "\t" " "| sed -e "s/^.*MINOR//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMinor"; then
	echo "Can't get \$czVerMinor!"; exit 1
fi

czBldNum=`cat $czBldFile | grep "CZ_VER_BUILD[^_]" | tr "\t" " "| sed -e "s/^.*BUILD//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czBldNum"; then
	echo "Can't get \$czBldNum! Assume 0!"; czBldNum=0
fi

czVersion="$czVerMajor.$czVerMinor.$czBldNum"

czNameShort=`cat $czVerFile | grep "CZ_NAME_SHORT" | tr "\t" " " | sed -e "s/^.*SHORT//" -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if test -z "$czNameShort"; then
	echo "Can't get \$czNameShort!"; exit 1
fi

outFile="$czNameShort-$czVersion.dmg"
outVol="$czNameShort-$czVersion"

tmpPlistPath="Info.plist"

sed -e "s/\$czVersion/$czVersion/g" -e "s/\$czNameShort/$czNameShort/g" $czAppPlistPath > $tmpPlistPath
mv -f $tmpPlistPath $czAppPlistPath

tmpDmg="tmp.dmg"

sudo -v

dmgSize=`du -sk $czAppDir | tr "\t" " " | sed -e 's/ .*$//'`
dmgSize=$((${dmgSize}/1000+1))
hdiutil create "$tmpDmg" -megabytes ${dmgSize} -ov -type UDIF
dmgDisk=`hdid -nomount "$tmpDmg" | awk '/scheme/ {print substr ($1, 6, length)}'`
newfs_hfs -v "$outVol" /dev/r${dmgDisk}s1
hdiutil eject $dmgDisk

hdid "$tmpDmg"
cp $czVolumeIcon "/Volumes/$outVol/.VolumeIcon.icns"
SetFile -a C "/Volumes/$outVol/"
mkdir "/Volumes/$outVol/$czAppName.app"
sudo ditto -rsrcFork -v "$czAppDir" "/Volumes/$outVol/$czAppName.app"
sudo chflags -R nouchg,noschg "/Volumes/$outVol/$czAppName.app"
ls -l "/Volumes/$outVol"
hdiutil eject $dmgDisk

rm -f "$outFile"
hdiutil convert "$tmpDmg" -format UDBZ -imagekey bzip2-level=9 -o "$outFile"
#hdiutil convert "$tmpDmg" -format UDZO -imagekey zlib-level=9 -o "$outFile"
#cp "$tmpDmg" "$outFile"
rm -f "$tmpDmg"

exit 0

