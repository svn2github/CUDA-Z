/*!
	\file czdialog.cpp
	\brief Main window implementation source file.
	\author AG
*/

#include <QDebug>

#include "czdialog.h"
#include "version.h"

/*
	\brief Splash screen of application.
*/
QSplashScreen *splash;

/*!
	\class CZUpdateThread
	\brief This class implements bandwidth data update procedure.
*/

/*!
	\brief Creates the bandwidth data update thread.
*/
CZUpdateThread::CZUpdateThread(
	QObject *parent			/*!< Parent of the thread. */
)	: QThread(parent) {

	restart = false;
	abort = false;
	info = NULL;
	index = -1;

	qDebug() << "Thread created";
}

/*!
	\brief Terminates the bandwidth data update thread.
	This function waits util bandwidth test will be over.
*/
CZUpdateThread::~CZUpdateThread() {

	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();

	wait();

	qDebug() << "Thread is done";
}

/*!
	\brief Push bandwidth test in thread.
*/
void CZUpdateThread::testBandwidth(
	struct CZDeviceInfo *info,	/*!< Information about CUDA-device. */
	int index			/*!< Index of device in list. */
) {
	QMutexLocker locker(&mutex);

	this->info = info;
	this->index = index;

	qDebug() << "Rising update action for device" << index;

	if (!isRunning()) {
		start();
	} else {
		restart = true;
		condition.wakeOne();
	}
}

/*!
	\brief Main work function of the thread.
*/
void CZUpdateThread::run() {

	static struct CZDeviceInfo *lastInfo = NULL;
	static int lastIndex = -1;

	forever {
	        mutex.lock();
		struct CZDeviceInfo *info = this->info;
		int index = this->index;
        	mutex.unlock();

		if((lastIndex != index) && (lastIndex != -1)) {
			cudaCleanDevice(lastInfo);
		}

		if(abort) {
			cudaCleanDevice(info);
			return;
		}

		if(info != NULL) {
			qDebug() << "Starting test for device" << index;
			cudaCalcDeviceBandwidth(info);
			if(!restart)
				emit testedBandwidth(index);
			lastInfo = info;
			lastIndex = index;
		}

		mutex.lock();
		if(!restart)
			condition.wait(&mutex);
		restart = false;
		mutex.unlock();
	}
}

/*!
	\class CZDialog
	\brief This class implements main window of the application.
*/

/*!
	\brief Creates a new #CZDialog with the given \a parent.
	This function does following steps:
	- Sets up GUI.
	- Reads out CUDA-device information in to list.
	- Sets up connections.
	- Fills up data in to tabs of GUI.
	- Starts bandwidth data update thread.
*/
CZDialog::CZDialog(
	QWidget *parent,	/*!< Parent of widget. */
	Qt::WFlags f		/*!< Window flags. */
)	: QDialog(parent, f /*| Qt::MSWindowsFixedSizeDialogHint*/ | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint) {

	setupUi(this);
	readCudaDevices();

//	qDebug() << "device list size:" << deviceList.size();

	this->setWindowTitle(QString("%1 %2").arg(CZ_NAME_SHORT).arg(CZ_VERSION));
	connect(comboDevice, SIGNAL(activated(int)), SLOT(slotShowDevice(int)));
	
	setupDeviceList();
	setupDeviceInfo(comboDevice->currentIndex());
	setupAboutTab();
	freeCudaDevices();

	updateThread = new CZUpdateThread(this);
	updateThread->start();
	connect(updateThread, SIGNAL(testedBandwidth(int)), SLOT(slotUpdateBandwidth(int)));

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), SLOT(slotUpdateTimer()));
	updateTimer->start(5000);
}

/*
	\brief Class destructor. This function makes class data cleanup actions.
*/
CZDialog::~CZDialog() {
	updateTimer->stop();
	delete updateTimer;
	delete updateThread;
//	freeCudaDevices();
}

