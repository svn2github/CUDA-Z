TEMPLATE = app
QT = gui core network
CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug
#CONFIG += console
CONFIG += static

#message(CONFIG: $$CONFIG)

FORMS = ui/czdialog.ui
HEADERS = src/version.h \
	src/czdialog.h \
	src/czdeviceinfo.h \
	src/log.h \
	src/cudainfo.h
mac:HEADERS += src/plist.h
SOURCES = src/czdialog.cpp \
	src/czdeviceinfo.cpp \
	src/log.cpp \
	src/main.cpp
mac:SOURCES += src/plist.cpp
CUSOURCES = src/cudainfo.cu
RESOURCES = res/cuda-z.qrc
win32:RC_FILE += res/cuda-z.rc
mac: {
	ICON = res/img/icon.icns
	TARGET = CUDA-Z
	QMAKE_INFO_PLIST = res/Info.plist
}

CUFLAGS = \
	-gencode=arch=compute_10,code=sm_10 \
	-gencode=arch=compute_10,code=compute_10 \
	-gencode=arch=compute_11,code=sm_11 \
	-gencode=arch=compute_11,code=compute_11 \
	-gencode=arch=compute_13,code=sm_13 \
	-gencode=arch=compute_13,code=compute_13 \
	-gencode=arch=compute_20,code=sm_20 \
	-gencode=arch=compute_20,code=compute_20 \

#	-Xcompiler "-nologo"

unix:LIBS += -lcudart
win32:LIBS += \
	$$quote($$(CUDA_LIB_PATH)\\cuda.lib) \
	$$quote($$(CUDA_LIB_PATH)\\cudart.lib) \
	Version.lib \
	Kernel32.lib \
	Psapi.lib

BUILD_H = src/build.h
QMAKE_EXTRA_VARIABLES += BUILD_H
PRE_TARGETDEPS += build_h

build_h.target = build_h
build_h.commands = sh ./make_build_svn.sh $(EXPORT_BUILD_H)
QMAKE_EXTRA_TARGETS += build_h

QCLEANFILES = \
	Makefile \
	Makefile.Debug \
	Makefile.Release \
	vc80.pdb \
	cuda-z.ncb \
	cudainfo.linkinfo \
	version.nsi \
	build.nsi
win32:QCLEANFILES += bin\\cuda-z.exe
unix:QCLEANFILES += bin/cuda-z
QMAKE_EXTRA_VARIABLES += QCLEANFILES

qclean.target = qclean
qclean.commands = -$(DEL_FILE) $(EXPORT_QCLEANFILES) #$(EXPORT_BUILD_H)
mac:qclean.depends = clean appclean
else:qclean.depends = clean
QMAKE_EXTRA_TARGETS += qclean

mac: {
	APPCLEANFILES += bin/CUDA-Z.app
	QMAKE_EXTRA_VARIABLES += APPCLEANFILES

	appclean.target = appclean
	appclean.commands = -$(DEL_FILE) -fR $(EXPORT_APPCLEANFILES)
	appclean.depends = clean
	QMAKE_EXTRA_TARGETS += appclean
}

win32: {
	pkg-win32.target = pkg-win32
	pkg-win32.commands = makensis.exe pkg-win32.nsi
	pkg-win32.depends = release
	QMAKE_EXTRA_TARGETS += pkg-win32
}

unix: {
	pkg-linux.target = pkg-linux
	pkg-linux.commands = sh pkg-linux.sh
	pkg-linux.depends = all
	QMAKE_EXTRA_TARGETS += pkg-linux
}

mac: {
	pkg-macosx.target = pkg-macosx
	pkg-macosx.commands = sh pkg-macosx.sh
	pkg-macosx.depends = all
	QMAKE_EXTRA_TARGETS += pkg-macosx
}

DESTDIR = bin
OBJECTS_DIR = bld/o
MOC_DIR = bld/moc
UI_DIR = bld/ui
RCC_DIR = bld/rcc

#
# Cuda extra-compiler for handling files specified in the CUSOURCES variable
#

{
	cu.name = Cuda ${QMAKE_FILE_IN}
	cu.input = CUSOURCES
	cu.CONFIG += no_link
	cu.variable_out = OBJECTS

	isEmpty(QMAKE_CUC) {
		win32:QMAKE_CUC = $$quote($$(CUDA_BIN_PATH)\\nvcc.exe)
		else:QMAKE_CUC = nvcc
	}
	isEmpty(CU_DIR):CU_DIR = .
	isEmpty(QMAKE_CPP_MOD_CU):QMAKE_CPP_MOD_CU = 
	isEmpty(QMAKE_EXT_CPP_CU):QMAKE_EXT_CPP_CU = .cu

	win32:INCLUDEPATH += $$quote($$(CUDA_INC_PATH))
	unix:INCLUDEPATH += /usr/local/cuda/include
	unix:LIBPATH += /usr/local/cuda/lib
	unix:LIBPATH += /usr/local/cuda/lib64

	QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS
	DebugBuild:QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_DEBUG
	ReleaseBuild:QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_RELEASE
	QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_RTTI_ON $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_STL_ON
#	message(QMAKE_CUFLAGS: $$QMAKE_CUFLAGS)

#	QMAKE_CUEXTRAFLAGS += -Xcompiler \"$$QMAKE_CUFLAGS\" $$CUFLAGS
	unix:QMAKE_CUEXTRAFLAGS += -Xcompiler $$join(QMAKE_CUFLAGS, ",") $$CUFLAGS
	win32:QMAKE_CUEXTRAFLAGS += $$CUFLAGS
	QMAKE_CUEXTRAFLAGS += $(DEFINES)
	win32:QMAKE_CUEXTRAFLAGS += $$join(QMAKE_COMPILER_DEFINES, " -D", -D)
	win32:QMAKE_CUEXTRAFLAGS += $(INCPATH)
	QMAKE_CUEXTRAFLAGS += -c
#	QMAKE_CUEXTRAFLAGS += -v
#	QMAKE_CUEXTRAFLAGS += -keep
#	QMAKE_CUEXTRAFLAGS += -clean
#	message(QMAKE_CUEXTRAFLAGS: $$QMAKE_CUEXTRAFLAGS)
	QMAKE_EXTRA_VARIABLES += QMAKE_CUEXTRAFLAGS

	cu.commands = $$QMAKE_CUC $(EXPORT_QMAKE_CUEXTRAFLAGS) -o $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ} ${QMAKE_FILE_NAME}$$escape_expand(\n\t)
	cu.output = $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ}
	silent:cu.commands = @echo nvcc ${QMAKE_FILE_IN} && $$cu.commands
	QMAKE_EXTRA_COMPILERS += cu

	build_pass|isEmpty(BUILDS):cuclean.depends = compiler_cu_clean
	else:cuclean.CONFIG += recursive
	QMAKE_EXTRA_TARGETS += cuclean
}
