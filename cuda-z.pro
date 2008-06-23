TEMPLATE = app
QT = gui core
CONFIG += qt release warn_on
#CONFIG += console

FORMS = ui/czdialog.ui
HEADERS = \
	src/version.h \
	src/czdialog.h \
	src/cudainfo.h
SOURCES = src/czdialog.cpp \
	src/main.cpp
RESOURCES = res/cuda-z.qrc
win32:RC_FILE += res/cuda-z.rc
CUSOURCES = src/cudainfo.cu

unix:LIBS += -lcuda
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
	Makefile.Release
win32:QCLEANFILES += bin\cuda-z.exe
QMAKE_EXTRA_VARIABLES += QCLEANFILES

qclean.target = qclean
qclean.commands = -$(DEL_FILE) $(EXPORT_QCLEANFILES) #$(EXPORT_BUILD_H)
qclean.depends = clean
QMAKE_EXTRA_TARGETS += qclean

DESTDIR = bin
OBJECTS_DIR = bld/o
MOC_DIR = bld/moc
UI_DIR = bld/ui
RCC_DIR = bld/rcc

#
# Cuda extra-compiler for handling files specified in the CUSOURCES variable
#

QMAKE_CUC = $(CUDA_BIN_PATH)/nvcc.exe

{
    cu.name = Cuda ${QMAKE_FILE_IN}
    cu.input = CUSOURCES
    cu.CONFIG += no_link
    cu.variable_out = OBJECTS

    isEmpty(QMAKE_CUC) {
        win32:QMAKE_CUC = nvcc.exe
        else:QMAKE_CUC = nvcc
    }
    isEmpty(CU_DIR):CU_DIR = .
    isEmpty(QMAKE_CPP_MOD_CU):QMAKE_CPP_MOD_CU = 
    isEmpty(QMAKE_EXT_CPP_CU):QMAKE_EXT_CPP_CU = .cu

    QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE $$QMAKE_CXXFLAGS_RTTI_ON $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_STL_ON
    QMAKE_CUEXTRAFLAGS += -Xcompiler $$join(QMAKE_CUFLAGS, ",")
    QMAKE_CUEXTRAFLAGS += $(DEFINES) $(INCPATH) $$join(QMAKE_COMPILER_DEFINES, " -D", -D)
    QMAKE_CUEXTRAFLAGS += -c

    cu.commands = $$QMAKE_CUC $$QMAKE_CUEXTRAFLAGS -o $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ} ${QMAKE_FILE_NAME}$$escape_expand(\n\t)
    cu.output = $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ}
    silent:cu.commands = @echo nvcc ${QMAKE_FILE_IN} && $$cu.commands
    QMAKE_EXTRA_COMPILERS += cu

    build_pass|isEmpty(BUILDS):cuclean.depends = compiler_cu_clean
    else:cuclean.CONFIG += recursive
    QMAKE_EXTRA_TARGETS += cuclean
}
