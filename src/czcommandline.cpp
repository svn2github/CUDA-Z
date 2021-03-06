/*!	\file czcommandline.cpp
	\brief Command line interface implementation source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>

#include "log.h"
#include "cudainfo.h"
#include "czdeviceinfodecoder.h"
#include "platform.h"
#include "version.h"
#include "czcommandline.h"

/*!	\class CZCommandLine
	\brief This class handles command line interface of CUDA-Z.
*/

/*!	\brief Creates the command line interface.
*/
CZCommandLine::CZCommandLine(
	int argc,			/*!<[in] Count of command line arguments. */
	char **argv,			/*!<[in] List of command line arguments. */
	QObject *parent			/*!<[in,out] Parent of CUDA device information. */
)	: QObject(parent) {

	m_argc = argc;
	m_argv = argv;

	m_needHelp = false;
	m_needVersion = false;
	m_printVerbose = false;
	m_listDevices = false;
	m_devIndex = 0;
	m_printToConsole = false;
	m_exportHTML = false;
	m_exportTXT = false;
}

/*!	\brief Terminates the command line interface.
*/
CZCommandLine::~CZCommandLine() {
}

/*!	\brief This function parses command line arguments and gives back a ser of flags.
	\returns \a true if success, or \a false in case of failure
*/
bool CZCommandLine::parse() {

	for(int i = 1; i < m_argc; i++) {

		CZLog(CZLogLevelLow, tr("Processing option '%1' ...").arg(m_argv[i]));

		if(QString(m_argv[i]) == "-help") {
			m_needHelp = true;
		} else if(QString(m_argv[i]) == "-version") {
			m_needVersion = true;
		} else if(QString(m_argv[i]) == "-cli") {
			/* do nothing here! */
		} else if(QString(m_argv[i]) == "-verbose") {
			m_printVerbose = true;
		} else if(QString(m_argv[i]) == "-list") {
			m_listDevices = true;
		} else if(QString(m_argv[i]) == "-dev") {
			if(++i < m_argc) {
				bool intOk;
				m_devIndex = QString(m_argv[i]).toInt(&intOk);
				if(!intOk) {
					CZLog(CZLogLevelError, tr("Wrong usage of option '-dev <n>'!"));
					return false;
				}
				CZLog(CZLogLevelLow, tr("Device index: %1").arg(m_devIndex));
			} else {
				CZLog(CZLogLevelError, tr("Wrong usage of option '-dev <n>'!"));
				return false;
			}
		} else if(QString(m_argv[i]) == "-print") {
			m_printToConsole = true;
		} else if(QString(m_argv[i]) == "-html") {
			if(++i < m_argc) {
				m_exportHTML = true;
				m_fileNameHTML = m_argv[i];
				CZLog(CZLogLevelLow, tr("HTML file name: %1").arg(m_fileNameHTML));
			} else {
				CZLog(CZLogLevelError, tr("Wrong usage of option '-html <file>'!"));
				return false;
			}
		} else if(QString(m_argv[i]) == "-txt") {
			if(++i < m_argc) {
				m_exportTXT = true;
				m_fileNameTXT = m_argv[i];
				CZLog(CZLogLevelLow, tr("TXT file name: %1").arg(m_fileNameTXT));
			} else {
				CZLog(CZLogLevelError, tr("Wrong usage of option '-txt <file>'!"));
				return false;
			}
		} else {
			CZLog(CZLogLevelError, tr("Wrong option '%1'!").arg(m_argv[i]));
			return false;
		}
	}

	return true;
}

