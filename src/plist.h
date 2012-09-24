/*!	\file plist.h
	\brief OSX propery list access definitions header.
	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html
*/

#ifndef CZ_PLIST_H
#define CZ_PLIST_H

#ifdef __cplusplus
extern "C" {
#endif

int CZPlistGet(char *fileName, char *propertyName, char *valueBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif//CZ_PLIST_H
