/*!	\file czdialog.cpp
	\brief Main window implementation source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://ag.embedded.org.ru/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#define QDesktopServices QStandardPaths
#define storageLocation StandardLocation
#endif//QT_VERSION

#include <time.h>

#include "log.h"
#include "czdialog.h"
#include "version.h"

/*!	\def CZ_USE_QHTTP
	\brief Use QHttp code or QNetworkAccessManager code.
*/

#define CZ_TIMER_REFRESH	2000	/*!< Test results update timer period (ms). */

/*!	\def CZ_OS_PLATFORM_STR
	\brief Platform ID string.
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

/*!	\name Update progress icons definitions.
*/
/*@{*/
#define CZ_UPD_ICON_INFO	":/img/upd-info.png"
#define CZ_UPD_ICON_WARNING	":/img/upd-warning.png"
#define CZ_UPD_ICON_ERROR	":/img/upd-error.png"
#define CZ_UPD_ICON_DOWNLOAD	":/img/upd-download.png"
#define CZ_UPD_ICON_DOWNLOAD_CR	":/img/upd-download-critical.png"
/*@}*/

/*!	\class CZSplashScreen
	\brief Splash screen with multiline logging effect.
*/

/*!	\brief Creates a new #CZSplashScreen and initializes internal
	parameters of the class.
*/
CZSplashScreen::CZSplashScreen(
	const QPixmap &pixmap,	/*!<[in] Picture for window background. */
	int maxLines,		/*!<[in] Number of lines in boot log. */
	Qt::WindowFlags f	/*!<[in] Window flags. */
):	QSplashScreen(pixmap, f),
	m_maxLines(maxLines) {
	m_message = QString::null;
	m_lines = 0;
	m_alignment = Qt::AlignLeft;
	m_color = Qt::black;
}

/*!	\brief Creates a new #CZSplashScreen with the given \a parent and
	initializes internal parameters of the class.
*/
CZSplashScreen::CZSplashScreen(
	QWidget *parent,	/*!<[in,out] Parent of widget. */
	const QPixmap &pixmap,	/*!<[in] Picture for window background. */
	int maxLines,		/*!<[in] Number of lines in boot log. */
	Qt::WindowFlags f	/*!<[in] Window flags. */
):	QSplashScreen(parent, pixmap, f),
	m_maxLines(maxLines) {
	m_message = QString::null;
	m_lines = 0;
	m_alignment = Qt::AlignLeft;
	m_color = Qt::black;
}

/*!	\brief Class destructor.
*/
CZSplashScreen::~CZSplashScreen() {
}

/*!	\brief Sets the maximal number of lines in log.
*/
void CZSplashScreen::setMaxLines(
	int maxLines		/*!<[in] Number of lines in log. */
) {
	if(maxLines >= 1) {
		m_maxLines = maxLines;
		if(m_lines > m_maxLines) {
			deleteTop(m_lines - m_maxLines);
			QSplashScreen::showMessage(m_message, m_alignment, m_color);
		}
	}
}

/*!	\brief Returns the maximal number of lines in log.
	\return number of lines in log.
*/
int CZSplashScreen::maxLines() {
	return m_maxLines;
}

/*!	\brief Adds new message line in log.
*/
void CZSplashScreen::showMessage(
	const QString &message,	/*!<[in] Message text. */
	int alignment,		/*!<[in] Placement of log in window. */
	const QColor &color	/*!<[in] Color used for protocol display. */
) {

	m_alignment = alignment;
	m_color = color;

	if(m_message.size() != 0) {
		m_message += '\n' + message;
	} else {
		m_message = message;
	}
	QStringList linesList = m_message.split('\n');
	m_lines = linesList.size();

	if(m_lines > m_maxLines) {
		deleteTop(m_lines - m_maxLines);
	}

	QSplashScreen::showMessage(m_message, m_alignment, m_color);
}

/*!	\brief Removes all messages being displayed in log.
*/
void CZSplashScreen::clearMessage() {
	m_message = QString::null;
	m_lines = 0;
	QSplashScreen::showMessage(m_message, m_alignment, m_color);
}

/*!	\brief Removes first \a lines entries in log.
*/
void CZSplashScreen::deleteTop(
	int lines		/*!<[in] Number of lines to be removed. */
) {
	QStringList linesList = m_message.split('\n');
	for(int i = 0; i < lines; i++) {
		linesList.removeFirst();
	}

	m_message = linesList.join(QString('\n'));
	m_lines -= lines;
}

/*!	\brief Splash screen of application.
*/
CZSplashScreen *splash;

/*!	\class CZDialog
	\brief This class implements main window of the application.
*/

/*!	\brief Creates a new #CZDialog with the given \a parent.
	This function does following steps:
	- Sets up GUI.
	- Setup CUDA-device information containers and add them in list.
	- Sets up connections.
	- Fills up data in to tabs of GUI.
	- Starts Performance data update timer.
*/
CZDialog::CZDialog(
	QWidget *parent,	/*!<[in,out] Parent of widget. */
	Qt::WindowFlags f	/*!<[in] Window flags. */
)	: QDialog(parent, f /*| Qt::MSWindowsFixedSizeDialogHint*/ | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint) {

#ifdef CZ_USE_QHTTP
	http = new QHttp(this);
	httpId = -1;
	connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(slotHttpRequestFinished(int,bool)));
	connect(http, SIGNAL(stateChanged(int)), this, SLOT(slotHttpStateChanged(int)));
