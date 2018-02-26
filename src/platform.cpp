/*!	\file platform.cpp
	\brief OS/platform information data and function implementation.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "platform.h"

/*!	\brief Get C/C++ compiler name string
	\hint This code is taken from Qt Creator code
	creator-3.3.0/src/plugins/coreplugin/icore.cpp
	\returns C/C++ compiler string
*/
const QString getCompilerVersion() {
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
	QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
	isAppleString = QLatin1String(" (Apple)");
#endif
	return QLatin1String("Clang ") + QString::number(__clang_major__) + QLatin1Char('.')
		+ QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
	return QLatin1String("GCC ") + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
	if(_MSC_VER >= 1800) // 1800: MSVC 2013 (yearly release cycle)
		return QLatin1String("MSVC ") + QString::number(2008 + ((_MSC_VER / 100) - 13));
	if(_MSC_VER >= 1500) // 1500: MSVC 2008, 1600: MSVC 2010, ... (2-year release cycle)
		return QLatin1String("MSVC ") + QString::number(2008 + 2 * ((_MSC_VER / 100) - 15));
#endif
	return QLatin1String("<unknown compiler>");
}

/*!	\fn getOSVersion
	\brief Get OS version string.
	\returns string that describes version of OS we running at.
*/

/*!	\fn getPlatformString
	\brief Get platform ID string.
	\returns string that describes platform we running at.
*/
#if defined(Q_OS_WIN)
#include <windows.h>
typedef BOOL (WINAPI *IsWow64Process_t)(HANDLE, PBOOL);

const QString getOSVersion() {
	QString OSVersion = "Windows";

	BOOL is_os64bit = FALSE;
	IsWow64Process_t p_IsWow64Process = (IsWow64Process_t)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if(p_IsWow64Process != NULL) {
		if(!p_IsWow64Process(GetCurrentProcess(), &is_os64bit)) {
			is_os64bit = FALSE;
	        }
	}

	OSVersion += QString(" %1").arg(
		(is_os64bit == TRUE)? "AMD64": "x86");

/*	GetSystemInfo(&systemInfo);
	OSVersion += QString(" %1").arg(
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)? "AMD64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)? "IA64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)? "x86":
		"Unknown architecture");*/

	OSVERSIONINFO versionInfo;
	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);
	OSVersion += QString(" %1.%2.%3 %4")
		.arg(versionInfo.dwMajorVersion)
		.arg(versionInfo.dwMinorVersion)
		.arg(versionInfo.dwBuildNumber)
		.arg(QString::fromWCharArray(versionInfo.szCSDVersion));

	return OSVersion;
}

const QString getPlatformString() {
	QString platformString = "win32";

	if(QSysInfo::WordSize == 64)
		platformString = "win64";

	return platformString;
}

#elif defined (Q_OS_LINUX)
#include <QProcess>
const QString getOSVersion() {
	QProcess uname; 

	uname.start("uname", QStringList() << "-srvm");
	if(!uname.waitForFinished())
		return QString("Linux (unknown)");
	QString OSVersion = uname.readLine();

	return OSVersion.remove('\n');
}

const QString getPlatformString() {
	QString platformString = "linux";

	if(QSysInfo::WordSize == 64)
		platformString = "linux64";

	return platformString;
}

#elif defined (Q_OS_MAC)
#include "plist.h"
const QString getOSVersion() {
	char osName[256];
	char osVersion[256];
	char osBuild[256];

	if((CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductName", osName, sizeof(osName)) != 0) ||
	   (CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductUserVisibleVersion", osVersion, sizeof(osVersion)) != 0) ||
	   (CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductBuildVersion", osBuild, sizeof(osBuild)) != 0))
	   return QString("Mac OS X (unknown)");
	QString OSVersion = QString(osName) + " " + QString(osVersion) + " " + QString(osBuild);
	
	return OSVersion.remove('\n');
}

const QString getPlatformString() {
	QString platformString = "macosx";

	if(QSysInfo::WordSize == 64)
		platformString = "macosx64";

	return platformString;
}

#else
#error Functions getOSVersion() and getPlatformString() are not implemented for your platform!
#endif//Q_OS_WIN
