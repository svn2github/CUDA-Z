/*!	\file czdialog.cpp
	\brief Main window implementation source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QClipboard>
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

/*!	\def CZ_OS_OLD_PLATFORM_STR
	\brief Old platform ID string.
*/
#if defined(Q_OS_WIN)
#define CZ_OS_OLD_PLATFORM_STR	"win32"
#elif defined(Q_OS_MAC)
#define CZ_OS_OLD_PLATFORM_STR	"macosx"
#elif defined(Q_OS_LINUX)
#define CZ_OS_OLD_PLATFORM_STR	"linux"
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
	m_http = new QHttp(this);
	m_httpId = -1;
	connect(m_http, SIGNAL(requestFinished(int,bool)), this, SLOT(slotHttpRequestFinished(int,bool)));
	connect(m_http, SIGNAL(stateChanged(int)), this, SLOT(slotHttpStateChanged(int)));
#else
#endif /*CZ_USE_QHTTP*/

	setupUi(this);
	this->setWindowTitle(QString("%1 %2 %3 bit").arg(CZ_NAME_SHORT).arg(CZ_VERSION).arg(QString::number(QSysInfo::WordSize)));
	connect(comboDevice, SIGNAL(activated(int)), SLOT(slotShowDevice(int)));

	connect(pushUpdate, SIGNAL(clicked()), SLOT(slotUpdateVersion()));

	QMenu *exportMenu = new QMenu(pushExport);
	exportMenu->addAction(tr("to &Text"), this, SLOT(slotExportToText()));
	exportMenu->addAction(tr("to &HTML"), this, SLOT(slotExportToHTML()));
	exportMenu->addAction(tr("to &Clipboard"), this, SLOT(slotExportToClipboard()));
	pushExport->setMenu(exportMenu);
	
	readCudaDevices();
	setupDeviceList();
	setupDeviceInfo(comboDevice->currentIndex());
	setupAboutTab();

	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, SIGNAL(timeout()), SLOT(slotUpdateTimer()));
	m_updateTimer->start(CZ_TIMER_REFRESH);

	slotUpdateVersion();
}

/*!	\brief Class destructor.
	This function makes class data cleanup actions.
*/
CZDialog::~CZDialog() {
	m_updateTimer->stop();
	delete m_updateTimer;
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

			info->waitPerformance();
			
			connect(info, SIGNAL(testedPerformance(int)), SLOT(slotUpdatePerformance(int)));
			m_deviceList.append(info);
		} else {
			delete info;
		}
	}
}

