/*!
	\file czdialog.cpp
	\brief Main window implementation source file.
	\author AG
*/

#include <QDebug>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

#include <time.h>

#include "czdialog.h"
#include "version.h"

#define CZ_TIMER_REFRESH	2000	/*!< Test results update timer period (ms). */

/*
	\def CZ_OS_PLATFORM_STR Platform ID string.
*/
#if defined(Q_OS_WIN)
#define CZ_OS_PLATFORM_STR	"win32"
#elif defined(Q_OS_MAC)
#define CZ_OS_PLATFORM_STR	"macosx"
#elif defined(Q_OS_LINUX)
#define CZ_OS_PLATFORM_STR	"linux"
#else
#error Your platform is not supported by CUDA! Or it does but I know nothing about this...
#endif

/*!
	\brief Splash screen of application.
*/
QSplashScreen *splash;

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

	http = NULL;

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

	labelAppUpdate->setText(tr("Looking for new version..."));
	startGetHistoryHttp();
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
	- Starts Performance calculation procedure.
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
			info->waitPerformance();
			
			connect(info, SIGNAL(testedPerformance(int)), SLOT(slotUpdatePerformance(int)));
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
		deviceList[index]->testPerformance(index);
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
		deviceList[index]->testPerformance(index);
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
	labelClockText->setText(QString("%1 %2").arg((double)info.core.clockRate / 1000).arg(tr("MHz")));
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
	labelTotalGlobalText->setText(QString("%1 %2")
		.arg((double)info.mem.totalGlobal / (1024 * 1024)).arg(tr("MB")));
	labelSharedText->setText(QString("%1 %2")
		.arg((double)info.mem.sharedPerBlock / 1024).arg(tr("KB")));
	labelPitchText->setText(QString("%1 %2")
		.arg((double)info.mem.maxPitch / 1024).arg(tr("KB")));
	labelTotalConstText->setText(QString("%1 %2")
		.arg((double)info.mem.totalConst / 1024).arg(tr("KB")));
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
		labelHDRatePinText->setText(QString("%1 %2")
			.arg((double)info.band.copyHDPin / 1024).arg(tr("MB/s")));

	if(info.band.copyHDPage == 0)
		labelHDRatePageText->setText("--");
	else
		labelHDRatePageText->setText(QString("%1 %2")
			.arg((double)info.band.copyHDPage / 1024).arg(tr("MB/s")));

	if(info.band.copyDHPin == 0)
		labelDHRatePinText->setText("--");
	else
		labelDHRatePinText->setText(QString("%1 %2")
			.arg((double)info.band.copyDHPin / 1024).arg(tr("MB/s")));

	if(info.band.copyDHPage == 0)
		labelDHRatePageText->setText("--");
	else
		labelDHRatePageText->setText(QString("%1 %2")
			.arg((double)info.band.copyDHPage / 1024).arg(tr("MB/s")));

	if(info.band.copyDD == 0)
		labelDDRateText->setText("--");
	else
		labelDDRateText->setText(QString("%1 %2")
			.arg((double)info.band.copyDD / 1024).arg(tr("MB/s")));

	if(info.perf.calcFloat == 0)
		labelFloatRateText->setText("--");
	else
		labelFloatRateText->setText(QString("%1 %2")
			.arg((double)info.perf.calcFloat / 1000).arg(tr("Mflop/s")));

	if(info.perf.calcInteger32 == 0)
		labelInt32RateText->setText("--");
	else
		labelInt32RateText->setText(QString("%1 %2")
			.arg((double)info.perf.calcInteger32 / 1000).arg(tr("Miop/s")));

	if(info.perf.calcInteger24 == 0)
		labelInt24RateText->setText("--");
	else
		labelInt24RateText->setText(QString("%1 %2")
			.arg((double)info.perf.calcInteger24 / 1000).arg(tr("Miop/s")));
}