/*!
	\brief Read CUDA devices information.
	For each of detected CUDA-devices does following:
	- Initialize CUDA-data structure.
	- Reads CUDA-information about device.
	- Shows progress message in splash screen.
	- Starts bandwidth calculation.
	- Appends entry in to device-list.
*/
void CZDialog::readCudaDevices() {

	int num;

	num = getCudaDeviceNumber();

	for(int i = 0; i < num; i++) {
		struct CZDeviceInfo info;
		memset(&info, 0, sizeof(info));

		if(readCudaDeviceInfo(info, i) == 0) {
			splash->showMessage(tr("Getting information about %1 ...").arg(info.deviceName),
				Qt::AlignLeft | Qt::AlignBottom);
			qApp->processEvents();

//			wait(10000000);
			
			if(calcCudaDeviceBandwidth(info) != 0) {
				qDebug() << "Can't calc bandwidth for" << info.deviceName;
			}
			deviceList.append(info);
		}
	}
}

/*
	\brief Cleanup after bandwidth tests.
*/
void CZDialog::freeCudaDevices() {

	for(int i = 0; i < deviceList.size(); i++) {
		cudaCleanDevice(&deviceList[i]);
	}
}

/*!
	\brief Get number of CUDA devices.
	\return number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
*/
int CZDialog::getCudaDeviceNumber() {
	return cudaDeviceFound();
}

/*!
	\brief Get information about CUDA device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZDialog::readCudaDeviceInfo(
	struct CZDeviceInfo &info,	/*!< Information about CUDA-device. */
	int dev				/*!< Number of CUDA-device. */
) {
	return cudaReadDeviceInfo(&info, dev);
}

/*!
	\brief Get information about CUDA device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZDialog::calcCudaDeviceBandwidth(
	struct CZDeviceInfo &info	/*!< Information about CUDA-device. */
) {
	return cudaCalcDeviceBandwidth(&info);
}

/*!
	\brief Put devices in combo box.
*/
void CZDialog::setupDeviceList() {
	comboDevice->clear();

	for(int i = 0; i < deviceList.size(); i++) {
		comboDevice->addItem(QString("%1: %2").arg(i).arg(deviceList[i].deviceName));
	}
}

/*!
	\brief This slot shows in dialog information about given device.
*/
void CZDialog::slotShowDevice(
	int index			/*!< Index of device in list. */
) {
	setupDeviceInfo(index);
	if(checkUpdateResults->checkState() == Qt::Checked) {
		qDebug() << "Switch device -> update bandwidth for device" << index;
		updateThread->testBandwidth(&deviceList[index], index);
	}
}

/*!
	\brief This slot updates bandwidth information of device
	pointed by \a index.
*/
void CZDialog::slotUpdateBandwidth(
	int index			/*!< Index of device in list. */
) {
	if(index == comboDevice->currentIndex())
	setupBandwidthTab(deviceList[index]);
}

/*!
	\brief This slot updates bandwidth information of current device
	every timer tick.
*/
void CZDialog::slotUpdateTimer() {

	int index = comboDevice->currentIndex();
	if(checkUpdateResults->checkState() == Qt::Checked) {
		qDebug() << "Timer shot -> update bandwidth for device" << index;
		updateThread->testBandwidth(&deviceList[index], index);
	} else {
		qDebug() << "Timer shot -> update ignored";
	}
}

/*!
	\brief Place in dialog's tabs information about given device.
*/
void CZDialog::setupDeviceInfo(
	int dev				/*!< Number of CUDA-device. */
) {
	setupCoreTab(deviceList[dev]);
	setupMemoryTab(deviceList[dev]);
	setupBandwidthTab(deviceList[dev]);
}

