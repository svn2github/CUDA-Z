/*!	\file czcommandline.cpp
	\brief Command line interface implementation source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "log.h"
#include "czdialog.h"
#include "czdeviceinfodecoder.h"
#include "platform.h"
#include "version.h"

/*!	\brief This function parses command line arguments and gives back a ser of flags.
	\returns \a true if success, or \a false in case of failure
*/
bool CZParseCommandLine(
	int argc,		/*!<[in] Count of command line arguments. */
	char *argv[],		/*!<[in] List of command line arguments. */
	struct CZCommandLineFlags *flags	/*!<[in,out] Output of parsed command line. */
) {
}

/*!	\brief This function print a command line help message
*/
void CZPrintCommandLineHelp(
	void
) {

	QString version = QString("%1 %2 %3 bit").arg(QObject::tr("Version")).arg(CZ_VERSION).arg(QString::number(QSysInfo::WordSize));
#ifdef CZ_VER_STATE
	version += QString("%1 %2 %3").arg(QObject::tr("Built")).arg(CZ_DATE).arg(CZ_TIME);
	version += QObject::tr("\n%1 %2 %3").arg(QObject::tr("Based on Qt")).arg(QT_VERSION_STR).arg(getCompilerVersion());
#ifdef CZ_VER_BUILD_URL
	version += QObject::tr("\n%1 %2:%3").arg(QObject::tr("SVN URL")).arg(CZ_VER_BUILD_URL).arg(CZ_VER_BUILD_STRING);
#endif//CZ_VER_BUILD_URL
#endif//CZ_VER_STATE

	CZLog(CZLogLevelHigh, QString("%1 - %2")
		.arg(CZ_NAME_SHORT)
		.arg(CZ_NAME_LONG));

	CZLog(CZLogLevelHigh, version);
	CZLog(CZLogLevelHigh, QString("%1: %2\n%3: %4")
		.arg(QObject::tr("Main page")).arg(CZ_ORG_URL_MAINPAGE)
		.arg(QObject::tr("Project page")).arg(CZ_ORG_URL_PROJECT));
	CZLog(CZLogLevelHigh, QString("%1 %2").arg(QObject::tr("Author")).arg(CZ_ORG_NAME));
	CZLog(CZLogLevelHigh, QString("%1 %2").arg(CZ_COPY_INFO).arg(CZ_COPY_URL));

}