/*!
	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
//	labelAppLogo->setPixmap(QPixmap(":/img/icon.png"));
	labelAppName->setText(QString("<b><i><font size=\"+2\">%1</font></i></b><br>%2")
		.arg(CZ_NAME_SHORT).arg(CZ_NAME_LONG));

	QString version = tr("<b>%1</b> %2").arg(tr("Version")).arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	version += QString(tr("<br><b>%1</b> %2 %3").arg(tr("Built")).arg(CZ_DATE).arg(CZ_TIME));
#endif//CZ_VER_STATE
	labelAppVersion->setText(version);
	labelAppURL->setText(tr("<b>%1</b> <a href=\"%2\">%2</a><br><b>%3</b> <a href=\"%4\">%4</a>")
		.arg(tr("Main page")).arg(CZ_ORG_URL_MAINPAGE)
		.arg(tr("Project")).arg(CZ_ORG_URL_PROJECT));
	labelAppAuthor->setText(tr("<b>%1</b> %2").arg(tr("Author")).arg(CZ_ORG_NAME));
	labelAppCopy->setText(CZ_COPY_INFO);
}

/*!
	\fn CZDialog::getOSVersion
	\brief Get OS version string.
	\return string that describes version of OS we running at.
*/
#ifdef Q_OS_WIN
#include <windows.h>
QString CZDialog::getOSVersion() {
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
#elif defined (Q_OS_LINUX)
QString CZDialog::getOSVersion() {
	QString OSVersion = "Linux";

	return OSVersion;
}
#else//!Q_WS_WIN
#error Function getOSVersion() is not implemented for your platform!
#endif//Q_WS_WIN

/*!
	\brief Export information to plane text file.
*/
void CZDialog::slotExportToText() {

	struct CZDeviceInfo info = deviceList[comboDevice->currentIndex()]->info();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text as..."),
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
	out << QString("%1: %2").arg(tr("Version")).arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	out << QString(" %1 %2 %3 ").arg("Built").arg(CZ_DATE).arg(CZ_TIME);
#endif//CZ_VER_STATE
	out << endl;
	out << CZ_ORG_URL_MAINPAGE << endl;
	out << QString("%1: %2").arg(tr("OS Version")).arg(getOSVersion()) << endl;
	out << endl;

	subtitle = tr("Core Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << "\t" << QString("%1: %2").arg(tr("Name")).arg(info.deviceName) << endl;
	out << "\t" << QString("%1: %2.%3").arg(tr("Compute Capability")).arg(info.major).arg(info.minor) << endl;
	out << "\t" << QString("%1: %2 %3").arg(tr("Clock Rate")).arg((double)info.core.clockRate / 1000).arg(tr("MHz")) << endl;
	out << "\t" << QString("%1: %2").arg(tr("Wrap Size")).arg(info.core.SIMDWidth) << endl;
	out << "\t" << QString("%1: %2").arg(tr("Regs Per Block")).arg(info.core.regsPerBlock) << endl;
	out << "\t" << QString("%1: %2").arg(tr("Threads Per Block")).arg(info.core.maxThreadsPerBlock) << endl;
	out << "\t" << QString("%1: %2 x %3 x %4").arg(tr("Threads Dimentions")).arg(info.core.maxThreadsDim[0]).arg(info.core.maxThreadsDim[1]).arg(info.core.maxThreadsDim[2]) << endl;
	out << "\t" << QString("%1: %2 x %3 x %4").arg(tr("Grid Dimentions")).arg(info.core.maxGridSize[0]).arg(info.core.maxGridSize[1]).arg(info.core.maxGridSize[2]) << endl;
	out << endl;

	subtitle = tr("Memory Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << "\t" << QString("%1: %2 %3").arg(tr("Total Global")).arg((double)info.mem.totalGlobal / (1024 * 1024)).arg(tr("MB")) << endl;
	out << "\t" << QString("%1: %2 %3").arg(tr("Shared Per Block")).arg((double)info.mem.sharedPerBlock / 1024).arg(tr("KB")) << endl;
	out << "\t" << QString("%1: %2 %3").arg(tr("Pitch")).arg((double)info.mem.maxPitch / 1024).arg(tr("KB")) << endl;
	out << "\t" << QString("%1: %2 %3").arg(tr("Total Constant")).arg((double)info.mem.totalConst / 1024).arg(tr("KB")) << endl;
	out << "\t" << QString("%1: %2").arg(tr("Texture Alignment")).arg(info.mem.textureAlignment) << endl;
	out << "\t" << QString("%1: %2").arg(tr("GPU Overlap")).arg(info.mem.gpuOverlap? tr("Yes"): tr("No")) << endl;
	out << endl;

	subtitle = tr("Performance Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << tr("Memory Copy") << endl;
	out << "\t" << tr("Host Pinned to Device") << ": ";
	if(info.band.copyHDPin == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.band.copyHDPin / 1024).arg(tr("MB/s")) << endl;
	out << "\t" << tr("Host Pageable to Device") << ": ";
	if(info.band.copyHDPage == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.band.copyHDPage / 1024).arg(tr("MB/s")) << endl;

	out << "\t" << tr("Device to Host Pinned") << ": ";
	if(info.band.copyDHPin == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.band.copyDHPin / 1024).arg(tr("MB/s")) << endl;
	out << "\t" << tr("Device to Host Pageable") << ": ";
	if(info.band.copyDHPage == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.band.copyDHPage / 1024).arg(tr("MB/s")) << endl;
	out << "\t" << tr("Device to Device") << ": ";
	if(info.band.copyDD == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.band.copyDD / 1024).arg(tr("MB/s")) << endl;
	out << tr("GPU Core Performance") << endl;
	out << "\t" << tr("Float Point") << ": ";
	if(info.perf.calcFloat == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.perf.calcFloat / 1000).arg(tr("Mflop/s")) << endl;
	out << "\t" << tr("32-bit Integer") << ": ";
	if(info.perf.calcInteger32 == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.perf.calcInteger32 / 1000).arg(tr("Miop/s")) << endl;
	out << "\t" << tr("24-bit Integer") << ": ";
	if(info.perf.calcInteger24 == 0)
		out << "--" << endl;
	else
		out << QString("%1 %2").arg((double)info.perf.calcInteger24 / 1000).arg(tr("Miop/s")) << endl;
	out << endl;

	time_t t;
	time(&t);
	out << QString("%1: %2").arg(tr("Generated")).arg(ctime(&t)) << endl;
}

/*!
	\brief Export information to HTML file.
*/
void CZDialog::slotExportToHTML() {

	struct CZDeviceInfo info = deviceList[comboDevice->currentIndex()]->info();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text as..."),
		tr("%1.html").arg(tr(CZ_NAME_SHORT)), tr("HTML files (*.html *.htm);;All files (*.*)"));

	if(fileName.isEmpty())
		return;

	qDebug() << "Export to HTML as" << fileName;

	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(CZ_NAME_SHORT),
			tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	QString title = tr("%1 Report").arg(tr(CZ_NAME_SHORT));

	out << 	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"mul\" lang=\"mul\" dir=\"ltr\">\n"
		"<head>\n"
		"<title>" << title << "</title>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
		"<style type=\"text/css\">\n"

		"@charset \"utf-8\";\n"
		"body { font-size: 12px; font-family: Verdana, Arial, Helvetica, sans-serif; font-weight: normal; font-style: normal; }\n"
		"h1 { font-size: 15px; color: #690; }\n"
		"h2 { font-size: 13px; color: #690; }\n"
		"table { border-collapse: collapse; border: 1px solid #000; width: 500px; }\n"
		"th { background-color: #deb; text-align: left; }\n"
		"td { width: 50%; }\n"
		"a:link { color: #9c3; text-decoration: none; }\n"
		"a:visited { color: #690; text-decoration: none; }\n"
		"a:hover { color: #9c3; text-decoration: underline; }\n"
		"a:active { color: #9c3; text-decoration: underline; }\n"

		"</style>\n"
		"</head>\n"
		"<body style=\"background: #fff;\">\n";

	out << "<h1>" << title << "</h1>\n";
	out << "<p><small>";
	out << tr("<b>Version:</b> %1").arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	out << tr(" <b>Built</b> %1 %2 ").arg(CZ_DATE).arg(CZ_TIME);
#endif//CZ_VER_STATE
	out << QString("<a href=\"%1\">%1</a><br/>\n").arg(CZ_ORG_URL_MAINPAGE);
	out << tr("<b>OS Version:</b> %1<br/>").arg(getOSVersion());
	out << "</small></p>\n";

	out << 	"<h2>" << tr("Core Information") << "</h2>\n"
		"<table border=\"1\">\n"
		"<tr><th>" << tr("Name") << "</th><td>" << info.deviceName << "</td></tr>\n"
		"<tr><th>" << tr("Compute Capability") << "</th><td>" << info.major << "." << info.minor << "</td></tr>\n"
		"<tr><th>" << tr("Clock Rate") << "</th><td>" << (double)info.core.clockRate / 1000 << " " << tr("MHz") << "</td></tr>\n"
		"<tr><th>" << tr("Wrap Size") << "</th><td>" << info.core.SIMDWidth << "</td></tr>\n"
		"<tr><th>" << tr("Regs Per Block") << "</th><td>" << info.core.regsPerBlock << "</td></tr>\n"
		"<tr><th>" << tr("Threads Per Block") << "</th><td>" << info.core.maxThreadsPerBlock << "</td></tr>\n"
		"<tr><th>" << tr("Threads Dimentions") << "</th><td>" << info.core.maxThreadsDim[0] << " x " << info.core.maxThreadsDim[1] << " x " << info.core.maxThreadsDim[2] << "</td></tr>\n"
		"<tr><th>" << tr("Grid Dimentions") << "</th><td>" << info.core.maxGridSize[0] << " x " << info.core.maxGridSize[1] << " x " << info.core.maxGridSize[2] << "</td></tr>\n"
		"</table>\n";

	out << 	"<h2>" << tr("Memory Information") << "</h2>\n"
		"<table border=\"1\">\n"
		"<tr><th>" << tr("Total Global") << "</th><td>" << (double)info.mem.totalGlobal / (1024 * 1024) << " " << tr("MB") << "</td></tr>\n"
		"<tr><th>" << tr("Shared Per Block") << "</th><td>" << (double)info.mem.sharedPerBlock / 1024 << " " << tr("KB") << "</td></tr>\n"
		"<tr><th>" << tr("Pitch") << "</th><td>" << (double)info.mem.maxPitch / 1024 << " " << tr("KB") << "</td></tr>\n"
		"<tr><th>" << tr("Total Constant") << "</th><td>" << (double)info.mem.totalConst / 1024 << " " << tr("KB") << "</td></tr>\n"
		"<tr><th>" << tr("Texture Alignment") << "</th><td>" << info.mem.textureAlignment << "</td></tr>\n"
		"<tr><th>" << tr("GPU Overlap") << "</th><td>" << (info.mem.gpuOverlap? tr("Yes"): tr("No")) << "</td></tr>\n"
		"</table>\n";

	out << 	"<h2>" << tr("Performance Information") << "</h2>\n"
		"<table border=\"1\">\n"
		"<tr><th colspan=\"2\">" << tr("Memory Copy") << "</th></tr>\n"
		"<tr><th>" << tr("Host Pinned to Device") << "</th><td>";
		if(info.band.copyHDPin == 0)
			out << "--";
		else
			out << (double)info.band.copyHDPin / 1024 << " " << tr("MB/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("Host Pageable to Device") << "</th><td>";
		if(info.band.copyHDPage == 0)
			out << "--";
		else
			out << (double)info.band.copyHDPage / 1024 << " " << tr("MB/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("Device to Host Pinned") << "</th><td>";
		if(info.band.copyDHPin == 0)
			out << "--";
		else
			out << (double)info.band.copyDHPin / 1024 << " " << tr("MB/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("Device to Host Pageable") << "</th><td>";
		if(info.band.copyDHPage == 0)
			out << "--";
		else
			out << (double)info.band.copyDHPage / 1024 << " " << tr("MB/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("Device to Device") << "</th><td>";
		if(info.band.copyDD == 0)
			out << "--";
		else
			out << (double)info.band.copyDD / 1024 << " " << tr("MB/s");
		out << "</td></tr>\n"
		"<tr><th colspan=\"2\">" << tr("GPU Core Performance") << "</th></tr>\n"
		"<tr><th>" << tr("Float Point") << "</th><td>";
		if(info.perf.calcFloat == 0)
			out << "--";
		else
			out << (double)info.perf.calcFloat / 1000 << " " << tr("Mflop/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("32-bit Integer") << "</th><td>";
		if(info.perf.calcInteger32 == 0)
			out << "--";
		else
			out << (double)info.perf.calcInteger32 / 1000 << " " << tr("Miop/s");
		out << "</td></tr>\n"
		"<tr><th>" << tr("24-bit Integer") << "</th><td>";
		if(info.perf.calcInteger24 == 0)
			out << "--";
		else
			out << (double)info.perf.calcInteger24 / 1000 << " " << tr("Miop/s");
		out << "</td></tr>\n"
		"</table>\n";

	time_t t;
	time(&t);
	out <<	"<p><small><b>" << tr("Generated") << ":</b> " << ctime(&t) << "</small></p>\n";

	out <<	"</body>\n"
		"</html>\n";
}

/*!
	\brief Start version reading procedure.
*/
void CZDialog::startGetHistoryHttp() {

	if(http == NULL) {
		http = new QHttp(this);

		connect(http, SIGNAL(done(bool)), this, SLOT(slotGetHistoryDone(bool)));
		connect(http, SIGNAL(stateChanged(int)), this, SLOT(slotGetHistoryStateChanged(int)));

		http->setHost(CZ_ORG_DOMAIN);
		http->get("/history.txt");
	}

}

/*!
	\brief Clean up after version reading procedure.
*/
void CZDialog::cleanGetHistoryHttp() {

	if(http != NULL) {
		disconnect(http, SIGNAL(done(bool)), this, SLOT(slotGetHistoryDone(bool)));
		disconnect(http, SIGNAL(stateChanged(int)), this, SLOT(slotGetHistoryStateChanged(int)));

		delete http;
		http = NULL;
	}
}

/*!
	\brief HTTP operation result slot.
*/
void CZDialog::slotGetHistoryDone(
	bool error			/*!< HTTP operation error state. */
) {
	if(error) {
		qDebug() << "Get version request done with error" << http->error() << http->errorString();

		labelAppUpdate->setText(tr("Can't load version information.\n") + http->errorString());
	} else {
		qDebug() << "Get version request done successfully";

		QString history(http->readAll().data());
		history.remove('\r');
		QStringList historyStrings(history.split("\n"));

		for(int i = 0; i < historyStrings.size(); i++) {
			qDebug() << i << historyStrings[i];
		}

		QString lastVersion;
		QString downloadUrl;
		QString releaseNotes;

		bool validVersion = false;
		QString version;
		QString notes;
		QString url;

		QString nameVersion("version ");
		QString nameNotes("release-notes ");
		QString nameDownload = QString("download-") + CZ_OS_PLATFORM_STR + " ";

		for(int i = 0; i < historyStrings.size(); i++) {

			if(historyStrings[i].left(nameVersion.size()) == nameVersion) {

				if(validVersion) {
					downloadUrl = url;
					releaseNotes = notes;
					lastVersion = version;
				}

				version = historyStrings[i];
				version.remove(0, nameVersion.size());
				qDebug() << "Version found:" << version;
				notes = "";
				url = "";
				validVersion = false;
			}
			if(historyStrings[i].left(nameNotes.size()) == nameNotes) {
				notes = historyStrings[i];
				notes.remove(0, nameNotes.size());
				qDebug() << "Notes found:" << notes;
			}
			if(historyStrings[i].left(nameDownload.size()) == nameDownload) {
				url = historyStrings[i];
				url.remove(0, nameDownload.size());
				qDebug() << "Valid URL found:" << url;
				validVersion = true;
			}
		}

		if(validVersion) {
			downloadUrl = url;
			releaseNotes = notes;
			lastVersion = version;
		}

		qDebug() << "Last valid version:" << lastVersion << releaseNotes << downloadUrl;

		bool haveNewer = false;

		if(!lastVersion.isEmpty()) {

			QStringList versionNumbers = lastVersion.split('.');
			if(versionNumbers[0].toInt() < CZ_VER_MAJOR) {
				haveNewer = false;
			} else if((versionNumbers[0].toInt() == CZ_VER_MAJOR) && (versionNumbers[1].toInt() < CZ_VER_MINOR)) {
				haveNewer = false;
#ifdef CZ_VER_BUILD
			} else if((versionNumbers[0].toInt() == CZ_VER_MAJOR) && (versionNumbers[1].toInt() == CZ_VER_MINOR) && (versionNumbers.size() > 2) && (versionNumbers[2].toInt() == CZ_VER_BUILD)) {
				haveNewer = false;
#endif//CZ_VER_BUILD
			} else {
				haveNewer = true;
			}
		}

		if(haveNewer) {
			QString updateString = QString("%1 <b>%2</b>.")
				.arg(tr("New version is available")).arg(lastVersion);
			if(!downloadUrl.isEmpty()) {
				updateString += QString("<br><a href=\"%1\">%2</a>")
					.arg(downloadUrl)
					.arg(tr("Download"));
			} else {
				updateString += QString("<br><a href=\"%1\">%2</a>")
					.arg(CZ_ORG_URL_MAINPAGE)
					.arg(tr("Main page"));
			}
			if(!releaseNotes.isEmpty()) {
				updateString += QString(" <a href=\"%1\">%2</a>")
					.arg(releaseNotes)
					.arg(tr("Release notes"));
			}
			labelAppUpdate->setText(updateString);
		} else {
			labelAppUpdate->setText(tr("No new version found."));
		}
	}
}

/*!
	\brief HTTP connection state change slot.
*/
void CZDialog::slotGetHistoryStateChanged(
	int state			/*!< Current state of HTTP link. */
) {
	qDebug() << "Get version connection state changed to" << state;

	if(state == QHttp::Unconnected) {
		cleanGetHistoryHttp();
	}
}
