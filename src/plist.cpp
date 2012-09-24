/*!	\file plist.cpp
	\brief OSX propery list access source file.
	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html
*/

#include <QSettings>

#include "plist.h"
#include "log.h"

/*!	\brief Get property string function.
 	\returns
		- \a 0 in case of successful property read out;
		- \a 1 in case of access error.
*/
int CZPlistGet(
	char *fileName,			/*!<[in] Plist file name. */
	char *propertyName,		/*!<[in] Plist property name. */
	char *valueBuffer,		/*!<[out] Value output buffer. */
	int bufferSize			/*!<[in] Output buffer size. */
) {

	QSettings plist(fileName, QSettings::NativeFormat);

	if(plist.status() != QSettings::NoError) {
		CZLog(CZLogLevelModerate, "Plist open failed %s!", fileName);
		return 1;
	}

	QString value = plist.value(propertyName, QString()).toString();

	if(value.isNull()) {
		CZLog(CZLogLevelModerate, "Can't read property %s from plist %s!", propertyName, fileName);
		return 1;
	}

	QByteArray buffer = value.toUtf8();
	if(strlen(buffer.constData()) >= bufferSize) {
		CZLog(CZLogLevelModerate, "Buffer is too small for value %s!", buffer.constData());
		return 1;
	}

	strcpy(valueBuffer, buffer.constData());
	CZLog(CZLogLevelModerate, "Property %s from plist %s is: %s!", propertyName, fileName, buffer.constData());
	return 0;
}
