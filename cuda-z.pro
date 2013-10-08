#	\file cuda-z.pro
#	\brief CUDA-Z project file.
#	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html

TEMPLATE = app
QT = gui core network
CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug
#CONFIG += console
CONFIG += static

isEqual(QT_MAJOR_VERSION, 5) {
QT += widgets
}

#message(CONFIG: $$CONFIG)

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
build_h.commands = perl ./make_build_svn.pl $(EXPORT_BUILD_H)
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
	pkg-win32.commands = makensis.exe pkg-win32.nsi
	pkg-win32.depends = release
	QMAKE_EXTRA_TARGETS += pkg-win32
}

unix: {
	pkg.depends = pkg-linux
	pkg-linux.target = pkg-linux
	pkg-linux.commands = sh pkg-linux.sh
	pkg-linux.depends = all
	QMAKE_EXTRA_TARGETS += pkg-linux
}

mac: {
	pkg.depends = pkg-macosx
	pkg-macosx.target = pkg-macosx
	pkg-macosx.commands = sh pkg-macosx.sh
	pkg-macosx.depends = all
	QMAKE_EXTRA_TARGETS += pkg-macosx
}

# Outputs...

DESTDIR = bin
OBJECTS_DIR = bld/o
MOC_DIR = bld/moc
UI_DIR = bld/ui
RCC_DIR = bld/rcc

include(cuda.pri)
