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
#include "czdeviceinfodecoder.h"
#include "platform.h"
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
	\returns number of lines in log.
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

	m_index = 0;
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
	\returns number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
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
	m_index = index;
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

#define CZ_DLG_FILL(decoder, _id_) \
	{ \
		label ## _id_->setText(decoder.getName(CZCudaDeviceInfoDecoder::id ## _id_)); \
		label ## _id_ ## Text->setText(decoder.getValue(CZCudaDeviceInfoDecoder::id ## _id_)); \
	}

/*!	\brief Fill tab "Core" with CUDA devices information.
*/
void CZDialog::setupCoreTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {
	CZCudaDeviceInfoDecoder decoder(info);

	CZ_DLG_FILL(decoder, DrvVersion);
	CZ_DLG_FILL(decoder, DrvDllVersion);
	CZ_DLG_FILL(decoder, RtDllVersion);
	CZ_DLG_FILL(decoder, Name);
	CZ_DLG_FILL(decoder, Capability);
	CZ_DLG_FILL(decoder, Clock);
	CZ_DLG_FILL(decoder, PCIInfo);
	CZ_DLG_FILL(decoder, MultiProc);
	CZ_DLG_FILL(decoder, ThreadsMulti);
	CZ_DLG_FILL(decoder, Warp);
	CZ_DLG_FILL(decoder, RegsBlock);
	CZ_DLG_FILL(decoder, ThreadsBlock);
	CZ_DLG_FILL(decoder, ThreadsDim);
	CZ_DLG_FILL(decoder, GridDim);
	CZ_DLG_FILL(decoder, Watchdog);
	CZ_DLG_FILL(decoder, Integrated);
	CZ_DLG_FILL(decoder, ConcurrentKernels);
	CZ_DLG_FILL(decoder, ComputeMode);
	CZ_DLG_FILL(decoder, StreamPriorities);
}

/*!	\brief Fill tab "Memory" with CUDA devices information.
*/
void CZDialog::setupMemoryTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {
	CZCudaDeviceInfoDecoder decoder(info);

	CZ_DLG_FILL(decoder, TotalGlobal);
	CZ_DLG_FILL(decoder, BusWidth);
	CZ_DLG_FILL(decoder, MemClock);
	CZ_DLG_FILL(decoder, ErrorCorrection);
	CZ_DLG_FILL(decoder, L2CasheSize);
	CZ_DLG_FILL(decoder, Shared);
	CZ_DLG_FILL(decoder, Pitch);
	CZ_DLG_FILL(decoder, TotalConst);
	CZ_DLG_FILL(decoder, TextureAlign);
	CZ_DLG_FILL(decoder, Texture1D);
	CZ_DLG_FILL(decoder, Texture2D);
	CZ_DLG_FILL(decoder, Texture3D);
	CZ_DLG_FILL(decoder, GpuOverlap);
	CZ_DLG_FILL(decoder, MapHostMemory);
	CZ_DLG_FILL(decoder, UnifiedAddressing);
	CZ_DLG_FILL(decoder, AsyncEngine);
}

/*!	\brief Fill tab "Performance" with CUDA devices information.
*/
void CZDialog::setupPerformanceTab(
	struct CZDeviceInfo &info	/*!<[in] Information about CUDA-device. */
) {
	CZCudaDeviceInfoDecoder decoder(info);

	labelHDRatePinText->setText(decoder.getValue(CZCudaDeviceInfoDecoder::idHostPinnedToDevice));
	labelHDRatePageText->setText(decoder.getValue(CZCudaDeviceInfoDecoder::idHostPageableToDevice));
	labelDHRatePinText->setText(decoder.getValue(CZCudaDeviceInfoDecoder::idDeviceToHostPinned));
	labelDHRatePageText->setText(decoder.getValue(CZCudaDeviceInfoDecoder::idDeviceToHostPageable));
	labelDDRateText->setText(decoder.getValue(CZCudaDeviceInfoDecoder::idDeviceToDevice));
	CZ_DLG_FILL(decoder, FloatRate);
	CZ_DLG_FILL(decoder, DoubleRate);
	CZ_DLG_FILL(decoder, Int64Rate);
	CZ_DLG_FILL(decoder, Int32Rate);
	CZ_DLG_FILL(decoder, Int24Rate);
}

/*!	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
	labelAppName->setText(QString("<b><font size=\"+2\">%1</font></b>")
		.arg(CZ_NAME_LONG));

	QString version = QString("<b>%1</b> %2 %3 bit").arg(tr("Version")).arg(CZ_VERSION).arg(QString::number(QSysInfo::WordSize));
#ifdef CZ_VER_STATE
	version += QString("<br /><b>%1</b> %2 %3").arg(tr("Built")).arg(CZ_DATE).arg(CZ_TIME);
	version += tr("<br /><b>%1</b> %2 %3").arg(tr("Based on Qt")).arg(QT_VERSION_STR).arg(getCompilerVersion());
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
	CZCudaDeviceInfoDecoder decoder(m_deviceList[m_index]->info());
	stream << decoder.generateTextReport();
}

/*!	\brief Export information to clipboard as a plane text.
*/
void CZDialog::slotExportToClipboard() {

	QClipboard *clipboard = QApplication::clipboard();

	CZCudaDeviceInfoDecoder decoder(m_deviceList[m_index]->info());
	clipboard->setText(decoder.generateTextReport());
}

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
	CZCudaDeviceInfoDecoder decoder(m_deviceList[m_index]->info());
	stream << decoder.generateHTMLReport();
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
