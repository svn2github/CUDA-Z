/*!
	\file czdialog.cpp
	\brief Main window implementation source file.
	\author AG
*/

#include <QDebug>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

#include "czdialog.h"
#include "version.h"

#define CZ_TIMER_REFRESH	2000	/*!< Test results update timer period (ms). */

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
	\brief This function updates CUDA-device performance information.
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
	\brief This function cleans buffers used for bandwidth tests.
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
	\brief This class implements performance data update procedure.
*/

/*!
	\brief Creates the performance data update thread.
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
	\brief Terminates the performance data update thread.
	This function waits util performance test will be over.
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
	\brief Push performance test in thread.
*/
void CZUpdateThread::testPerformance(
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

	forever {

		mutex.lock();
		condition.wait(&mutex);
		index = this->index;
		mutex.unlock();

		qDebug() << "Thread loop started";

		if(abort) {
			info->cleanDevice();
			return;
		}

		info->updateInfo();
		if(index != -1)
			emit testedPerformance(index);

		if(abort) {
			info->cleanDevice();
			return;
		}
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
	- Starts Performance data update timer.
*/
CZDialog::CZDialog(
	QWidget *parent,	/*!< Parent of widget. */
	Qt::WFlags f		/*!< Window flags. */
)	: QDialog(parent, f /*| Qt::MSWindowsFixedSizeDialogHint*/ | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint) {

	setupUi(this);
	this->setWindowTitle(QString("%1 %2").arg(CZ_NAME_SHORT).arg(CZ_VERSION));
	connect(comboDevice, SIGNAL(activated(int)), SLOT(slotShowDevice(int)));

	QMenu *exportMenu = new QMenu(pushExport);
	exportMenu->addAction(tr("to &Text"), this, SLOT(slotExportToText()));
	exportMenu->addAction(tr("to &HTML"), this, SLOT(slotExportToHTML()));
	pushExport->setMenu(exportMenu);
	
	readCudaDevices();
//	qDebug() << "device list size:" << deviceList.size();
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
	- Starts Performance calculation thread.
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
			
			connect(info->thread(), SIGNAL(testedPerformance(int)), SLOT(slotUpdatePerformance(int)));
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
		qDebug() << "Switch device -> update performance for device" << index;
		deviceList[index]->thread()->testPerformance(index);
	}
}

/*!
	\brief This slot updates performance information of device
	pointed by \a index.
*/
void CZDialog::slotUpdatePerformance(
	int index			/*!< Index of device in list. */
) {
	if(index == comboDevice->currentIndex())
	setupPerformanceTab(deviceList[index]->info());
}

/*!
	\brief This slot updates performance information of current device
	every timer tick.
*/
void CZDialog::slotUpdateTimer() {

	int index = comboDevice->currentIndex();
	if(checkUpdateResults->checkState() == Qt::Checked) {
		qDebug() << "Timer shot -> update performance for device" << index;
		deviceList[index]->thread()->testPerformance(index);
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
	setupPerformanceTab(deviceList[dev]->info());
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
	\brief Fill tab "Performance" with CUDA devices information.
*/
void CZDialog::setupPerformanceTab(
	struct CZDeviceInfo &info	/*!< Information about CUDA-device. */
) {

/*	qDebug() << "PERFORMANCE:";
	qDebug() << "copyHDPin:" << info.band.copyHDPin;
	qDebug() << "copyHDPage:" << info.band.copyHDPage;
	qDebug() << "copyDHPin:" << info.band.copyDHPin;
	qDebug() << "copyDHPage:" << info.band.copyDHPage;
	qDebug() << "copyDD:" << info.band.copyDD;
	qDebug() << "calcFloat:" << info.perf.calcFloat;
	qDebug() << "calcInteger32:" << info.perf.calcInteger32;
	qDebug() << "calcInteger24:" << info.perf.calcInteger24;*/

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

#if defined(Q_WS_WIN)
#include <windows.h>
static QString getOSVersion() {
	QString OSVersion = "Windows";

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	OSVersion += QString(" %1").arg(
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)? "AMD64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)? "IA64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)? "x86":
		"Unknown architecture");

	OSVERSIONINFO versionInfo;
	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);
	OSVersion += QString(" %1.%2.%3 %4")
		.arg(versionInfo.dwMajorVersion)
		.arg(versionInfo.dwMinorVersion)
		.arg(versionInfo.dwBuildNumber)
		.arg(QString::fromWCharArray(versionInfo.szCSDVersion));

	return OSVersion;
}
#else//!Q_WS_WIN
#error Function getOSVersion() is not implemented for your platform!
#endif//Q_WS_WIN

void CZDialog::slotExportToText() {

	struct CZDeviceInfo info = deviceList[comboDevice->currentIndex()]->info();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text as.."),
		tr("%1.txt").arg(tr(CZ_NAME_SHORT)), tr("Text files (*.txt);;All files (*.*)"));

	if(fileName.isEmpty())
		return;

	qDebug() << "Export to text as" << fileName;

	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(CZ_NAME_SHORT),
			tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	QString title = tr("%1 Report").arg(tr(CZ_NAME_SHORT));
	QString subtitle;

	out << title << endl;
	for(int i = 0; i < title.size(); i++)
		out << "=";
	out << endl;
	out << tr("Version: %1").arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	out << tr(" Built %1 %2 ").arg(CZ_DATE).arg(CZ_TIME);
#endif//CZ_VER_STATE
	out << endl;
	out << CZ_ORG_URL_MAINPAGE << endl;
	out << tr("OS Version: %1").arg(getOSVersion()) << endl;
	out << endl;

	subtitle = tr("Core Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << "\t" << tr("Name: %1").arg(info.deviceName) << endl;
	out << "\t" << tr("Compute Capability: %1.%2").arg(info.major).arg(info.minor) << endl;
	out << "\t" << tr("Clock Rate: %1 MHz").arg((double)info.core.clockRate / 1000) << endl;
	out << "\t" << tr("Wrap Size: %1").arg(info.core.SIMDWidth) << endl;
	out << "\t" << tr("Regs Per Block: %1").arg(info.core.regsPerBlock) << endl;
	out << "\t" << tr("Threads Per Block: %1").arg(info.core.maxThreadsPerBlock) << endl;
	out << "\t" << tr("Threads Dimentions: %1 x %2 x %3").arg(info.core.maxThreadsDim[0]).arg(info.core.maxThreadsDim[1]).arg(info.core.maxThreadsDim[2]) << endl;
	out << "\t" << tr("Grid Dimentions: %1 x %2 x %3").arg(info.core.maxGridSize[0]).arg(info.core.maxGridSize[1]).arg(info.core.maxGridSize[2]) << endl;
	out << endl;

	subtitle = tr("Memory Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << "\t" << tr("Total Global: %1 MB").arg((double)info.mem.totalGlobal / (1024 * 1024)) << endl;
	out << "\t" << tr("Shared Per Block: %1 KB").arg((double)info.mem.sharedPerBlock / 1024) << endl;
	out << "\t" << tr("Pitch: %1 KB").arg((double)info.mem.maxPitch / 1024) << endl;
	out << "\t" << tr("Total Constant: %1 KB").arg((double)info.mem.totalConst / 1024) << endl;
	out << "\t" << tr("Texture Alignment: %1").arg(info.mem.textureAlignment) << endl;
	out << "\t" << tr("GPU Overlap: %1").arg(info.mem.gpuOverlap? tr("Yes"): tr("No")) << endl;
	out << endl;

	subtitle = tr("Performance Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << tr("Memory Copy") << endl;
	out << "\t" << tr("Host Pinned to Device: ");
	if(info.band.copyHDPin == 0)
		out << "--" << endl;
	else
		out << tr("%1 MB/s").arg((double)info.band.copyHDPin / 1024) << endl;
	out << "\t" << tr("Host Pageable to Device: ");
	if(info.band.copyHDPage == 0)
		out << "--" << endl;
	else
		out << tr("%1 MB/s").arg((double)info.band.copyHDPage / 1024) << endl;

	out << "\t" << tr("Device to Host Pinned: ");
	if(info.band.copyDHPin == 0)
		out << "--" << endl;
	else
		out << tr("%1 MB/s").arg((double)info.band.copyDHPin / 1024) << endl;
	out << "\t" << tr("Device to Host Pageable: ");
	if(info.band.copyDHPage == 0)
		out << "--" << endl;
	else
		out << tr("%1 MB/s").arg((double)info.band.copyDHPage / 1024) << endl;
	out << "\t" << tr("Device to Device: ");
	if(info.band.copyDD == 0)
		out << "--" << endl;
	else
		out << tr("%1 MB/s").arg((double)info.band.copyDD / 1024) << endl;
	out << tr("GPU Core Performance") << endl;
	out << "\t" << tr("Float Point: ");
	if(info.perf.calcFloat == 0)
		out << "--" << endl;
	else
		out << tr("%1 Mflop/s").arg((double)info.perf.calcFloat / 1000) << endl;
	out << "\t" << tr("32-bit Integer: ");
	if(info.perf.calcInteger32 == 0)
		out << "--" << endl;
	else
		out << tr("%1 Miop/s").arg((double)info.perf.calcInteger32 / 1000) << endl;
	out << "\t" << tr("24-bit Integer: ");
	if(info.perf.calcInteger24 == 0)
		out << "--" << endl;
	else
		out << tr("%1 Miop/s").arg((double)info.perf.calcInteger24 / 1000) << endl;
	out << endl;
}

void CZDialog::slotExportToHTML() {

	struct CZDeviceInfo info = deviceList[comboDevice->currentIndex()]->info();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text as.."),
		tr("%1.html").arg(tr(CZ_NAME_SHORT)), tr("HTML files (*.html *.htm);;All files (*.*)"));

	if(fileName.isEmpty())
		return;

	qDebug() << "Export to HTML as" << fileName;

}
