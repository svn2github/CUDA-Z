!system "sh ./make_build_svn.sh | sed -e $\"s,endif//,endif \;,$\" | tr \# ! > build.nsi"
!system "cat src/version.h | sed -e $\"s,\$\"build.h\$\",build.nsi,$\" -e $\"s,endif//,endif \;,$\" -e $\"s,^//,\;,$\" | tr \# ! > version.nsi"
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
!define VERSION_STR "${CZ_VER_STRING}.${CZ_VER_BUILD} ${CZ_VER_STATE}"
!define NAME "${CZ_NAME_SHORT} Container"

Name "${NAME} ${VERSION_STR}"
Icon res\img\icon.ico 
OutFile "${CZ_NAME_SHORT}-${VERSION_NUM}.exe"
SetCompressor /SOLID lzma
SetCompressorDictSize 8

LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
VIProductVersion "${VERSION_NUM}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${CZ_ORG_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION_STR}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${CZ_COPY_INFO}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${CZ_NAME_SHORT}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION_STR}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "It's a Smart Container for ${CZ_NAME_SHORT}"

Function .onInit
	SetSilent silent
FunctionEnd

Section "-Go"
	GetTempFileName $OUTDIR
	Delete $OUTDIR
	CreateDirectory $OUTDIR
	SetOutPath $OUTDIR
	File /oname=cuda-z.exe bin\cuda-z.exe
	File /oname=cudart.dll bin\cudart.dll
#	SearchPath $1 nvcuda.dll
#	IfFileExists $1 +2 0
#	File /oname=nvcuda.dll bin\nvcuda.dll

	ExecWait '"$OUTDIR\cuda-z.exe"'

	Delete $OUTDIR\cuda-z.exe
	IfFileExists $OUTDIR\cuda-z.exe -1 0
	Delete $OUTDIR\cudart.dll
	IfFileExists $OUTDIR\cudart.dll -1 0
#	Delete $OUTDIR\nvcuda.dll
#	IfFileExists $OUTDIR\nvcuda.dll -1 0
	SetOutPath $TEMP
	RMDir $OUTDIR
SectionEnd
