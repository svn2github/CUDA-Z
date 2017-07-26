/*!	\file log.cpp
	\brief Logging source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <QString>

#include <stdarg.h>

#include "log.h"

#define CZ_LOG_BUFFER_LENGTH		4096	/*!< Log bufer size. */
#define CZ_LOG_DEFAULT_LEVEL		CZLogLevelHigh	/*!< Default logging level. */

/*!	\brief Logging function.
*/
void CZLog(
	CZLogLevel level,		/*!<[in] Log level value. */
	const char *fmt,		/*!<[in] printf()-like format string. */
	...				/* Additional arguments for printout. */
) {
	QString text;

#ifdef QT_NO_DEBUG
	if(level > CZ_LOG_DEFAULT_LEVEL) {
		return;
	}
#endif

	va_list ap;
	va_start(ap, fmt);
	if(fmt)
		text.vsprintf(fmt, ap);
	va_end(ap);

	CZLog(level, text);

}

/*!	\brief Logging function for QString.
*/
void CZLog(
	CZLogLevel level,		/*!<[in] Log level value. */
	QString text			/*!<[in] Qt string. */
) {

#ifdef QT_NO_DEBUG
	if(level > CZ_LOG_DEFAULT_LEVEL) {
		return;
	}
#endif

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

#if QT_VERSION < 0x050000
	qt_message_output(type, text.toLocal8Bit().constData());
#else
	QMessageLogContext context;
	qt_message_output(type, context, text.toLocal8Bit().constData());
#endif//QT_VERSION
}
