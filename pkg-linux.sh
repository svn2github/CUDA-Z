#!/bin/sh
#	\file pkg-linux.sh
#	\brief linux package generator.
#	\author AG
#

#set -x

czPkgHeader=pkg-linux.dat

# Script $czPkgHeader expects variables:
# - czNameShort - short name of cuda-z
# - czVersion - version of cuda-z
# - czPackSuffix - packer suffix: "gz"
# - czExeName - name of of cuda-z
# - czExeSize - size of packed cuda-z
# - czExeSum - CRC of cuda-z
# - czDllName - name of libcudart.so.*
# - czDllSize - size of packed libcudart.so.*
# - czDllSum - CRC of libcudart.so.*
# Order of files in package: 1 - cuda-z.gz, 2 - libcudart.so.*

czPackSuffix=gz # Let's assume gzip is more common than bzip2...
czExeName=cuda-z
czExeDir=bin
czDllName= #libcudart.so.3 # Warning: check this with ldd!
czDllDir= #/usr/local/cuda/lib

czVerFile=src/version.h
czBldFile=src/build.h

if test -z "$czDllName" -o -z "$czDllDir"; then
	czDll=`ldd bin/cuda-z | grep -i cudart | sed -e "s/^.*=> *//" -e "s/ *(.*$//"`
	if test -f "$czDll"; then
		czDllName=`basename $czDll`
		czDllDir=`dirname $czDll`
	else
		echo "Can't find CUDART library!"; exit 1
	fi
fi

strip $czExeDir/$czExeName
czExeSum=`md5sum $czExeDir/$czExeName | sed -e "s/ .*$//"`
if test -z "$czExeSum"; then
	echo "Can't get md5 sum of $czExeDir/$czExeName!"; exit 1
fi

czDllSum=`md5sum $czDllDir/$czDllName | sed -e "s/ .*$//"`
if test -z "$czDllSum"; then
	echo "Can't get md5 sum of $czDllDir/$czDllName!"; exit 1
fi

czVerMajor=`cat $czVerFile | grep "CZ_VER_MAJOR" | sed -e "s/^.*MAJOR//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMajor"; then
	echo "Can't get \$czVerMajor!"; exit 1
fi

czVerMinor=`cat $czVerFile | grep "CZ_VER_MINOR" | sed -e "s/^.*MINOR//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czVerMinor"; then
	echo "Can't get \$czVerMinor!"; exit 1
fi

czBldNum=`cat $czBldFile | grep "CZ_VER_BUILD[^_]" | sed -e "s/^.*BUILD//" -e "s,/\*.*\*/,," -e "s/[ \t]//g"`
if test -z "$czBldNum"; then
	echo "Can't get \$czBldNum! Assume 0!"; czBldNum=0
fi

czVersion="$czVerMajor.$czVerMinor.$czBldNum"

czNameShort=`cat $czVerFile | grep "CZ_NAME_SHORT" | sed -e "s/^.*SHORT//" -e "s,/\*.*\*/,," -e "s/[ \t]//g" -e "s/\"//g"`
if test -z "$czNameShort"; then
	echo "Can't get \$czNameShort!"; exit 1
fi

osArch=`uname -m`

outFile="$czNameShort-$czVersion-$osArch.run"

tmpDir=/tmp
randID=`head -c8 /dev/random | od -x | head -n1 | sed -e "s/^[0-9]* //g" | tr " " -`
outDir=$tmpDir/$czNameShort-$randID

mkdir $outDir
if test $? != 0; then echo "Cannot create temp directory."; exit 1; fi

case "$czPackSuffix" in
gz) czPack="gzip -9c" ;;
bz2) czPack="bzip2 -9c" ;;
*) echo "Unknown packer suffix!"; exit 1
esac

$czPack $czExeDir/$czExeName > $outDir/$czExeName.$czPackSuffix
czExeSize=`stat -c%s $outDir/$czExeName.$czPackSuffix`
if test -z "$czExeSize"; then
	echo "Can't get \$czExeSize!"; exit 1
fi

$czPack $czDllDir/$czDllName > $outDir/$czDllName.$czPackSuffix
czDllSize=`stat -c%s $outDir/$czDllName.$czPackSuffix`
if test -z "$czDllSize"; then
	echo "Can't get \$czDllSize!"; exit 1
fi

echo "#!/bin/sh" >$outFile
echo "czNameShort=$czNameShort" >>$outFile
echo "czVersion=$czVersion" >>$outFile
echo "czPackSuffix=$czPackSuffix" >>$outFile
echo "czExeName=$czExeName" >>$outFile
echo "czExeSize=$czExeSize" >>$outFile
echo "czExeSum=$czExeSum" >>$outFile
echo "czDllName=$czDllName" >>$outFile
echo "czDllSize=$czDllSize" >>$outFile
echo "czDllSum=$czDllSum" >>$outFile

cat $czPkgHeader >>$outFile

echo "exit 0" >>$outFile
echo "# End of code" >>$outFile

cat $outDir/$czExeName.$czPackSuffix >>$outFile
cat $outDir/$czDllName.$czPackSuffix >>$outFile

chmod +x $outFile

rm -Rf $outDir
exit 0
