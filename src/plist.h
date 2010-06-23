/*!
	\file plist.h
	\brief OSX propery list access definitions header.
	\author AG
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
