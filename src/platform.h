/*!	\file platform.h
	\brief OS/platform information data and function definition.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_PLATFORM_H
#define CZ_PLATFORM_H

#ifdef __cplusplus

#include <QString>
const QString getCompilerVersion();
const QString getOSVersion();
const QString getPlatformString();

extern "C" {
#endif

/*!	\def CZ_OS_OLD_PLATFORM_STR
	\brief Old platform ID string.
*/
#if defined(Q_OS_WIN)
#define CZ_OS_OLD_PLATFORM_STR	"win32"
#elif defined(Q_OS_MAC)
#define CZ_OS_OLD_PLATFORM_STR	"macosx"
#elif defined(Q_OS_LINUX)
#define CZ_OS_OLD_PLATFORM_STR	"linux"
#else
#error Your platform is not supported by CUDA! Or it does but I know nothing about this...
#endif

#ifdef __cplusplus
}
#endif

#endif//CZ_PLATFORM_H
