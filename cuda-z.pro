TEMPLATE = app
QT = gui core network
CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug
#CONFIG += console
CONFIG += static

FORMS = ui/czdialog.ui
HEADERS = \
	src/version.h \
	src/czdialog.h \
	src/czdeviceinfo.h \
	src/cudainfo.h
SOURCES = src/czdialog.cpp \
	src/czdeviceinfo.cpp \
	src/main.cpp
RESOURCES = res/cuda-z.qrc
win32:RC_FILE += res/cuda-z.rc
CUSOURCES = src/cudainfo.cu

unix:LIBS += -lcudart
win32:LIBS += \
	$(CUDA_LIB_PATH)\cuda.lib \
	$(CUDA_LIB_PATH)\cudart.lib

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
	version.nsi \
	build.nsi
win32:QCLEANFILES += bin\cuda-z.exe
QMAKE_EXTRA_VARIABLES += QCLEANFILES

qclean.target = qclean
qclean.commands = -$(DEL_FILE) $(EXPORT_QCLEANFILES) #$(EXPORT_BUILD_H)
qclean.depends = clean
QMAKE_EXTRA_TARGETS += qclean

pkg-win32.target = pkg-win32
pkg-win32.commands = makensis.exe cuda-z.nsi
pkg-win32.depends = release
QMAKE_EXTRA_TARGETS += pkg-win32

DESTDIR = bin
OBJECTS_DIR = bld/o
MOC_DIR = bld/moc
UI_DIR = bld/ui
RCC_DIR = bld/rcc

#
# Cuda extra-compiler for handling files specified in the CUSOURCES variable
#

win32:QMAKE_CUC = $(CUDA_BIN_PATH)/nvcc.exe
unix:QMAKE_CUC = nvcc

{
	cu.name = Cuda ${QMAKE_FILE_IN}
	cu.input = CUSOURCES
	cu.CONFIG += no_link
	cu.variable_out = OBJECTS

	isEmpty(QMAKE_CUC) {
		win32:QMAKE_CUC = $(CUDA_BIN_PATH)/nvcc.exe
		else:QMAKE_CUC = nvcc
	}
	isEmpty(CU_DIR):CU_DIR = .
	isEmpty(QMAKE_CPP_MOD_CU):QMAKE_CPP_MOD_CU = 
	isEmpty(QMAKE_EXT_CPP_CU):QMAKE_EXT_CPP_CU = .cu

	win32:INCLUDEPATH += $(CUDA_INC_PATH)
	unix:INCLUDEPATH += /usr/local/cuda/include
	unix:LIBPATH += /usr/local/cuda/lib

	QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS
	CONFIG(debug, debug|release) {
		QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_DEBUG
	} else {
		QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_RELEASE
	}
	QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS_RTTI_ON $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_STL_ON

	QMAKE_CUEXTRAFLAGS += -Xcompiler $$join(QMAKE_CUFLAGS, ",")
	QMAKE_CUEXTRAFLAGS += $(DEFINES) $(INCPATH) $$join(QMAKE_COMPILER_DEFINES, " -D", -D)
	QMAKE_CUEXTRAFLAGS += -c
#	QMAKE_CUEXTRAFLAGS += --keep

	cu.commands = $$QMAKE_CUC $$QMAKE_CUEXTRAFLAGS -o $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ} ${QMAKE_FILE_NAME}$$escape_expand(\n\t)
	cu.output = $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ}
	silent:cu.commands = @echo nvcc ${QMAKE_FILE_IN} && $$cu.commands
	QMAKE_EXTRA_COMPILERS += cu

	build_pass|isEmpty(BUILDS):cuclean.depends = compiler_cu_clean
	else:cuclean.CONFIG += recursive
	QMAKE_EXTRA_TARGETS += cuclean
}
