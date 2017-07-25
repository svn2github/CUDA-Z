#	\file cuda-z.pro
#	\brief CUDA-Z project file.
#	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

TEMPLATE = app
QT = gui core network
CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug
CONFIG += console
#CONFIG += static
#CONFIG += sm_all

isEqual(QT_MAJOR_VERSION, 5) {
	QT += widgets
}

#message(CONFIG: $$CONFIG)

CZ_SOURCE_DIR = $$PWD
CZ_BUILD_DIR = $$OUT_PWD
CZ_BUILD_SRC_DIR = $$CZ_BUILD_DIR/src

# Compiling...

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

SM_CONFIG = $$find(CONFIG, sm_.*)
isEmpty(SM_CONFIG): CONFIG += sm_all

sm_all:CONFIG += sm_30 sm_32 sm_35 sm_37 sm_50 sm_52 sm_53 sm_60 sm_61 sm_62

sm_10:CUFLAGS += -gencode=arch=compute_10,code=sm_10 \
	-gencode=arch=compute_10,code=compute_10

sm_11:CUFLAGS += -gencode=arch=compute_11,code=sm_11 \
	-gencode=arch=compute_11,code=compute_11

sm_13:CUFLAGS += -gencode=arch=compute_13,code=sm_13 \
	-gencode=arch=compute_13,code=compute_13

sm_20:CUFLAGS += -gencode=arch=compute_20,code=sm_20 \
	-gencode=arch=compute_20,code=compute_20

sm_30:CUFLAGS += -gencode=arch=compute_30,code=sm_30 \
	-gencode=arch=compute_30,code=compute_30

sm_32:CUFLAGS += -gencode arch=compute_32,code=sm_32 \
	-gencode arch=compute_32,code=compute_32

sm_35:CUFLAGS += -gencode=arch=compute_35,code=sm_35 \
	-gencode=arch=compute_35,code=compute_35

sm_37:CUFLAGS += -gencode=arch=compute_37,code=sm_37 \
	-gencode=arch=compute_37,code=compute_37

sm_50:CUFLAGS += -gencode arch=compute_50,code=sm_50 \
	-gencode arch=compute_50,code=compute_50

sm_52:CUFLAGS += -gencode arch=compute_52,code=sm_52 \
	-gencode arch=compute_52,code=compute_52

sm_53:CUFLAGS += -gencode arch=compute_53,code=sm_53 \
	-gencode arch=compute_53,code=compute_53

sm_60:CUFLAGS += -gencode arch=compute_60,code=sm_60 \
	-gencode arch=compute_60,code=compute_60

sm_61:CUFLAGS += -gencode arch=compute_61,code=sm_61 \
	-gencode arch=compute_61,code=compute_61

sm_62:CUFLAGS += -gencode arch=compute_62,code=sm_62 \
	-gencode arch=compute_62,code=compute_62

win32:INCLUDEPATH += $$quote($$replace(CZ_BUILD_SRC_DIR, /, \\))
else:INCLUDEPATH += $$CZ_BUILD_SRC_DIR

win32:RC_INCLUDEPATH += $$quote($$replace(CZ_SOURCE_DIR, /, \\))
win32:RC_INCLUDEPATH += $$quote($$replace(CZ_SOURCE_DIR, /, \\)\\src)
win32:RC_INCLUDEPATH += $$quote($$replace(CZ_BUILD_SRC_DIR, /, \\))

win32:isEqual(QT_MAJOR_VERSION, 4) {
	QMAKE_RC = rc $$join(RC_INCLUDEPATH, " -I ", "-I ")
}

unix:LIBS += -lcudart_static
linux:!static:LIBS += -ldl -lm -lrt
win32:LIBS += \
	$$quote($$(CUDA_LIB_PATH)\\cuda.lib) \
	$$quote($$(CUDA_LIB_PATH)\\cudart_static.lib) \
	Version.lib \
	Kernel32.lib \
	Psapi.lib

