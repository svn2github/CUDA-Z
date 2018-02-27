/*!	\file czcommandline.h
	\brief Command line interface implementation header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_COMMANDLINE_H
#define CZ_COMMANDLINE_H

#include <QString>

#define CZ_EXPORT_FILENAME_LEN

struct CZCommandLineFlags {
	bool needHelp;
	int devIndex;
	bool exportHTML;
	QString fileNameHTML;
	bool exportTXT;
	QString fileNameTXT;
};

bool CZParseCommandLine(int argc, char *argv[], struct CZCommandLineFlags *flags);

void CZPrintCommandLineHelp(void);

#endif//CZ_COMMANDLINE_H
