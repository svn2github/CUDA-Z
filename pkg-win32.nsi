#	\file pkg-win32.nsi
#	\brief Windows package generator.
#	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html

!define CZ_CUDART_DLL cudart32_55.dll
!define CZ_CUDAZ_EXE cuda-z.exe

!define CZ_CUDA_PATH "%CUDA_PATH%"
!define CZ_CUDA_PATH_VALUE "$%CUDA_PATH%"

!if "${CZ_CUDA_PATH_VALUE}" != "$${CZ_CUDA_PATH}"
!define CZ_CUDART_DLL_PATH "$%CUDA_PATH%\bin"
!else
!define CZ_CUDART_DLL_PATH "bin"
!endif

!system "perl -pe $\"s/#/!/g;s,http://,http:!!,;s,https://,https:!!,;s,svn://,svn:!!,;s,//.*$,,;s,/\*.*\*/,,;s,http:!!,http://,;s,https:!!,https://,;s,svn:!!,svn://,$\" < src/build.h > build.nsi"
!system "perl -pe $\"s/#/!/g;s,http://,http:!!,;s,https://,https:!!,;s,svn://,svn:!!,;s,//.*$,,;s,/\*.*\*/,,;s,http:!!,http://,;s,https:!!,https://,;s,svn:!!,svn://,;s/build.h/build.nsi/g$\" < src/version.h > version.nsi"
!include version.nsi
!delfile build.nsi
!delfile version.nsi

!ifndef CZ_VER_BUILD
!define CZ_VER_BUILD "0"
!endif

!ifndef CZ_VER_STATE
!define CZ_VER_STATE ""
!endif

!define VERSION_NUM "${CZ_VER_STRING}.${CZ_VER_BUILD}"
!define NAME "${CZ_NAME_SHORT} Container"

!if "${CZ_VER_STATE}" != ""
!define FILENAME "${CZ_NAME_SHORT}-${VERSION_NUM}-${CZ_VER_STATE}.exe"
!define VERSION_STR "${CZ_VER_STRING}.${CZ_VER_BUILD} ${CZ_VER_STATE}"
!else
!define FILENAME "${CZ_NAME_SHORT}-${VERSION_NUM}.exe"
!define VERSION_STR "${CZ_VER_STRING}.${CZ_VER_BUILD}"
!endif

Name "${NAME} ${VERSION_STR}"
Icon res\img\icon.ico 
OutFile "${FILENAME}"
SetCompressor /SOLID lzma
SetCompressorDictSize 16

LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
VIProductVersion "${VERSION_NUM}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${CZ_ORG_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION_STR}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${CZ_COPY_INFO}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${CZ_NAME_SHORT}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION_STR}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Smart Container for ${CZ_NAME_SHORT}"

Function .onInit
	SetSilent silent
FunctionEnd

Section "-Go"
	GetTempFileName $OUTDIR
	Delete $OUTDIR
	CreateDirectory $OUTDIR
	SetOutPath $OUTDIR
	File "/oname=${CZ_CUDAZ_EXE}" "bin\${CZ_CUDAZ_EXE}"
	File "/oname=${CZ_CUDART_DLL}" "${CZ_CUDART_DLL_PATH}\${CZ_CUDART_DLL}"

	ExecWait '"$OUTDIR\${CZ_CUDAZ_EXE}"'

	Delete $OUTDIR\${CZ_CUDAZ_EXE}
	IfFileExists $OUTDIR\${CZ_CUDAZ_EXE} -1 0
	Delete $OUTDIR\${CZ_CUDART_DLL}
	IfFileExists $OUTDIR\${CZ_CUDART_DLL} -1 0
	SetOutPath $TEMP
	RMDir $OUTDIR
SectionEnd
