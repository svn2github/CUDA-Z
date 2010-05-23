#!/bin/sh
#	\file pkg-macosx.sh
#	\brief MacOSX package generator.
#	\author AG
#

#set -x

czAppName=CUDA-Z
czAppDir=bin/$czAppName.app
czAppBinPath=$czAppDir/Contents/MacOS
czAppResPath=$czAppDir/Contents/Resources
czAppLibs="libcudart.dylib libtlshook.dylib"
czLibPath=/usr/local/cuda/lib
czQtPath=`qmake --version | tail -n1 | sed -e "s/^Using .* in //"`
czQtGuiResPath=$czQtPath/QtGui.framework/Resources

czVerFile=src/version.h
czBldFile=src/build.h

#install_name_tool -change @rpath/libcudart.dylib @executable_path/libcudart.dylib bin/CUDA-Z.app/Contents/MacOS/CUDA-Z
#install_name_tool -change @rpath/libcudart.dylib @executable_path/libcudart.dylib bin/CUDA-Z.app/Contents/MacOS/libcudart.dylib
#install_name_tool -change @rpath/libtlshook.dylib @executable_path/libtlshook.dylib bin/CUDA-Z.app/Contents/MacOS/libtlshook.dylib

for lib in $czAppLibs
do
	cp $czLibPath/$lib $czAppBinPath
	install_name_tool -change @rpath/$lib @executable_path/$lib $czAppBinPath/$lib
	install_name_tool -change @rpath/$lib @executable_path/$lib $czAppBinPath/$czAppName
done

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

if test $? != 0; then echo "Cannot create temp directory."; exit 1; fi

tmpDmg="tmp.dmg"

sudo -v

dmgSize=`du -sk $czAppDir | tr "\t" " " | sed -e 's/ .*$//'`
dmgSize=$((${dmgSize}/1000+1))
hdiutil create "$tmpDmg" -megabytes ${dmgSize} -ov -type UDIF
#dmgDisk=`hdid "$tmpDmg" | sed -ne ' /Apple_partition_scheme/ s|^/dev/([^ ]*).*$|1|p'`
dmgDisk=`hdid -nomount "$tmpDmg" |awk '/scheme/ {print substr ($1, 6, length)}'`
newfs_hfs -v "$outVol" /dev/r${dmgDisk}s1
hdiutil eject $dmgDisk

hdid "$tmpDmg"
#sudo cp -Rv "$czAppDir" "/Volumes/$outVol"
mkdir "/Volumes/$outVol/$czAppName.app"
sudo ditto -rsrcFork -v "$czAppDir" "/Volumes/$outVol/$czAppName.app"
sudo chflags -R nouchg,noschg "/Volumes/$outVol/*"
ls -l "/Volumes/$outVol"
#sudo ditto -rsrcFork -v "$czAppDir" "/Volumes/$outVol"
hdiutil eject $dmgDisk

rm -f "$outFile"
hdiutil convert "$tmpDmg" -format UDZO -o "$outFile"
rm -f "$tmpDmg"

exit 0

