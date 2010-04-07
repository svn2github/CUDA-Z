/*!
	\file log.cpp
	\brief Logging source file.
	\author AG
*/

#include <QString>

#include <stdarg.h>

#include "log.h"

#define CZ_LOG_BUFFER_LENGTH		4096	/*!< Log bufer size. */

/*
	\brief Logging function.
*/
void CZLog(
	CZLogLevel level,		/*!<[in] Log level value. */
	char *fmt,			/*!<[in] printf()-like format string. */
	...				/* Additional arguments for printout. */
) {
	QString buf;

#ifdef QT_NO_DEBUG
	if(level <= CZLogLevelHigh) {
		return;
	}
#endif

	va_list ap;
	va_start(ap, fmt);
	if(fmt)
		buf.vsprintf(fmt, ap);
	va_end(ap);

	QtMsgType type;
	switch(level) {
	case CZLogLevelFatal:
		type = QtFatalMsg;
		break;
	case CZLogLevelError:
		type = QtCriticalMsg;
		break;
	case CZLogLevelWarning:
		type = QtWarningMsg;
		break;
	default:
		type = QtDebugMsg;
		break;
	}

	qt_message_output(type, buf.toLocal8Bit().constData());
}
