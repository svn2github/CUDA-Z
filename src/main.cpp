/*!
	\file main.cpp
	\brief Main source file.
	\author AG
*/

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "log.h"
#include "czdialog.h"
#include "cudainfo.h"
#include "version.h"

/*!
	\brief Call function that checks CUDA presents.
*/
bool testCudaPresent() {
	bool res = CZCudaCheck();
	CZLog(CZLogLevelHigh, "CUDA Present: %d", res);
	return res;
}

/*!
	\brief Call function that returns numbed of CUDA-devices.
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
			CZLog(CZLogLevelHigh, "CUDA Devices error: Emilator detected!");
			return 0;
		}
	}

	return res;
}

/*!
	\brief Busy loop wait function (brutal hack for testing).
	Actually, I don't know how to call sleep() function from Qt code :).
*/
void wait(
	int n			/*!< Number of times to tun this busy loop to. */
) {
	char str[256];
	int i;

	for(i = 0; i < n; i++) {
		sprintf(str, "loop%d", i);
	}
}

/*!
	\brief Main initialization function.
*/
int main(
	int argc,		/*!< Count of command line arguments. */
	char *argv[]		/*!< List of command line arguments. */
) {

	QApplication app(argc, argv);

	CZLog(CZLogLevelHigh, "CUDA-Z Started!");

	QPixmap pixmap(":/img/splash.png");
	splash = new CZSplashScreen(pixmap, 2);
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

//	wait(10000000);

	int devs = getCudaDeviceNum();
	if(devs == 0) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("No CUDA devices found!"));
		delete splash;
		exit(1);
	}

	splash->showMessage(QObject::tr("Found %1 CUDA Device(s) ...").arg(devs),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();

//	wait(10000000);

	CZDialog window;
	window.show(); 
	splash->finish(&window);

	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	delete splash;
	return app.exec();

	CZLog(CZLogLevelHigh, "CUDA-Z Stopped!");
}
