/*!	\file log.h
	\brief Logging definitions header.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_LOG_H
#define CZ_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/*!	\brief Logging level definition.
*/
typedef enum {
	CZLogLevelFatal = -3,		/*!< Fatal error. Causes termination of application. */
	CZLogLevelError = -2,		/*!< Error. */
	CZLogLevelWarning = -1,		/*!< Warning. */
	CZLogLevelHigh = 0,		/*!< Important information. */
	CZLogLevelModerate = 1,		/*!< Moderate information. */
	CZLogLevelLow = 2,		/*!< Not important information. */
} CZLogLevel;

void CZLog(CZLogLevel level, const char *fmt, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
	__attribute__ ((format (printf, 2, 3)))
#endif
;

#ifdef __cplusplus
}
#endif

#endif//CZ_LOG_H