make_src_dir.target = $$CZ_BUILD_SRC_DIR
win32:make_src_dir.commands = $(MKDIR) $$replace(CZ_BUILD_SRC_DIR, /, \\)
else:make_src_dir.commands = $(MKDIR) $$CZ_BUILD_SRC_DIR
QMAKE_EXTRA_TARGETS += make_src_dir

BUILD_H = $$CZ_BUILD_SRC_DIR/build.h
QMAKE_EXTRA_VARIABLES += BUILD_H
PRE_TARGETDEPS += build_h

build_h.target = build_h
build_h.commands = perl $$CZ_SOURCE_DIR/bld/bin/make_build_svn.pl $(EXPORT_BUILD_H) $$CZ_SOURCE_DIR
build_h.depends = make_src_dir
QMAKE_EXTRA_TARGETS += build_h

# Cleaning...

QCLEANFILES = \
	Makefile \
	*.fatbin \
	*.fatbin.c \
	*.hash \
	*.ptx \
	*.cpp.ii \
	*.cpp?.ii \
	*.cpp?.i \
	*.cudafe?.c \
	*.cudafe?.cpp \
	*.cudafe?.gpu \
	*.cudafe?.stub.c \
	*.cubin
win32:QCLEANFILES += \
	Makefile.Debug \
	Makefile.Release \
	vc?0.pdb \
	vc1?0.pdb \
	bin\\cuda-z.exe \
	bin\\cuda-z.pdb \
	cuda-z.ncb \
	cudainfo.linkinfo \
	version.nsi \
	build.nsi
unix:!mac:QCLEANFILES += \
	bin/cuda-z

QMAKE_EXTRA_VARIABLES += QCLEANFILES

qclean.target = qclean
qclean.commands = -$(DEL_FILE) $(EXPORT_QCLEANFILES) #$(EXPORT_BUILD_H)
qclean.depends = clean
QMAKE_EXTRA_TARGETS += qclean

mac: {
	QCLEANDIRS += bin/CUDA-Z.app
	QMAKE_EXTRA_VARIABLES += QCLEANDIRS
	qclean.commands += && $(DEL_FILE) -fR $(EXPORT_QCLEANDIRS)
}

# Packaging...

pkg.target = pkg
QMAKE_EXTRA_TARGETS += pkg

win32: {
	pkg.depends = pkg-win32
	pkg-win32.target = pkg-win32
	pkg-win32.commands = cmd /C $$CZ_SOURCE_DIR\\bld\\bin\\pkg-win32.cmd $$quote($$replace(CZ_SOURCE_DIR, /, \\)) $$quote($$replace(CZ_BUILD_DIR, /, \\)) $(EXTRA_SUFFIX)
	pkg-win32.depends = release
	QMAKE_EXTRA_TARGETS += pkg-win32
}

unix: {
	pkg.depends = pkg-linux
	pkg-linux.target = pkg-linux
	pkg-linux.commands = sh $$CZ_SOURCE_DIR/bld/bin/pkg-linux.sh $$CZ_SOURCE_DIR $$CZ_BUILD_DIR $(EXTRA_SUFFIX)
	pkg-linux.depends = all
	QMAKE_EXTRA_TARGETS += pkg-linux
}

mac: {
	pkg.depends = pkg-macosx
	pkg-macosx.target = pkg-macosx
	pkg-macosx.commands = sh $$CZ_SOURCE_DIR/bld/bin/pkg-macosx.sh $$CZ_SOURCE_DIR $$CZ_BUILD_DIR $(EXTRA_SUFFIX)
	pkg-macosx.depends = all
	QMAKE_EXTRA_TARGETS += pkg-macosx
}

# Outputs...

DESTDIR = $$CZ_BUILD_DIR/bin
OBJECTS_DIR = $$CZ_BUILD_DIR/bld/o
MOC_DIR = $$CZ_BUILD_DIR/bld/moc
UI_DIR = $$CZ_BUILD_DIR/bld/ui
RCC_DIR = $$CZ_BUILD_DIR/bld/rcc

include(cuda.pri)
