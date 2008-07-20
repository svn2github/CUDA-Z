/*!
	\file czdialog.cpp
	\brief Main window implementation source file.
	\author AG
*/

#include <QDebug>

#include "czdialog.h"
#include "version.h"

#define CZ_TIMER_REFRESH	5000	/*!< Test results update timer period (ms). */

/*!
	\brief Splash screen of application.
*/
QSplashScreen *splash;

/*!
	\class CZCudaDeviceInfo
	\brief This class implements a container for CUDA-device information.
*/

/*!
	\brief Creates CUDA-device information container.
*/
CZCudaDeviceInfo::CZCudaDeviceInfo(
	int devNum,
	QObject *parent
) 	: QObject(parent) {
	memset(&_info, 0, sizeof(_info));
	_info.num = devNum;
	readInfo();
	_thread = new CZUpdateThread(this, this);
	_thread->start();
}

/*!
	\brief Destroys cuda information container.
*/
CZCudaDeviceInfo::~CZCudaDeviceInfo() {
	delete _thread;
}

/*!
	\brief This function reads CUDA-device basic information.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::readInfo() {
	return CZCudaReadDeviceInfo(&_info, _info.num);
}

/*!
	\brief This function prepare some buffers for budwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::prepareDevice() {
	return CZCudaPrepareDevice(&_info);
}

/*!
	\brief This function updates CUDA-device bandwidth information.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::updateInfo() {
	int r;
	r = CZCudaCalcDeviceBandwidth(&_info);
	if(r == -1)
		return r;
	return CZCudaCalcDevicePerformance(&_info);
}

/*!
	\brief This function cleans buffers used for budwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::cleanDevice() {
	return CZCudaCleanDevice(&_info);
}

/*!
	\brief Returns pointer to inforation structure.
*/
struct CZDeviceInfo &CZCudaDeviceInfo::info() {
	return _info;
}

/*!
	\brief Returns pointer to update thread.
*/
CZUpdateThread *CZCudaDeviceInfo::thread() {
	return _thread;
}

/*!
	\class CZUpdateThread
	\brief This class implements bandwidth data update procedure.
*/

/*!
	\brief Creates the bandwidth data update thread.
*/
CZUpdateThread::CZUpdateThread(
	CZCudaDeviceInfo *info,
	QObject *parent			/*!< Parent of the thread. */
)	: QThread(parent) {

	abort = false;
	this->info = info;
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
	int index			/*!< Index of device in list. */
) {
	QMutexLocker locker(&mutex);

	this->index = index;

	qDebug() << "Rising update action for device" << index;

	if (!isRunning()) {
		start();
	} else {
		condition.wakeOne();
	}
}

/*!
	\brief Main work function of the thread.
*/
void CZUpdateThread::run() {

	qDebug() << "Thread started";

	info->prepareDevice();

	mutex.lock();
	int index = this->index;
	mutex.unlock();

	forever {

		qDebug() << "Thread loop started";

		if(abort) {
			info->cleanDevice();
			return;
		}

		info->updateInfo();
		if(index != -1)
			emit testedBandwidth(index);

		if(abort) {
			info->cleanDevice();
			return;
		}

		mutex.lock();
		condition.wait(&mutex);
		index = this->index;
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
	- Setup CUDA-device information containers and add them in list.
	- Sets up connections.
	- Fills up data in to tabs of GUI.
	- Starts bandwidth data update timer.
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

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), SLOT(slotUpdateTimer()));
	updateTimer->start(CZ_TIMER_REFRESH);
}

/*
	\brief Class destructor.
	This function makes class data cleanup actions.
*/
CZDialog::~CZDialog() {
	updateTimer->stop();
	delete updateTimer;
	freeCudaDevices();
}

/*!
	\brief Read CUDA devices information.
	For each of detected CUDA-devices does following:
	- Initialize CUDA-data structure.
	- Reads CUDA-information about device.
	- Shows progress message in splash screen.
	- Starts bandwidth calculation thread.
	- Appends entry in to device-list.
*/
void CZDialog::readCudaDevices() {

	int num = getCudaDeviceNumber();

	for(int i = 0; i < num; i++) {

		CZCudaDeviceInfo *info = new CZCudaDeviceInfo(i);

		if(info->info().major != 0) {
			splash->showMessage(tr("Getting information about %1 ...").arg(info->info().deviceName),
				Qt::AlignLeft | Qt::AlignBottom);
			qApp->processEvents();

//			wait(10000000);
			
			connect(info->thread(), SIGNAL(testedBandwidth(int)), SLOT(slotUpdateBandwidth(int)));
			deviceList.append(info);
		}
	}
}

/*
	\brief Cleanup after bandwidth tests.
*/
void CZDialog::freeCudaDevices() {

	while(deviceList.size() > 0) {
		CZCudaDeviceInfo *info = deviceList[0];
		deviceList.removeFirst();
		delete info;
	}
}

/*!
	\brief Get number of CUDA devices.
	\return number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
*/
int CZDialog::getCudaDeviceNumber() {
	return CZCudaDeviceFound();
}

/*!
	\brief Put devices in combo box.
*/
void CZDialog::setupDeviceList() {
	comboDevice->clear();

	for(int i = 0; i < deviceList.size(); i++) {
		comboDevice->addItem(QString("%1: %2").arg(i).arg(deviceList[i]->info().deviceName));
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
		deviceList[index]->thread()->testBandwidth(index);
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
	setupBandwidthTab(deviceList[index]->info());
}

/*!
	\brief This slot updates bandwidth information of current device
	every timer tick.
*/
void CZDialog::slotUpdateTimer() {

	int index = comboDevice->currentIndex();
	if(checkUpdateResults->checkState() == Qt::Checked) {
		qDebug() << "Timer shot -> update bandwidth for device" << index;
		deviceList[index]->thread()->testBandwidth(index);
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
	setupCoreTab(deviceList[dev]->info());
	setupMemoryTab(deviceList[dev]->info());
	setupBandwidthTab(deviceList[dev]->info());
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
	qDebug() << "calcFloat:" << info.perf.calcFloat;
	qDebug() << "calcInteger32:" << info.perf.calcInteger32;
	qDebug() << "calcInteger24:" << info.perf.calcInteger24;

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

	if(info.perf.calcFloat == 0)
		labelFloatRateText->setText("--");
	else
		labelFloatRateText->setText(tr("%1 Mflop/s").arg((double)info.perf.calcFloat / 1000));

	if(info.perf.calcInteger32 == 0)
		labelInt32RateText->setText("--");
	else
		labelInt32RateText->setText(tr("%1 Miop/s").arg((double)info.perf.calcInteger32 / 1000));

	if(info.perf.calcInteger24 == 0)
		labelInt24RateText->setText("--");
	else
		labelInt24RateText->setText(tr("%1 Miop/s").arg((double)info.perf.calcInteger24 / 1000));
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