#else
#endif /*CZ_USE_QHTTP*/

	setupUi(this);
	this->setWindowTitle(QString("%1 %2").arg(CZ_NAME_SHORT).arg(CZ_VERSION));
	connect(comboDevice, SIGNAL(activated(int)), SLOT(slotShowDevice(int)));

	connect(pushUpdate, SIGNAL(clicked()), SLOT(slotUpdateVersion()));

	QMenu *exportMenu = new QMenu(pushExport);
	exportMenu->addAction(tr("to &Text"), this, SLOT(slotExportToText()));
	exportMenu->addAction(tr("to &HTML"), this, SLOT(slotExportToHTML()));
	pushExport->setMenu(exportMenu);
	
	readCudaDevices();
	setupDeviceList();
	setupDeviceInfo(comboDevice->currentIndex());
	setupAboutTab();

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), SLOT(slotUpdateTimer()));
	updateTimer->start(CZ_TIMER_REFRESH);

	slotUpdateVersion();
}

/*!	\brief Class destructor.
	This function makes class data cleanup actions.
*/
CZDialog::~CZDialog() {
	updateTimer->stop();
	delete updateTimer;
	freeCudaDevices();
	cleanGetHistoryHttp();
}

/*!	\brief Reads CUDA devices information.
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
		} else {
			delete info;
		}
	}
}

/*!	\brief Cleans up after bandwidth tests.
*/
void CZDialog::freeCudaDevices() {

	while(deviceList.size() > 0) {
		CZCudaDeviceInfo *info = deviceList[0];
		deviceList.removeFirst();
		delete info;
	}
}

/*!	\brief Gets number of CUDA devices.
	\return number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
*/
int CZDialog::getCudaDeviceNumber() {
	return CZCudaDeviceFound();
}

/*!	\brief Puts devices in combo box.
*/
void CZDialog::setupDeviceList() {
	comboDevice->clear();

	for(int i = 0; i < deviceList.size(); i++) {
		comboDevice->addItem(QString("%1: %2").arg(i).arg(deviceList[i]->info().deviceName));
	}
}

/*!	\brief This slot shows in dialog information about given device.
*/
void CZDialog::slotShowDevice(
	int index			/*!<[in] Index of device in list. */
) {
	setupDeviceInfo(index);
	if(checkUpdateResults->checkState() == Qt::Checked) {
		CZLog(CZLogLevelModerate, "Switch device -> update performance for device %d", index);
		deviceList[index]->testPerformance(index);
	}
}

/*!	\brief This slot updates performance information of device
	pointed by \a index.
*/
void CZDialog::slotUpdatePerformance(
	int index			/*!<[in] Index of device in list. */
) {
	if(index == comboDevice->currentIndex())
	setupPerformanceTab(deviceList[index]->info());
}

/*!	\brief This slot updates performance information of current device
	every timer tick.
*/
void CZDialog::slotUpdateTimer() {

	int index = comboDevice->currentIndex();
	if(checkUpdateResults->checkState() == Qt::Checked) {
		if(checkHeavyMode->checkState() == Qt::Checked) {
			deviceList[index]->info().heavyMode = 1;
		} else {
			deviceList[index]->info().heavyMode = 0;
		}
		CZLog(CZLogLevelModerate, "Timer shot -> update performance for device %d in mode %d", index, deviceList[index]->info().heavyMode);
		deviceList[index]->testPerformance(index);
	} else {
		CZLog(CZLogLevelModerate, "Timer shot -> update ignored");
	}
}

/*!	\brief Places in dialog's tabs information about given device.
*/
void CZDialog::setupDeviceInfo(
	int dev				/*!<[in] Number/index of CUDA-device. */
) {
	setupCoreTab(deviceList[dev]->info());
	setupMemoryTab(deviceList[dev]->info());
	setupPerformanceTab(deviceList[dev]->info());
}

