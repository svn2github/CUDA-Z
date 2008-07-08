/*!
	\file main.cpp
	\brief Main source file.
	\author AG
*/

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

#include "czdialog.h"
#include "cudainfo.h"
#include "version.h"

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

	QPixmap pixmap(":/img/splash.png");
	splash = new QSplashScreen(pixmap);
	splash->show();

	splash->showMessage(QObject::tr("Checking CUDA ..."),
		Qt::AlignLeft | Qt::AlignBottom);
	app.processEvents();

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
}
