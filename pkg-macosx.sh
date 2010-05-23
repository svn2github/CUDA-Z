#!/bin/sh
#	\file pkg-macosx.sh
#	\brief MacOSX package generator.
#	\author AG
#

#set -x

czAppName=CUDA-Z
czAppBinPath=bin/$czAppName.app/Contents/MacOS
czAppResPath=bin/$czAppName.app/Contents/Resources
czAppLibs="libcudart.dylib libtlshook.dylib"
czLibPath=/usr/local/cuda/lib
czQtPath=`qmake --version | tail -n1 | sed -e "s/^Using .* in //"`
czQtGuiResPath=$czQtPath/QtGui.framework/Resources

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