/*!	\brief Fill tab "Core" with CUDA devices information.
*/
void CZDialog::setupCoreTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {
	QString deviceName(info.deviceName);

	labelNameText->setText(deviceName);
	labelCapabilityText->setText(QString("%1.%2").arg(info.major).arg(info.minor));
	labelClockText->setText(getValue1000(info.core.clockRate, prefixKilo, tr("Hz")));
	if(info.core.muliProcCount == 0)
		labelMultiProcText->setText(tr("Unknown"));
	else {
		if(info.core.cudaCores == 0)
			labelMultiProcText->setNum(info.core.muliProcCount);
		else
			labelMultiProcText->setText(tr("%1 (%2 Cores)")
				.arg(info.core.muliProcCount)
				.arg(info.core.cudaCores));
	}
	labelThreadsMultiText->setNum(info.core.maxThreadsPerMultiProcessor);
	labelWarpText->setNum(info.core.SIMDWidth);
	labelRegsBlockText->setNum(info.core.regsPerBlock);
	labelThreadsBlockText->setNum(info.core.maxThreadsPerBlock);
	if(info.core.watchdogEnabled == -1)
		labelWatchdogText->setText(tr("Unknown"));
	else
		labelWatchdogText->setText(info.core.watchdogEnabled? tr("Yes"): tr("No"));
	labelIntegratedText->setText(info.core.integratedGpu? tr("Yes"): tr("No"));
	labelConcurrentKernelsText->setText(info.core.concurrentKernels? tr("Yes"): tr("No"));
	if(info.core.computeMode == CZComputeModeDefault) {
		labelComputeModeText->setText(tr("Default"));
	} else if(info.core.computeMode == CZComputeModeExclusive) {
		labelComputeModeText->setText(tr("Compute-exclusive"));
	} else if(info.core.computeMode == CZComputeModeProhibited) {
		labelComputeModeText->setText(tr("Compute-prohibited"));
	} else {
		labelComputeModeText->setText(tr("Unknown"));
	}
	labelStreamPrioritiesText->setText(info.core.streamPrioritiesSupported? tr("Yes"): tr("No"));

	labelThreadsDimText->setText(QString("%1 x %2 x %3").arg(info.core.maxThreadsDim[0]).arg(info.core.maxThreadsDim[1]).arg(info.core.maxThreadsDim[2]));
	labelGridDimText->setText(QString("%1 x %2 x %3").arg(info.core.maxGridSize[0]).arg(info.core.maxGridSize[1]).arg(info.core.maxGridSize[2]));

	labelDeviceLogo->setPixmap(QPixmap(":/img/logo-unknown.png"));
	if(deviceName.contains("tesla", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-tesla.png"));
	} else
	if(deviceName.contains("quadro", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-quadro.png"));
	} else
	if(deviceName.contains("ion", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-ion.png"));
	} else
	if(deviceName.contains("geforce", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-geforce.png"));
	}

	labelPCIInfoText->setText(tr("%1:%2:%3")
		.arg(info.core.pciDomainID)
		.arg(info.core.pciBusID)
		.arg(info.core.pciDeviceID));

	QString version;
	if(strlen(info.drvVersion) != 0) {
		version = QString(info.drvVersion);
		version += info.tccDriver? " (TCC)": "";
	} else {
		version = tr("Unknown");
	}
	labelDrvVersionText->setText(version);

	if(info.drvDllVer == 0) {
		version = tr("Unknown");
	} else {
		version = QString("%1.%2")
			.arg(info.drvDllVer / 1000)
			.arg(info.drvDllVer % 1000);
	}
	if(strlen(info.drvDllVerStr) != 0) {
		version += " (" + QString(info.drvDllVerStr) + ")";
	}
	labelDrvDllVersionText->setText(version);

	if(info.rtDllVer == 0) {
		version = tr("Unknown");
	} else {
		version = QString("%1.%2")
			.arg(info.rtDllVer / 1000)
			.arg(info.rtDllVer % 1000);
	}
	if(strlen(info.rtDllVerStr) != 0) {
		version += " (" + QString(info.rtDllVerStr) + ")";
	}
	labelRtDllVersionText->setText(version);
}

/*!	\brief Fill tab "Memory" with CUDA devices information.
*/
void CZDialog::setupMemoryTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {
	labelTotalGlobalText->setText(getValue1024(info.mem.totalGlobal, prefixNothing, tr("B")));
	labelBusWidthText->setText(tr("%1 bits").arg(info.mem.memoryBusWidth));
	labelMemClockText->setText(getValue1000(info.mem.memoryClockRate, prefixKilo, tr("Hz")));
	labelErrorCorrectionText->setText(info.mem.errorCorrection? tr("Yes"): tr("No"));
	labelL2CasheSizeText->setText(info.mem.l2CacheSize?getValue1024(info.mem.sharedPerBlock, prefixNothing, tr("B")): tr("No"));
	labelSharedText->setText(getValue1024(info.mem.sharedPerBlock, prefixNothing, tr("B")));
	labelPitchText->setText(getValue1024(info.mem.maxPitch, prefixNothing, tr("B")));
	labelTotalConstText->setText(getValue1024(info.mem.totalConst, prefixNothing, tr("B")));
	labelTextureAlignText->setText(getValue1024(info.mem.textureAlignment, prefixNothing, tr("B")));
	labelTexture1DText->setText(QString("%1")
		.arg((double)info.mem.texture1D[0]));
	labelTexture2DText->setText(QString("%1 x %2")
		.arg((double)info.mem.texture2D[0])
		.arg((double)info.mem.texture2D[1]));
	labelTexture3DText->setText(QString("%1 x %2 x %3")
		.arg((double)info.mem.texture3D[0])
		.arg((double)info.mem.texture3D[1])
		.arg((double)info.mem.texture3D[2]));
	labelGpuOverlapText->setText(info.mem.gpuOverlap? tr("Yes"): tr("No"));
	labelMapHostMemoryText->setText(info.mem.mapHostMemory? tr("Yes"): tr("No"));
	labelUnifiedAddressingText->setText(info.mem.unifiedAddressing? tr("Yes"): tr("No"));
	labelAsyncEngineText->setText(
		(info.mem.asyncEngineCount == 2)? tr("Yes, Bidirectional"):
		(info.mem.asyncEngineCount == 1)? tr("Yes, Unidirectional"):
		tr("No"));
}

/*!	\brief Fill tab "Performance" with CUDA devices information.
*/
void CZDialog::setupPerformanceTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {

	if(info.band.copyHDPin == 0)
		labelHDRatePinText->setText("--");
	else
		labelHDRatePinText->setText(getValue1024(info.band.copyHDPin, prefixKibi, tr("B/s")));

	if(info.band.copyHDPage == 0)
		labelHDRatePageText->setText("--");
	else
		labelHDRatePageText->setText(getValue1024(info.band.copyHDPage, prefixKibi, tr("B/s")));

	if(info.band.copyDHPin == 0)
		labelDHRatePinText->setText("--");
	else
		labelDHRatePinText->setText(getValue1024(info.band.copyDHPin, prefixKibi, tr("B/s")));

	if(info.band.copyDHPage == 0)
		labelDHRatePageText->setText("--");
	else
		labelDHRatePageText->setText(getValue1024(info.band.copyDHPage, prefixKibi, tr("B/s")));

	if(info.band.copyDD == 0)
		labelDDRateText->setText("--");
	else
		labelDDRateText->setText(getValue1024(info.band.copyDD, prefixKibi, tr("B/s")));

	if(info.perf.calcFloat == 0)
		labelFloatRateText->setText("--");
	else
		labelFloatRateText->setText(getValue1000(info.perf.calcFloat, prefixKilo, tr("flop/s")));

	if(((info.major > 1)) ||
		((info.major == 1) && (info.minor >= 3))) {
		if(info.perf.calcDouble == 0)
			labelDoubleRateText->setText("--");
		else
			labelDoubleRateText->setText(getValue1000(info.perf.calcDouble, prefixKilo, tr("flop/s")));
	} else {
		labelDoubleRateText->setText(tr("Not Supported"));
	}

	if(info.perf.calcInteger32 == 0)
		labelInt32RateText->setText("--");
	else
		labelInt32RateText->setText(getValue1000(info.perf.calcInteger32, prefixKilo, tr("iop/s")));

	if(info.perf.calcInteger24 == 0)
		labelInt24RateText->setText("--");
	else
		labelInt24RateText->setText(getValue1000(info.perf.calcInteger24, prefixKilo, tr("iop/s")));
}

/*!	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
	labelAppName->setText(QString("<b><font size=\"+2\">%1</font></b>")
		.arg(CZ_NAME_LONG));

	QString version = QString("<b>%1</b> %2").arg(tr("Version")).arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	version += QString("<br /><b>%1</b> %2 %3").arg(tr("Built")).arg(CZ_DATE).arg(CZ_TIME);
	version += tr("<br /><b>%1</b> %2").arg(tr("Based on Qt")).arg(QT_VERSION_STR);
#ifdef CZ_VER_BUILD_URL
	version += tr("<br /><b>%1</b> %2:%3").arg(tr("SVN URL")).arg(CZ_VER_BUILD_URL).arg(CZ_VER_BUILD_STRING);
#endif//CZ_VER_BUILD_URL
#endif//CZ_VER_STATE
	labelAppVersion->setText(version);
	labelAppURL->setText(QString("<b>%1:</b> <a href=\"%2\">%2</a><br /><b>%3:</b> <a href=\"%4\">%4</a>")
		.arg(tr("Main page")).arg(CZ_ORG_URL_MAINPAGE)
		.arg(tr("Project page")).arg(CZ_ORG_URL_PROJECT));
	labelAppAuthor->setText(QString("<b>%1</b> %2").arg(tr("Author")).arg(CZ_ORG_NAME));
	labelAppCopy->setText(QString("%1 <a href=\"%2\">%2</a>").arg(CZ_COPY_INFO).arg(CZ_COPY_URL));
}

/*!	\fn CZDialog::getOSVersion
	\brief Get OS version string.
	\return string that describes version of OS we running at.
*/
#if defined(Q_OS_WIN)
#include <windows.h>
typedef BOOL (WINAPI *IsWow64Process_t)(HANDLE, PBOOL);

QString CZDialog::getOSVersion() {
	QString OSVersion = "Windows";

	BOOL is_os64bit = FALSE;
	IsWow64Process_t p_IsWow64Process = (IsWow64Process_t)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if(p_IsWow64Process != NULL) {
		if(!p_IsWow64Process(GetCurrentProcess(), &is_os64bit)) {
			is_os64bit = FALSE;
	        }
	}

	OSVersion += QString(" %1").arg(
		(is_os64bit == TRUE)? "AMD64": "x86");

/*	GetSystemInfo(&systemInfo);
	OSVersion += QString(" %1").arg(
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)? "AMD64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)? "IA64":
		(systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)? "x86":
		"Unknown architecture");*/

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
#include <QProcess>
QString CZDialog::getOSVersion() {
	QProcess uname; 

	uname.start("uname", QStringList() << "-srvm");
	if(!uname.waitForFinished())
		return QString("Linux (unknown)");
	QString OSVersion = uname.readLine();

	return OSVersion.remove('\n');
}
#elif defined (Q_OS_MAC)
#include "plist.h"
QString CZDialog::getOSVersion() {
	char osName[256];
	char osVersion[256];
	char osBuild[256];

	if((CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductName", osName, sizeof(osName)) != 0) ||
	   (CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductUserVisibleVersion", osVersion, sizeof(osVersion)) != 0) ||
	   (CZPlistGet("/System/Library/CoreServices/SystemVersion.plist", "ProductBuildVersion", osBuild, sizeof(osBuild)) != 0))
	   return QString("Mac OS X (unknown)");
	QString OSVersion = QString(osName) + " " + QString(osVersion) + " " + QString(osBuild);
	
	return OSVersion.remove('\n');
}
#else//!Q_WS_WIN
#error Function getOSVersion() is not implemented for your platform!
#endif//Q_WS_WIN

#define CZ_TXT_EXPORT(label)	out << label->text() << ": " << label ## Text->text() << endl
#define CZ_TXT_EXPORT_TAB(label)	out << "\t" << label->text() << ": " << label ## Text->text() << endl
#define CZ_TXT_EXPORT_TAB_TITLE(title, label)	out << "\t" << tr(title) << ": " << label ## Text->text() << endl

/*!	\brief Export information to plane text file.
*/
void CZDialog::slotExportToText() {

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text Report as..."),
		QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + tr("%1.txt").arg(tr(CZ_NAME_SHORT)),
		tr("Text files (*.txt);;All files (*.*)"));

	if(fileName.isEmpty())
		return;

	CZLog(CZLogLevelModerate, "Export to text as %s", fileName.toLocal8Bit().data());

	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(CZ_NAME_SHORT),
			tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	QString title = tr(CZ_NAME_SHORT " Report");
	QString subtitle;

	out << title << endl;
	for(int i = 0; i < title.size(); i++)
		out << "=";
	out << endl;
	out << tr("Version") << ": " CZ_VERSION;
#ifdef CZ_VER_STATE
	out << " " << tr("Built") << " " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out << " " CZ_ORG_URL_MAINPAGE << endl;
	out << tr("OS Version") << ": " << getOSVersion() << endl;

	CZ_TXT_EXPORT(labelDrvVersion);
	CZ_TXT_EXPORT(labelDrvDllVersion);
	CZ_TXT_EXPORT(labelRtDllVersion);
	out << endl;

	subtitle = tr("Core Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	CZ_TXT_EXPORT_TAB(labelName);
	CZ_TXT_EXPORT_TAB(labelCapability);
	CZ_TXT_EXPORT_TAB(labelClock);
	CZ_TXT_EXPORT_TAB(labelPCIInfo);
	CZ_TXT_EXPORT_TAB(labelMultiProc);
	CZ_TXT_EXPORT_TAB(labelThreadsMulti);
	CZ_TXT_EXPORT_TAB(labelWarp);
	CZ_TXT_EXPORT_TAB(labelRegsBlock);
	CZ_TXT_EXPORT_TAB(labelThreadsBlock);
	CZ_TXT_EXPORT_TAB(labelThreadsDim);
	CZ_TXT_EXPORT_TAB(labelGridDim);
	CZ_TXT_EXPORT_TAB(labelWatchdog);
	CZ_TXT_EXPORT_TAB(labelIntegrated);
	CZ_TXT_EXPORT_TAB(labelConcurrentKernels);
	CZ_TXT_EXPORT_TAB(labelComputeMode);
	CZ_TXT_EXPORT_TAB(labelStreamPriorities);
	out << endl;

	subtitle = tr("Memory Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	CZ_TXT_EXPORT_TAB(labelTotalGlobal);
	CZ_TXT_EXPORT_TAB(labelBusWidth);
	CZ_TXT_EXPORT_TAB(labelMemClock);
	CZ_TXT_EXPORT_TAB(labelErrorCorrection);
	CZ_TXT_EXPORT_TAB(labelL2CasheSize);
	CZ_TXT_EXPORT_TAB(labelShared);
	CZ_TXT_EXPORT_TAB(labelPitch);
	CZ_TXT_EXPORT_TAB(labelTotalConst);
	CZ_TXT_EXPORT_TAB(labelTextureAlign);
	CZ_TXT_EXPORT_TAB(labelTexture1D);
	CZ_TXT_EXPORT_TAB(labelTexture2D);
	CZ_TXT_EXPORT_TAB(labelTexture3D);
	CZ_TXT_EXPORT_TAB(labelGpuOverlap);
	CZ_TXT_EXPORT_TAB(labelMapHostMemory);
	CZ_TXT_EXPORT_TAB(labelUnifiedAddressing);
	CZ_TXT_EXPORT_TAB(labelAsyncEngine);
	out << endl;

	subtitle = tr("Performance Information");
	out << subtitle << endl;
	for(int i = 0; i < subtitle.size(); i++)
		out << "-";
	out << endl;
	out << tr("Memory Copy") << endl;
	CZ_TXT_EXPORT_TAB_TITLE("Host Pinned to Device", labelHDRatePin);
	CZ_TXT_EXPORT_TAB_TITLE("Host Pageable to Device", labelHDRatePage);
	CZ_TXT_EXPORT_TAB_TITLE("Device to Host Pinned", labelDHRatePin);
	CZ_TXT_EXPORT_TAB_TITLE("Device to Host Pageable", labelDHRatePage);
	CZ_TXT_EXPORT_TAB(labelDDRate);
	out << tr("GPU Core Performance") << endl;
	CZ_TXT_EXPORT_TAB(labelFloatRate);
	CZ_TXT_EXPORT_TAB(labelDoubleRate);
	CZ_TXT_EXPORT_TAB(labelInt32Rate);
	CZ_TXT_EXPORT_TAB(labelInt24Rate);
	out << endl;

	time_t t;
	time(&t);
	out << QString("%1: %2").arg(tr("Generated")).arg(ctime(&t)) << endl;
}
#define CZ_HTML_EXPORT(label)	out << "<b>" << label->text() << "</b>: " << label ## Text->text() << "<br/>" << endl
#define CZ_HTML_EXPORT_TAB(label)	out << "<tr><th>" << label->text() << "</th><td>" << label ## Text->text() << "</td></tr>" << endl
#define CZ_HTML_EXPORT_TAB_TITLE(title, label)	out << "<tr><th>" << tr(title) << "</th><td>" << label ## Text->text() << "</td></tr>" << endl

/*!	\brief Export information to HTML file.
*/
void CZDialog::slotExportToHTML() {

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save HTML Report as..."),
		QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + tr("%1.html").arg(tr(CZ_NAME_SHORT)),
		tr("HTML files (*.html *.htm);;All files (*.*)"));

	if(fileName.isEmpty())
		return;

	CZLog(CZLogLevelModerate, "Export to HTML as %s", fileName.toLocal8Bit().data());

	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(CZ_NAME_SHORT),
			tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	QString title = tr(CZ_NAME_SHORT " Report");

	out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
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
	out << "<b>" << tr("Version")<< ":</b> " CZ_VERSION;
#ifdef CZ_VER_STATE
	out << " <b>" << tr("Built") << "</b> " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out << " <a href=\"" CZ_ORG_URL_MAINPAGE "\">" CZ_ORG_URL_MAINPAGE "</a><br/>" << endl;
	out << "<b>" << tr("OS Version") << ":</b> " << getOSVersion() << "<br/>" << endl;

	CZ_HTML_EXPORT(labelDrvVersion);
	CZ_HTML_EXPORT(labelDrvDllVersion);
	CZ_HTML_EXPORT(labelRtDllVersion);
	out << "</small></p>" << endl;

	out << "<h2>" << tr("Core Information") << "</h2>" << endl;
	out << "<table border=\"1\">" << endl;
	CZ_HTML_EXPORT_TAB(labelName);
	CZ_HTML_EXPORT_TAB(labelCapability);
	CZ_HTML_EXPORT_TAB(labelClock);
	CZ_HTML_EXPORT_TAB(labelPCIInfo);
	CZ_HTML_EXPORT_TAB(labelMultiProc);
	CZ_HTML_EXPORT_TAB(labelThreadsMulti);
	CZ_HTML_EXPORT_TAB(labelWarp);
	CZ_HTML_EXPORT_TAB(labelRegsBlock);
	CZ_HTML_EXPORT_TAB(labelThreadsBlock);
	CZ_HTML_EXPORT_TAB(labelThreadsDim);
	CZ_HTML_EXPORT_TAB(labelGridDim);
	CZ_HTML_EXPORT_TAB(labelWatchdog);
	CZ_HTML_EXPORT_TAB(labelIntegrated);
	CZ_HTML_EXPORT_TAB(labelConcurrentKernels);
	CZ_HTML_EXPORT_TAB(labelComputeMode);
	CZ_HTML_EXPORT_TAB(labelStreamPriorities);
	out << "</table>" << endl;

	out << "<h2>" << tr("Memory Information") << "</h2>" << endl;
	out << "<table border=\"1\">" << endl;
	CZ_HTML_EXPORT_TAB(labelTotalGlobal);
	CZ_HTML_EXPORT_TAB(labelBusWidth);
	CZ_HTML_EXPORT_TAB(labelMemClock);
	CZ_HTML_EXPORT_TAB(labelErrorCorrection);
	CZ_HTML_EXPORT_TAB(labelL2CasheSize);
	CZ_HTML_EXPORT_TAB(labelShared);
	CZ_HTML_EXPORT_TAB(labelPitch);
	CZ_HTML_EXPORT_TAB(labelTotalConst);
	CZ_HTML_EXPORT_TAB(labelTextureAlign);
	CZ_HTML_EXPORT_TAB(labelTexture1D);
	CZ_HTML_EXPORT_TAB(labelTexture2D);
	CZ_HTML_EXPORT_TAB(labelTexture3D);
	CZ_HTML_EXPORT_TAB(labelGpuOverlap);
	CZ_HTML_EXPORT_TAB(labelMapHostMemory);
	CZ_HTML_EXPORT_TAB(labelUnifiedAddressing);
	CZ_HTML_EXPORT_TAB(labelAsyncEngine);
	out << "</table>" << endl;

	out << "<h2>" << tr("Performance Information") << "</h2>" << endl;
	out << "<table border=\"1\">" << endl;
	out << "<tr><th colspan=\"2\">" << tr("Memory Copy") << "</th></tr>" << endl;
	CZ_HTML_EXPORT_TAB_TITLE("Host Pinned to Device", labelHDRatePin);
	CZ_HTML_EXPORT_TAB_TITLE("Host Pageable to Device", labelHDRatePage);
	CZ_HTML_EXPORT_TAB_TITLE("Device to Host Pinned", labelDHRatePin);
	CZ_HTML_EXPORT_TAB_TITLE("Device to Host Pageable", labelDHRatePage);
	CZ_HTML_EXPORT_TAB(labelDDRate);
	out << "<tr><th colspan=\"2\">" << tr("GPU Core Performance") << "</th></tr>" << endl;
	CZ_HTML_EXPORT_TAB(labelFloatRate);
	CZ_HTML_EXPORT_TAB(labelDoubleRate);
	CZ_HTML_EXPORT_TAB(labelInt32Rate);
	CZ_HTML_EXPORT_TAB(labelInt24Rate);
	out << "</table>" << endl;

	time_t t;
	time(&t);
	out <<	"<p><small><b>" << tr("Generated") << ":</b> " << ctime(&t) << "</small></p>" << endl;

	out <<	"<p><a href=\"http://cuda-z.sourceforge.net/\"><img src=\"http://cuda-z.sourceforge.net/img/web-button.png\" border=\"0\" alt=\"CUDA-Z\" title=\"CUDA-Z\" /></a></p>\n";

	out <<	"</body>\n"
		"</html>\n";
}

/*!	\brief Resend a version request.
*/
void CZDialog::slotUpdateVersion() {
	startGetHistoryHttp();
}

/*!	\brief Start version reading procedure.
*/
void CZDialog::startGetHistoryHttp() {

	labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_INFO));
	labelAppUpdate->setText(tr("Looking for new version..."));
	pushUpdate->setEnabled(false);
	url = QString(CZ_ORG_URL_MAINPAGE) + "/history.txt";
	startHttpRequest(url);
}

/*!	\brief Start a HTTP request with a given \a url.
*/
void CZDialog::startHttpRequest(
	QUrl url			/*!<[in] URL to be read out. */
) {
	history.clear();
	CZLog(CZLogLevelLow, "Requesting %s!", url.toString().toLocal8Bit().data());
#ifdef CZ_USE_QHTTP
	QHttp::ConnectionMode mode = url.scheme() == "https"? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
	quint16 port = url.port() < 0 ? 0 : url.port();
	http->setHost(url.host(), mode, port);
	httpId = http->get(url.path());
#else
	reply = qnam.get(QNetworkRequest(url));
	connect(reply, SIGNAL(finished()), this, SLOT(slotHttpFinished()));
	connect(reply, SIGNAL(readyRead()), this, SLOT(slotHttpReadyRead()));
#endif /*CZ_USE_QHTTP*/
}

/*!	\brief Clean up after version reading procedure.
*/
void CZDialog::cleanGetHistoryHttp() {

#ifdef CZ_USE_QHTTP
	http->abort();
#endif /*CZ_USE_QHTTP*/

}

#ifdef CZ_USE_QHTTP
/*!	\brief HTTP connection state change slot.
*/
void CZDialog::slotHttpStateChanged(
	int state			/*!< Current state of HTTP link. */
) {
	CZLog(CZLogLevelLow, "Get version connection state changed to %d", state);

	if(state == QHttp::Unconnected) {
		CZLog(CZLogLevelLow, "Disconnected!");
	}
}

/*!	\brief HTTP operation result slot.
*/
void CZDialog::slotHttpRequestFinished(
	int id,				/*!<[in] HTTP request ID. */
	bool error			/*!<[in] HTTP operation error state. */
) {
	if(id != httpId)
		return;

	pushUpdate->setEnabled(true);

	QString errorString;

	if(error || (!http->lastResponse().isValid())) {
		errorString = http->errorString();
report_error:
		CZLog(CZLogLevelWarning, "Get version request done with error: %s", errorString.toLocal8Bit().data());
		labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_ERROR));
		labelAppUpdate->setText(tr("Can't load version information. ") + errorString);
	} else {
		int statusCode = http->lastResponse().statusCode();

		if(statusCode == 200) { /* Ok */
			CZLog(CZLogLevelModerate, "Get version request done successfully");
			history = http->readAll().data();
			parseHistoryTxt(history);

		} else if((statusCode >= 300) && (statusCode < 400)) { /* Redirect */
			if(!http->lastResponse().hasKey("Location")) {
				CZLog(CZLogLevelModerate, "Can't redirect - no location.");
				errorString = QString("%1 %2 %3.").arg(tr("Error")).arg(http->lastResponse().statusCode()).arg(tr("Can't redirect"));
				goto report_error;
			} else {
				url = http->lastResponse().value("Location");
				CZLog(CZLogLevelModerate, "Get version redirected to %s", url.toString().toLocal8Bit().data());
				startHttpRequest(url);
			}
		} else { /* Error */
			errorString = QString("%1 %2.").arg(tr("Error")).arg(http->lastResponse().statusCode());
			goto report_error;
		}
	}
}

#else
/*!	\brief HTTP requst status processing slot.
*/
void CZDialog::slotHttpFinished() {

	pushUpdate->setEnabled(true);

	QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if(reply->error()) {
		QString errorString;
		errorString = QString("%1 %2.").arg(tr("Error")).arg(reply->errorString());
		CZLog(CZLogLevelWarning, "Get version request done with error: %s", errorString.toLocal8Bit().data());
		labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_ERROR));
		labelAppUpdate->setText(tr("Can't load version information. ") + errorString);

	} else if (!redirectionTarget.isNull()) {
		url = url.resolved(redirectionTarget.toUrl());
		CZLog(CZLogLevelModerate, "Get version redirected to %s", url.toString().toLocal8Bit().data());
		startHttpRequest(url);

	} else {
		CZLog(CZLogLevelModerate, "Get version request done successfully");
		parseHistoryTxt(history);
	}
}

/*!	\brief HTTP data processing slot.
*/
void CZDialog::slotHttpReadyRead() {
	CZLog(CZLogLevelLow, "Get potrion of data %d", reply->size());
	history += reply->readAll().data();
}
#endif /*CZ_USE_QHTTP*/

/*!	\brief Parse \a history.txt received over HTTP.
*/
void CZDialog::parseHistoryTxt(
	QString history			/*!<[in] history.txt as a string. */
) {

	history.remove('\r');
	QStringList historyStrings(history.split("\n"));

	for(int i = 0; i < historyStrings.size(); i++) {
		CZLog(CZLogLevelLow, "%3d %s", i, historyStrings[i].toLocal8Bit().data());
	}

	QString lastVersion;
	QString downloadUrl;
	QString releaseNotes;

	bool validVersion = false;
	QString version;
	QString notes;
	bool criticalVersion = false;
	QString url;

	QString nameVersion("version ");
	QString nameNotes("release-notes ");
	QString nameCritical("release-critical");
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
			CZLog(CZLogLevelLow, "Version found: %s", version.toLocal8Bit().data());
			notes = "";
			url = "";
			criticalVersion = false;
			validVersion = false;
		}

		if(historyStrings[i].left(nameNotes.size()) == nameNotes) {
			notes = historyStrings[i];
			notes.remove(0, nameNotes.size());
			CZLog(CZLogLevelLow, "Notes found: %s", notes.toLocal8Bit().data());
		}

		if(historyStrings[i].left(nameDownload.size()) == nameDownload) {
			url = historyStrings[i];
			url.remove(0, nameDownload.size());
			CZLog(CZLogLevelLow, "Valid URL found: %s", url.toLocal8Bit().data());
			validVersion = true;
		}

		if(historyStrings[i].left(nameCritical.size()) == nameCritical) {
			criticalVersion = true;
			CZLog(CZLogLevelLow, "Version is critical!");
		}
	}

	if(validVersion) {
		downloadUrl = url;
		releaseNotes = notes;
		lastVersion = version;
	}

	CZLog(CZLogLevelModerate, "Last valid version: %s\n%s\n%s",
		lastVersion.toLocal8Bit().data(),
		releaseNotes.toLocal8Bit().data(),
		downloadUrl.toLocal8Bit().data());

	bool isNewest = true;
	bool isNonReleased = false;

	if(!lastVersion.isEmpty()) {

		QStringList versionNumbers = lastVersion.split('.');

		#define GEN_VERSION(major, minor) ((major * 10000) + minor)
		unsigned int myVersion = GEN_VERSION(CZ_VER_MAJOR, CZ_VER_MINOR);
		unsigned int lastVersion = GEN_VERSION(versionNumbers[0].toInt(), versionNumbers[1].toInt());;

		if(myVersion < lastVersion) {
			isNewest = false;
		} else if(myVersion == lastVersion) {
			isNewest = true;
#ifdef CZ_VER_BUILD
			if(CZ_VER_BUILD < versionNumbers[2].toInt()) {
				isNewest = false;
			}
#endif//CZ_VER_BUILD
		} else { // myVersion > lastVersion
			isNonReleased = true;
		}
	}

	if(isNewest) {
		if(isNonReleased) {
			labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_WARNING));
			labelAppUpdate->setText(tr("WARNING: You are running non-released version!"));
		} else {
			labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_INFO));
			labelAppUpdate->setText(tr("No new version was found."));
		}
	} else {
		QString updateString = QString("%1 <b>%2</b>!")
			.arg(tr("New version is available")).arg(lastVersion);
		if(!downloadUrl.isEmpty()) {
			updateString += QString(" <a href=\"%1\">%2</a>")
				.arg(downloadUrl)
				.arg(tr("Download"));
		} else {
			updateString += QString(" <a href=\"%1\">%2</a>")
				.arg(CZ_ORG_URL_MAINPAGE)
				.arg(tr("Main page"));
		}
		if(!releaseNotes.isEmpty()) {
			updateString += QString(" <a href=\"%1\">%2</a>")
				.arg(releaseNotes)
				.arg(tr("Release notes"));
		}
		labelAppUpdateImg->setPixmap(QPixmap((criticalVersion == true)? CZ_UPD_ICON_DOWNLOAD_CR: CZ_UPD_ICON_DOWNLOAD));
		labelAppUpdate->setText(updateString);
	}
}

