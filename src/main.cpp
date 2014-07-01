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
#endif

/*!	\brief Main initialization function.
*/
int main(
	int argc,		/*!<[in] Count of command line arguments. */
	char *argv[]		/*!<[in] List of command line arguments. */
) {

	QApplication app(argc, argv);

	CZLog(CZLogLevelHigh, "CUDA-Z Started!");

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
			QObject::tr("CUDA not found!"));
		delete splash;
		exit(1);
	}

//	sleep(5);

	int devs = getCudaDeviceNum();
	if(devs == 0) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("No CUDA devices found!"));
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

	CZLog(CZLogLevelHigh, "CUDA-Z Stopped!");
}
