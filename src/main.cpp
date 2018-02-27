/*!	\file main.cpp
	\brief Main source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "log.h"
#include "czdialog.h"
#include "cudainfo.h"
#include "version.h"
#include "czcommandline.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*!	\brief Call function that checks CUDA presents.
*/
bool testCudaPresent() {
	bool res = CZCudaCheck();
	CZLog(CZLogLevelHigh, "CUDA Present: %d", res);
	return res;
}

/*!	\brief Call function that returns numbed of CUDA-devices.
*/
int getCudaDeviceNum() {
	int res = CZCudaDeviceFound();
	CZLog(CZLogLevelHigh, "CUDA Devices found: %d", res);

	if(res == 1) { // Check for emulator device
		struct CZDeviceInfo info;

		if(CZCudaReadDeviceInfo(&info, 0) == -1) {
			CZLog(CZLogLevelHigh, "CUDA Devices error: Can't get device info!");
			return 0;
		}

		if(info.deviceName[0] == 0) {
			CZLog(CZLogLevelHigh, "CUDA Devices error: Emulator detected!");
			return 0;
		}
	}

	return res;
}

#ifdef Q_OS_WIN
#include <Windows.h>
/*!	\brief Sleep function.
*/
static inline void sleep(
	unsigned sec		/*!<[in] Number of seconds to wait */
) {
	::Sleep(sec * 1000);
}

/*!	\brief Do a console detection in windows and restart the app in detouched mode.
	\note Inspired by behavior of ildasm.exe process on startup.
*/
static void check_open_windows_console(
	void
) {
	/* check if console is attached */
	HANDLE out_hdl = GetStdHandle(STD_OUTPUT_HANDLE);

	if(out_hdl != INVALID_HANDLE_VALUE) {
		/* if we are still attached to a console we should restart detouched */

		LPTSTR command_line = GetCommandLine();

		STARTUPINFO startup_info;
		memset(&startup_info, 0, sizeof(startup_info));
		startup_info.dwFlags = STARTF_USESTDHANDLES;
		startup_info.hStdInput = INVALID_HANDLE_VALUE;
		startup_info.hStdOutput = INVALID_HANDLE_VALUE;
		startup_info.hStdError = INVALID_HANDLE_VALUE;

		PROCESS_INFORMATION process_information;

		BOOL success = CreateProcess(NULL, command_line, NULL, NULL,
			TRUE, DETACHED_PROCESS, NULL, NULL, &startup_info, &process_information);

		if(!success) {
			printf("Can't start myself detouched!\n");
		}

		exit(!success);
	}

}

#endif

/*!	\brief Main initialization function for CLI mode.
*/
static int main_cli(
	int argc,		/*!<[in] Count of command line arguments. */
	char *argv[]		/*!<[in] List of command line arguments. */
) {
	QCoreApplication app(argc, argv);

	CZLog(CZLogLevelHigh, QObject::tr("Checking CUDA ..."));
	if(!testCudaPresent()) {
		CZLog(CZLogLevelFatal, QObject::tr("CUDA not found!"));
		CZLog(CZLogLevelHigh, QObject::tr("Please update your NVIDIA driver and try again!"));
		exit(1);
	}

	int devs = getCudaDeviceNum();
	if(devs == 0) {
		CZLog(CZLogLevelFatal, QObject::tr("No compatible CUDA devices found!"));
		CZLog(CZLogLevelHigh, QObject::tr("Please update your NVIDIA driver and try again!"));
		exit(1);
	}

	CZLog(CZLogLevelHigh, QObject::tr("Found %1 CUDA Device(s) ...").arg(devs));

	// TODO - add more info functionality here, e.g. generating a text-file export or running a certain test set
	CZPrintCommandLineHelp();

	return 0;
}

/*!	\brief Main initialization function for GUI mode.
*/
static int main_gui(
	int argc,		/*!<[in] Count of command line arguments. */
	char *argv[]		/*!<[in] List of command line arguments. */
) {
#ifdef Q_OS_WIN
	check_open_windows_console();
#endif

	QApplication app(argc, argv);

	CZLog(CZLogLevelHigh, QObject::tr("CUDA-Z Started!"));

	QPixmap pixmap1(":/img/splash1.png");
	QPixmap pixmap2(":/img/splash2.png");
	QPixmap pixmap3(":/img/splash3.png");

	splash = new CZSplashScreen(pixmap1, 2);
	splash->show();

	splash->showMessage(QObject::tr("Checking CUDA ..."),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();
	if(!testCudaPresent()) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("CUDA not found!") + "\n" +
			QObject::tr("Please update your NVIDIA driver and try again!"));
		delete splash;
		exit(1);
	}

//	sleep(5);

	int devs = getCudaDeviceNum();
	if(devs == 0) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("No compatible CUDA devices found!") + "\n" +
			QObject::tr("Please update your NVIDIA driver and try again!"));
		delete splash;
		exit(1);
	}

	splash->setPixmap(pixmap2);
	splash->showMessage(QObject::tr("Found %1 CUDA Device(s) ...").arg(devs),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();

//	sleep(5);

	splash->setPixmap(pixmap3);
	CZDialog window;
	window.show(); 
	splash->finish(&window);

	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	delete splash;
	return app.exec();

	CZLog(CZLogLevelHigh, QObject::tr("CUDA-Z Stopped!"));
}

/*!	\brief Main initialization function.
*/
int main(
	int argc,		/*!<[in] Count of command line arguments. */
	char *argv[]		/*!<[in] List of command line arguments. */
) {
	bool bCli = false;

	for(int n = 1; n < argc; n++) {
		if(stricmp(argv[n], "-cli") == 0)
			bCli = true;
	}

	if(bCli)
		return main_cli(argc, argv);
	else
		return main_gui(argc, argv);
}