/*!	\brief This function returns value and unit in SI format.
*/
QString CZDialog::getValue1000(
	double value,			/*!<[in] Value to print. */
	int valuePrefix,		/*!<[in] Value current prefix. */
	QString unitBase		/*!<[in] Unit base string. */
) {
	const int prefixBase = 1000;
	int resPrefix = valuePrefix;

	static const char *prefixTab[prefixSiMax + 1] = {
		"",	/* prefixNothing */
		"k",	/* prefixKilo */
		"M",	/* prefixMega */
		"G",	/* prefixGiga */
		"T",	/* prefixTera */
		"P",	/* prefixPeta */
		"E",	/* prefixExa */
		"Z",	/* prefixZetta */
		"Y",	/* prefixYotta */
	};

	while((value > (10 * prefixBase)) && (resPrefix < prefixSiMax)) {
		value /= prefixBase;
		resPrefix++;
	}

	return QString("%1 %2%3").arg(value).arg(prefixTab[resPrefix]).arg(unitBase);
}

/*!	\brief This function returns value and unit in IEC 60027 format.
*/
QString CZDialog::getValue1024(
	double value,			/*!<[in] Value to print. */
	int valuePrefix,		/*!<[in] Value current prefix. */
	QString unitBase		/*!<[in] Unit base string. */
) {
	const int prefixBase = 1024;
	int resPrefix = valuePrefix;

	static const char *prefixTab[prefixIecMax + 1] = {
		"",	/* prefixNothing */
		"Ki",	/* prefixKibi */
		"Mi",	/* prefixMebi */
		"Gi",	/* prefixGibi */
		"Ti",	/* prefixTebi */
		"Pi",	/* prefixPebi */
		"Ei",	/* prefixExbi */
		"Zi",	/* prefixZebi */
		"Yi",	/* prefixYobi */
	};

	while((value > (10 * prefixBase)) && (resPrefix < prefixIecMax)) {
		value /= prefixBase;
		resPrefix++;
	}

	return QString("%1 %2%3").arg(value).arg(prefixTab[resPrefix]).arg(unitBase);
}