/*!	\brief Cleans up after bandwidth tests.
*/
void CZDialog::freeCudaDevices() {

	while(m_deviceList.size() > 0) {
		CZCudaDeviceInfo *info = m_deviceList[0];
		m_deviceList.removeFirst();
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

	for(int i = 0; i < m_deviceList.size(); i++) {
		comboDevice->addItem(QString("%1: %2").arg(i).arg(m_deviceList[i]->info().deviceName));
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
		m_deviceList[index]->testPerformance(index);
	}
}

/*!	\brief This slot updates performance information of device
	pointed by \a index.
*/
void CZDialog::slotUpdatePerformance(
	int index			/*!<[in] Index of device in list. */
) {
	if(index == comboDevice->currentIndex())
	setupPerformanceTab(m_deviceList[index]->info());
}

/*!	\brief This slot updates performance information of current device
	every timer tick.
*/
void CZDialog::slotUpdateTimer() {

	int index = comboDevice->currentIndex();
	if(checkUpdateResults->checkState() == Qt::Checked) {
		if(checkHeavyMode->checkState() == Qt::Checked) {
			m_deviceList[index]->info().heavyMode = 1;
		} else {
			m_deviceList[index]->info().heavyMode = 0;
		}
		CZLog(CZLogLevelModerate, "Timer shot -> update performance for device %d in mode %d", index, m_deviceList[index]->info().heavyMode);
		m_deviceList[index]->testPerformance(index);
	} else {
		CZLog(CZLogLevelModerate, "Timer shot -> update ignored");
	}
}

/*!	\brief Places in dialog's tabs information about given device.
*/
void CZDialog::setupDeviceInfo(
	int dev				/*!<[in] Number/index of CUDA-device. */
) {
	setupCoreTab(m_deviceList[dev]->info());
	setupMemoryTab(m_deviceList[dev]->info());
	setupPerformanceTab(m_deviceList[dev]->info());
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
	if(deviceName.contains("tegra", Qt::CaseInsensitive)) {
		labelDeviceLogo->setPixmap(QPixmap(":/img/logo-tegra.png"));
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

	if(info.perf.calcInteger64 == 0)
		labelInt64RateText->setText("--");
	else
		labelInt64RateText->setText(getValue1000(info.perf.calcInteger64, prefixKilo, tr("iop/s")));

	if(info.perf.calcInteger32 == 0)
		labelInt32RateText->setText("--");
	else
		labelInt32RateText->setText(getValue1000(info.perf.calcInteger32, prefixKilo, tr("iop/s")));

	if(info.perf.calcInteger24 == 0)
		labelInt24RateText->setText("--");
	else
		labelInt24RateText->setText(getValue1000(info.perf.calcInteger24, prefixKilo, tr("iop/s")));
}

/*!	\brief Get C/C++ compiler name string
	\hint This code is taken from Qt Creator code
	creator-3.3.0/src/plugins/coreplugin/icore.cpp
	\return C/C++ compiler string
*/
static QString compilerString() {
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
	QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
	isAppleString = QLatin1String(" (Apple)");
#endif
	return QLatin1String("Clang ") + QString::number(__clang_major__) + QLatin1Char('.')
		+ QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
	return QLatin1String("GCC ") + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
	if(_MSC_VER >= 1800) // 1800: MSVC 2013 (yearly release cycle)
		return QLatin1String("MSVC ") + QString::number(2008 + ((_MSC_VER / 100) - 13));
	if(_MSC_VER >= 1500) // 1500: MSVC 2008, 1600: MSVC 2010, ... (2-year release cycle)
		return QLatin1String("MSVC ") + QString::number(2008 + 2 * ((_MSC_VER / 100) - 15));
#endif
	return QLatin1String("<unknown compiler>");
}

/*!	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
	labelAppName->setText(QString("<b><font size=\"+2\">%1</font></b>")
		.arg(CZ_NAME_LONG));

	QString version = QString("<b>%1</b> %2 %3 bit").arg(tr("Version")).arg(CZ_VERSION).arg(QString::number(QSysInfo::WordSize));
#ifdef CZ_VER_STATE
	version += QString("<br /><b>%1</b> %2 %3").arg(tr("Built")).arg(CZ_DATE).arg(CZ_TIME);
	version += tr("<br /><b>%1</b> %2 %3").arg(tr("Based on Qt")).arg(QT_VERSION_STR).arg(compilerString());
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

QString CZDialog::getPlatformString() {
	QString platformString = "win32";

	if(QSysInfo::WordSize == 64)
		platformString = "win64";

	return platformString;
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

QString CZDialog::getPlatformString() {
	QString platformString = "linux";

	if(QSysInfo::WordSize == 64)
		platformString = "linux64";

	return platformString;
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

QString CZDialog::getPlatformString() {
	QString platformString = "macosx";

	if(QSysInfo::WordSize == 64)
		platformString = "macosx64";

	return platformString;
}

#else
#error Functions getOSVersion() and getPlatformString() are not implemented for your platform!
#endif//Q_OS_WIN

#define CZ_TXT_EXPORT(label)	out += label->text() + ": " + label ## Text->text() + "\n"
#define CZ_TXT_EXPORT_TAB(label)	out += "\t" + label->text() + ": " + label ## Text->text() + "\n"
#define CZ_TXT_EXPORT_TAB_TITLE(title, label)	out += "\t" + tr(title) + ": " + label ## Text->text() + "\n"

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

	QTextStream stream(&file);
	stream << generateTextReport();
}

/*!	\brief Export information to clipboard as a plane text.
*/
void CZDialog::slotExportToClipboard() {

	QClipboard *clipboard = QApplication::clipboard();

	clipboard->setText(generateTextReport());
}

/*!	\brief Generate plane text report.
*/
QString CZDialog::generateTextReport() {

	QString out;
	QString title = tr(CZ_NAME_SHORT " Report");
	QString subtitle;

	out += title;
	out += "\n";
	for(int i = 0; i < title.size(); i++)
		out += "=";
	out += "\n";
	out += tr("Version") + ": " CZ_VERSION + " " + QString::number(QSysInfo::WordSize) + " bit";
#ifdef CZ_VER_STATE
	out += " " + tr("Built") + " " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out += " " CZ_ORG_URL_MAINPAGE "\n";
	out += tr("OS Version") + ": " + getOSVersion() + "\n";

	CZ_TXT_EXPORT(labelDrvVersion);
	CZ_TXT_EXPORT(labelDrvDllVersion);
	CZ_TXT_EXPORT(labelRtDllVersion);
	out += "\n";

	subtitle = tr("Core Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
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
	out += "\n";

	subtitle = tr("Memory Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
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
	out += "\n";

	subtitle = tr("Performance Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
	out += tr("Memory Copy") + "\n";
	CZ_TXT_EXPORT_TAB_TITLE("Host Pinned to Device", labelHDRatePin);
	CZ_TXT_EXPORT_TAB_TITLE("Host Pageable to Device", labelHDRatePage);
	CZ_TXT_EXPORT_TAB_TITLE("Device to Host Pinned", labelDHRatePin);
	CZ_TXT_EXPORT_TAB_TITLE("Device to Host Pageable", labelDHRatePage);
	CZ_TXT_EXPORT_TAB(labelDDRate);
	out += tr("GPU Core Performance") + "\n";
	CZ_TXT_EXPORT_TAB(labelFloatRate);
	CZ_TXT_EXPORT_TAB(labelDoubleRate);
	CZ_TXT_EXPORT_TAB(labelInt64Rate);
	CZ_TXT_EXPORT_TAB(labelInt32Rate);
	CZ_TXT_EXPORT_TAB(labelInt24Rate);
	out += "\n";

	time_t t;
	time(&t);
	out += QString("%1: %2").arg(tr("Generated")).arg(ctime(&t)) + "\n";

	return out;
}

#define CZ_HTML_EXPORT(label)	out += "<b>" + label->text() + "</b>: " + label ## Text->text() + "<br/>\n"
#define CZ_HTML_EXPORT_TAB(label)	out += "<tr><th>" + label->text() + "</th><td>" + label ## Text->text() + "</td></tr>\n"
#define CZ_HTML_EXPORT_TAB_TITLE(title, label)	out += "<tr><th>" + tr(title) + "</th><td>" + label ## Text->text() + "</td></tr>\n"

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

	QTextStream stream(&file);
	stream << generateHTMLReport();
}

/*!	\brief Generate HTML v4 report.
	\todo Use HTML v5 insteand of v4.
*/
QString CZDialog::generateHTMLReport() {

	QString out;
	QString title = tr(CZ_NAME_SHORT " Report");

	out += "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"mul\" lang=\"mul\" dir=\"ltr\">\n"
		"<head>\n"
		"<title>" + title + "</title>\n"
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

	out += "<h1>" + title + "</h1>\n";
	out += "<p><small>";
	out += "<b>" + tr("Version")+ ":</b> " CZ_VERSION " " + QString::number(QSysInfo::WordSize) + " bit";
#ifdef CZ_VER_STATE
	out += " <b>" + tr("Built") + "</b> " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out += " <a href=\"" CZ_ORG_URL_MAINPAGE "\">" CZ_ORG_URL_MAINPAGE "</a><br/>\n";
	out += "<b>" + tr("OS Version") + ":</b> " + getOSVersion() + "<br/>\n";

	CZ_HTML_EXPORT(labelDrvVersion);
	CZ_HTML_EXPORT(labelDrvDllVersion);
	CZ_HTML_EXPORT(labelRtDllVersion);
	out += "</small></p>\n";

	out += "<h2>" + tr("Core Information") + "</h2>\n";
	out += "<table border=\"1\">\n";
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
	out += "</table>\n";

	out += "<h2>" + tr("Memory Information") + "</h2>\n";
	out += "<table border=\"1\">\n";
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
	out += "</table>\n";

	out += "<h2>" + tr("Performance Information") + "</h2>\n";
	out += "<table border=\"1\">\n";
	out += "<tr><th colspan=\"2\">" + tr("Memory Copy") + "</th></tr>\n";
	CZ_HTML_EXPORT_TAB_TITLE("Host Pinned to Device", labelHDRatePin);
	CZ_HTML_EXPORT_TAB_TITLE("Host Pageable to Device", labelHDRatePage);
	CZ_HTML_EXPORT_TAB_TITLE("Device to Host Pinned", labelDHRatePin);
	CZ_HTML_EXPORT_TAB_TITLE("Device to Host Pageable", labelDHRatePage);
	CZ_HTML_EXPORT_TAB(labelDDRate);
	out += "<tr><th colspan=\"2\">" + tr("GPU Core Performance") + "</th></tr>\n";
	CZ_HTML_EXPORT_TAB(labelFloatRate);
	CZ_HTML_EXPORT_TAB(labelDoubleRate);
	CZ_HTML_EXPORT_TAB(labelInt64Rate);
	CZ_HTML_EXPORT_TAB(labelInt32Rate);
	CZ_HTML_EXPORT_TAB(labelInt24Rate);
	out += "</table>\n";

	time_t t;
	time(&t);
	out +=	"<p><small><b>" + tr("Generated") + ":</b> " + ctime(&t) + "</small></p>\n";

	out +=	"<p><a href=\"http://cuda-z.sourceforge.net/\"><img src=\"http://cuda-z.sourceforge.net/img/web-button.png\" border=\"0\" alt=\"CUDA-Z\" title=\"CUDA-Z\" /></a></p>\n";

	out +=	"</body>\n"
		"</html>\n";

	return out;
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
	m_url = QString(CZ_ORG_URL_MAINPAGE) + "history.txt";
	startHttpRequest(m_url);
}

/*!	\brief Start a HTTP request with a given \a url.
*/
void CZDialog::startHttpRequest(
	QUrl url			/*!<[in] URL to be read out. */
) {
	m_history.clear();
	CZLog(CZLogLevelLow, "Requesting %s!", url.toString().toLocal8Bit().data());
#ifdef CZ_USE_QHTTP
	QHttp::ConnectionMode mode = (url.scheme() == "https")? QHttp::ConnectionModeHttps: QHttp::ConnectionModeHttp;
	quint16 port = (url.port() < 0)? 0: url.port();
	m_http->setHost(url.host(), mode, port);
	m_httpId = m_http->get(url.path());
#else
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", (QString("Mozilla/5.0 (%1) " CZ_NAME_SHORT " " CZ_VERSION " %2 bit %3").arg(getOSVersion()).arg(QString::number(QSysInfo::WordSize).arg(getPlatformString())).toLocal8Bit()));
	request.setRawHeader("Accept-Encoding", "gzip, deflate");
	m_reply = m_qnam.get(request);
	connect(m_reply, SIGNAL(finished()), this, SLOT(slotHttpFinished()));
	connect(m_reply, SIGNAL(readyRead()), this, SLOT(slotHttpReadyRead()));
#endif /*CZ_USE_QHTTP*/
}

/*!	\brief Clean up after version reading procedure.
*/
void CZDialog::cleanGetHistoryHttp() {

#ifdef CZ_USE_QHTTP
	m_http->abort();
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
	if(id != m_httpId)
		return;

	QString errorString;

	if(error || (!m_http->lastResponse().isValid())) {
		errorString = m_http->errorString();
report_error:
		CZLog(CZLogLevelWarning, "Get version request done with error: %s", errorString.toLocal8Bit().data());
		labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_ERROR));
		labelAppUpdate->setText(tr("Can't load version information. ") + errorString);
		pushUpdate->setEnabled(true);
	} else {
		int statusCode = m_http->lastResponse().statusCode();

		if(statusCode == 200) { /* Ok */
			CZLog(CZLogLevelModerate, "Get version request done successfully");
			m_history = m_http->readAll().data();
			parseHistoryTxt(m_history);
			pushUpdate->setEnabled(true);
		} else if((statusCode >= 300) && (statusCode < 400)) { /* Redirect */
			if(!m_http->lastResponse().hasKey("Location")) {
				CZLog(CZLogLevelModerate, "Can't redirect - no location.");
				errorString = QString("%1 %2 %3.").arg(tr("Error")).arg(m_http->lastResponse().statusCode()).arg(tr("Can't redirect"));
				goto report_error;
			} else {
				m_url = m_http->lastResponse().value("Location");
				CZLog(CZLogLevelModerate, "Get version redirected to %s", m_url.toString().toLocal8Bit().data());
				startHttpRequest(m_url);
			}
		} else { /* Error */
			errorString = QString("%1 %2.").arg(tr("Error")).arg(m_http->lastResponse().statusCode());
			goto report_error;
		}
	}
}

#else
/*!	\brief HTTP requst status processing slot.
*/
void CZDialog::slotHttpFinished() {

	QVariant redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if(m_reply->error()) {
		QString errorString = m_reply->errorString();
		CZLog(CZLogLevelWarning, "Get version request done with error: %s", errorString.toLocal8Bit().data());
		labelAppUpdateImg->setPixmap(QPixmap(CZ_UPD_ICON_ERROR));
		labelAppUpdate->setText(tr("Can't load version information.") + " " + errorString);
		m_reply->deleteLater();
		pushUpdate->setEnabled(true);
	} else if(!redirectionTarget.isNull()) {
		QUrl url = m_url.resolved(redirectionTarget.toUrl());
		CZLog(CZLogLevelModerate, "Get version redirected to %s", url.toString().toLocal8Bit().data());
		m_reply->deleteLater();
		startHttpRequest(url);
	} else {
		CZLog(CZLogLevelModerate, "Get version request done successfully");
		parseHistoryTxt(m_history);
		m_reply->deleteLater();
		pushUpdate->setEnabled(true);
	}

}

/*!	\brief HTTP data processing slot.
*/
void CZDialog::slotHttpReadyRead() {
	CZLog(CZLogLevelLow, "Get potrion of data %d", m_reply->size());
	m_history += m_reply->readAll().data();
//	CZLog(CZLogLevelLow, "history: %s", m_history.toLocal8Bit().data());
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

	static const QString nameVersion("version ");
	static const QString nameNotes("release-notes ");
	static const QString nameCritical("release-critical");
	static const QString nameOldDownload("download-" CZ_OS_OLD_PLATFORM_STR " ");
	static const QString nameDownload("download-" + getPlatformString() + " ");

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

		if(historyStrings[i].left(nameOldDownload.size()) == nameOldDownload) {
			url = historyStrings[i];
			url.remove(0, nameOldDownload.size());
			CZLog(CZLogLevelLow, "Valid old URL found: %s", url.toLocal8Bit().data());
			validVersion = true;
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
