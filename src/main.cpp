/*!
	\file main.cpp
	\brief Main source file.
	\author AG
*/

#include <QApplication>
#include <QMessageBox>
#include <QSplashScreen>
#include <QDebug>

#include "czdialog.h"
#include "cudainfo.h"
#include "version.h"

/*!
	\brief Call function that checks CUDA presents.
*/
bool testCudaPresent() {
	bool res = cudaCheck();
	qDebug() << "CUDA Present:" << res;
	return res;
}

/*!
	\brief Call function that returns numbed of CUDA-devices.
*/
int getCudaDeviceNum() {
	int res = cudaDeviceFound();
	qDebug() << "CUDA Devices found:" << res;
	return res;
}

/*!
	\brief Busy loop wait function.
	Actually, I don't know how to call sleep() function from Qt code :).
*/
void wait(int n) {
	char str[256];
	int i;

	for(i = 0; i < n; i++) {
		sprintf(str, "loop%d", i);
	}
}

/*!
	\brief Main initialization function.
*/
int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

	QPixmap pixmap(":/img/splash.png");
	QSplashScreen splash(pixmap);
	splash.show();
	app.processEvents();

//	wait(10000000);

	splash.showMessage(QObject::tr("Checking CUDA ..."),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();
	if(!testCudaPresent()) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("CUDA not found!"));
		exit(1);
	}

//	wait(10000000);

	int devs = getCudaDeviceNum();
	if(devs == 0) {
		QMessageBox::critical(0, QObject::tr(CZ_NAME_LONG),
			QObject::tr("No CUDA devices found!"));
		exit(1);
	}

	splash.showMessage(QObject::tr("Found %1 CUDA Device(s) ...").arg(devs),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();

//	wait(10000000);

	CZDialog window;
	window.show(); 
	splash.finish(&window);

	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	return app.exec();
}
