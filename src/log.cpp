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
#ifdef QT_NO_DEBUG
#define CZ_LOG_DEFAULT_LEVEL		CZLogLevelHigh	/*!< Default logging level. */
#else
#define CZ_LOG_DEFAULT_LEVEL		CZLogLevelLow	/*!< Default logging level. */
#endif

/*!	\brief Current allowed verbosity level.
*/
static int s_verbosityLevel = CZ_LOG_DEFAULT_LEVEL;

/*!	\brief Set current verbosity logging level.
	\returns previous verbosity level.
*/
int CZLogSetVerbosityLevel(
	int newVerbosityLevel		/*![in] New verbosity level. */
) {
	int oldVerbosityLevel = s_verbosityLevel;
	s_verbosityLevel = newVerbosityLevel;
	return oldVerbosityLevel;
}

/*!	\brief C-like logging function.
*/
void CZLog(
	CZLogLevel level,		/*!<[in] Log level value. */
	const char *fmt,		/*!<[in] printf()-like format string. */
	...				/* Additional arguments for printout. */
) {
	QString text;

	if(level > s_verbosityLevel) {
		return;
	}

	va_list ap;
	va_start(ap, fmt);
	if(fmt)
		text.vsprintf(fmt, ap);
	va_end(ap);

	CZLog(level, text);
}

/*!	\brief C++ logging function.
*/
void CZLog(
	CZLogLevel level,		/*!<[in] Log level value. */
	QString text			/*!<[in] Qt string. */
) {

	if(level > s_verbosityLevel) {
		return;
	}

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