/*!
	\brief Fill tab "Core" with CUDA devices information.
*/
void CZDialog::setupCoreTab(
	struct CZDeviceInfo &info	/*!< Information about CUDA-device. */
) {
	QString deviceName(info.deviceName);

	labelNameText->setText(deviceName);
	labelCapabilityText->setText(QString("%1.%2").arg(info.major).arg(info.minor));
	labelClockText->setText(tr("%1 MHz").arg((double)info.core.clockRate / 1000));
	labelWrapText->setNum(info.core.SIMDWidth);
	labelRegsText->setNum(info.core.regsPerBlock);
	labelThreadsText->setNum(info.core.maxThreadsPerBlock);
	labelThreadsDimTextX->setNum(info.core.maxThreadsDim[0]);
	labelThreadsDimTextY->setNum(info.core.maxThreadsDim[1]);
	labelThreadsDimTextZ->setNum(info.core.maxThreadsDim[2]);
	labelGridDimTextX->setNum(info.core.maxGridSize[0]);
	labelGridDimTextY->setNum(info.core.maxGridSize[1]);
	labelGridDimTextZ->setNum(info.core.maxGridSize[2]);

	labelDeviceLogo->setPixmap(QPixmap(":/img/logo-unknown.png"));
	if(deviceName.contains("tesla", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-tesla.png"));
	} else
	if(deviceName.contains("quadro", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-quadro.png"));
	} else
	if(deviceName.contains("geforce", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-geforce.png"));
	}
}

/*!
	\brief Fill tab "Memory" with CUDA devices information.
*/
void CZDialog::setupMemoryTab(
	struct CZDeviceInfo &info	/*!< Information about CUDA-device. */
) {
	labelTotalGlobalText->setText(tr("%1 MB").arg((double)info.mem.totalGlobal / (1024 * 1024)));
	labelSharedText->setText(tr("%1 KB").arg((double)info.mem.sharedPerBlock / 1024));
	labelPitchText->setText(tr("%1 KB").arg((double)info.mem.maxPitch / 1024));
	labelTotalConstText->setText(tr("%1 KB").arg((double)info.mem.totalConst / 1024));
	labelTextureAlignmentText->setNum(info.mem.textureAlignment);
	labelGpuOverlapText->setText(info.mem.gpuOverlap? tr("Yes"): tr("No"));
}

/*!
	\brief Fill tab "Bandwidth" with CUDA devices information.
*/
void CZDialog::setupBandwidthTab(
	struct CZDeviceInfo &info	/*!< Information about CUDA-device. */
) {

	qDebug() << "BANDWIDTH:";
	qDebug() << "copyHDPin:" << info.band.copyHDPin;
	qDebug() << "copyHDPage:" << info.band.copyHDPage;
	qDebug() << "copyDHPin:" << info.band.copyDHPin;
	qDebug() << "copyDHPage:" << info.band.copyDHPage;
	qDebug() << "copyDD:" << info.band.copyDD;

	if(info.band.copyHDPin == 0)
		labelHDRatePinText->setText("--");
	else
		labelHDRatePinText->setText(tr("%1 MB/s").arg((double)info.band.copyHDPin / 1024));

	if(info.band.copyHDPage == 0)
		labelHDRatePageText->setText("--");
	else
		labelHDRatePageText->setText(tr("%1 MB/s").arg((double)info.band.copyHDPage / 1024));

	if(info.band.copyDHPin == 0)
		labelDHRatePinText->setText("--");
	else
		labelDHRatePinText->setText(tr("%1 MB/s").arg((double)info.band.copyDHPin / 1024));

	if(info.band.copyDHPage == 0)
		labelDHRatePageText->setText("--");
	else
		labelDHRatePageText->setText(tr("%1 MB/s").arg((double)info.band.copyDHPage / 1024));

	if(info.band.copyDD == 0)
		labelDDRateText->setText("--");
	else
		labelDDRateText->setText(tr("%1 MB/s").arg((double)info.band.copyDD / 1024));
}

/*!
	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
//	labelAppLogo->setPixmap(QPixmap(":/img/icon.png"));
	labelAppName->setText(QString("<b><i><font size=\"+2\">%1</font></i></b><br>%2").arg(CZ_NAME_SHORT).arg(CZ_NAME_LONG));

	QString version = tr("<b>Version</b> %1").arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	version += QString(tr("<br><b>Built</b> %1 %2").arg(CZ_DATE).arg(CZ_TIME));
#endif//CZ_VER_STATE
	labelAppVersion->setText(version);
	labelAppURL->setText(tr("<b>Main page</b> <a href=\"%1\">%1</a><br><b>Project</b> <a href=\"%2\">%2</a>").arg(CZ_ORG_URL_MAINPAGE).arg(CZ_ORG_URL_PROJECT));
	labelAppAuthor->setText(tr("<b>Author</b> %1").arg(CZ_ORG_NAME));
	labelAppCopy->setText(CZ_COPY_INFO);
}