/*!	\brief This function execures command line interface of CUDA-Z.
	\returns \a 0 in case of success, \a other in case of failure
*/
int CZCommandLine::exec() {

	if(!parse()) {
		CZLog(CZLogLevelHigh, tr("Run '%1 -cli -help' for more information").arg(CZ_NAME_SHORT));
		return 1;
	}

	if(m_needHelp) {
		printCommandLineHelp();
		return 0;
	}

	if(m_needVersion) {
		printUtilityVersion();
		return 0;
	}

	if(m_listDevices) {
		printDeviceList();
		return 0;
	}

	if(m_devIndex >= CZCudaDeviceFound()) {
		CZLog(CZLogLevelError, tr("Wrong CUDA device index!"));
		CZLog(CZLogLevelHigh, tr("Run '%1 -cli -list' for more information").arg(CZ_NAME_SHORT));
		return 1;
	}

	struct CZDeviceInfo info;
	memset(&info, 0, sizeof(info));
	info.num = m_devIndex;
	info.heavyMode = 0;

	CZLog(CZLogLevelLow, tr("Getting information about %1 ...").arg(info.num));
	if(CZCudaReadDeviceInfo(&info, info.num) != 0) {
		CZLog(CZLogLevelError, tr("Can't get information about device %1!").arg(info.num));
		return 1;
	}

	CZLog(CZLogLevelLow, tr("Preparing device %1 ...").arg(info.num));
	if(CZCudaPrepareDevice(&info) != 0) {
		CZLog(CZLogLevelError, tr("Can't prepare device %1!").arg(info.num));
		return 1;
	}

	for(int i = 0; i < 2; i++) { /* repeat tests twice for better precision */
		int r = CZCudaCalcDeviceBandwidth(&info);
		if(r != -1)
			r = CZCudaCalcDevicePerformance(&info);

		if(r != 0) {
			CZLog(CZLogLevelError, tr("Can't perform tests on device %1!").arg(info.num));
		}
	}

	CZCudaDeviceInfoDecoder decoder(info);

	if(m_exportHTML) {
		QString fileName = m_fileNameHTML;
		QFile file(fileName);
		if(!file.open(QFile::WriteOnly | QFile::Text)) {
			CZLog(CZLogLevelError,
				tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
			return 1;
		}

		QTextStream stream(&file);
		stream << decoder.generateHTMLReport();
	}

	if(m_exportTXT) {
		QString fileName = m_fileNameTXT;
		QFile file(fileName);
		if(!file.open(QFile::WriteOnly | QFile::Text)) {
			CZLog(CZLogLevelError,
				tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
			return 1;
		}

		QTextStream stream(&file);
		stream << decoder.generateTextReport();
	}

	if(!m_exportHTML && !m_exportTXT) {
		m_printToConsole = true;
	}

	if(m_printToConsole) {
		QTextStream stream(stdout);
		stream << decoder.generateTextReport();
		return 0;
	}

	CZCudaCleanDevice(&info);

	return 0;
}

/*!	\brief This function returns utility title information
	\returns title information string
*/
const QString CZCommandLine::getTitleString() {
	QString title;

	title += QString("%1 - %2\n").arg(CZ_NAME_SHORT).arg(CZ_NAME_LONG);

	return title;
}

/*!	\brief This function collects version information about CUDA-Z utility
	\returns version information string
*/
const QString CZCommandLine::getVersionString() {
	QString info;

	info += QString("%1 %2 %3 bit\n").arg(tr("Version")).arg(CZ_VERSION).arg(QString::number(QSysInfo::WordSize));
#ifdef CZ_VER_STATE
	info += QString("%1 %2 %3\n").arg(tr("Built")).arg(CZ_DATE).arg(CZ_TIME);
	info += QString("%1 %2 %3\n").arg(tr("Based on Qt")).arg(QT_VERSION_STR).arg(getCompilerVersion());
#ifdef CZ_VER_BUILD_URL
	info += QString("%1: %2:%3\n").arg(tr("SVN URL")).arg(CZ_VER_BUILD_URL).arg(CZ_VER_BUILD_STRING);
#endif//CZ_VER_BUILD_URL
#endif//CZ_VER_STATE

	info += QString("%1: %2\n").arg(tr("Main page")).arg(CZ_ORG_URL_MAINPAGE);
	info += QString("%1: %2\n").arg(tr("Project page")).arg(CZ_ORG_URL_PROJECT);
	info += QString("%1: %2\n").arg(tr("Facebook page")).arg(CZ_ORG_URL_FACEBOOK);
	info += QString("%1: %2\n").arg(tr("Author")).arg(CZ_ORG_NAME);
	info += QString("%1 %2\n").arg(CZ_COPY_INFO).arg(CZ_COPY_URL);

	return info;
}

/*!	\brief This function collects help information about CUDA-Z utility
	\returns help information string
*/
const QString CZCommandLine::getHelpString() {
	QString help;

	help += QString("%1:\n").arg(tr("Usage"));
	help += QString("\t%1 (%2)\n").arg(CZ_NAME_SHORT).arg(tr("Start as GUI app"));
	help += QString("\t%1 -cli <%2> (%3)\n").arg(CZ_NAME_SHORT).arg(tr("Options")).arg(tr("Start as command line utility"));

	help += QString("%1:\n").arg(tr("Options"));
	help += QString("\t-help         %1\n").arg(tr("Print this help message"));
	help += QString("\t-version      %1\n").arg(tr("Print version information"));
	help += QString("\t-cli          %1\n").arg(tr("Activate command line interface"));
	help += QString("\t-verbose      %1\n").arg(tr("Print more status information"));
	help += QString("\t-list         %1\n").arg(tr("Print list of available CUDA devices"));
	help += QString("\t-dev <n>      %1\n").arg(tr("Print/export CUDA information about device <n>"));
	help += QString("\t-print        %1\n").arg(tr("Print CUDA information to a console (default)"));
	help += QString("\t-html <file>  %1\n").arg(tr("Export CUDA information to a <file> as HTML"));
	help += QString("\t-txt <file>   %1\n").arg(tr("Export CUDA information to a <file> as TXT"));

	return help;
}

/*!	\brief This function prints a command line help message
*/
void CZCommandLine::printCommandLineHelp() {
	QTextStream stream(stdout);
	stream << getTitleString() + getHelpString();
}

/*!	\brief This function prints utility version message
*/
void CZCommandLine::printUtilityVersion() {
	QTextStream stream(stdout);
	stream << getTitleString() + getVersionString();
}

/*!	\brief This function prints list of CUDA-enabled devices
*/
void CZCommandLine::printDeviceList() {
	int num = CZCudaDeviceFound();
	struct CZDeviceInfo info;

	QTextStream stream(stdout);
	stream << tr("Available device(s): %1").arg(num) << endl;
	for(int i = 0; i < num; i++) {
		memset(&info, 0, sizeof(info));
		info.num = i;
		info.heavyMode = 0;

		CZLog(CZLogLevelLow, tr("Getting information about %1 ...").arg(info.num));
		if(CZCudaReadDeviceInfo(&info, info.num) == 0) {
			stream << QString("\t%1: %2").arg(info.num).arg(info.deviceName) << endl;
		}
	}
}
