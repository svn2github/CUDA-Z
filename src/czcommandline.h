/*!	\file czcommandline.h
	\brief Command line interface implementation header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_COMMANDLINE_H
#define CZ_COMMANDLINE_H

#include <QObject>
#include <QString>

class CZCommandLine: public QObject {
	Q_OBJECT

public:
	CZCommandLine(int argc, char **argv, QObject *parent = 0);
	~CZCommandLine();

	int exec();
	bool parse();

	static const QString getTitleString();
	static const QString getVersionString();
	static const QString getHelpString();

	static void printCommandLineHelp();
	static void printUtilityVersion();
	static void printDeviceList();

private:
	int m_argc;
	char **m_argv;

	bool m_needHelp;
	bool m_needVersion;
	bool m_printVerbose;
	bool m_listDevices;
	int m_devIndex;
	bool m_printToConsole;
	bool m_exportHTML;
	QString m_fileNameHTML;
	bool m_exportTXT;
	QString m_fileNameTXT;
};

#endif//CZ_COMMANDLINE_H
