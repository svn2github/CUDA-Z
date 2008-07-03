/*!
	\file czdialog.cpp
	\brief Main window implementation source file.
	\author AG
*/

#include <QDebug>

#include "czdialog.h"
#include "version.h"

QSplashScreen *splash;

/*!
	\class CZDialog
	\brief This class implements main window of the application.
*/

/*!
	\brief Creates a new #CZDialog with the given \a parent.
*/
CZDialog::CZDialog(
	QWidget *parent,
	Qt::WFlags f
)	: QDialog(parent, f /*| Qt::MSWindowsFixedSizeDialogHint*/ | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint) {

	setupUi(this);
	readCudaDevices();

//	qDebug() << "device list size:" << deviceList.size();

	this->setWindowTitle(QString("%1 %2").arg(CZ_NAME_SHORT).arg(CZ_VERSION));
	connect(comboDevice, SIGNAL(activated(int)), SLOT(slotShowDevice(int)));
	
	setupDeviceList();
	setupDeviceInfo(comboDevice->currentIndex());
	setupAboutTab();
}

/*!
	\brief Read CUDA devices information.
*/
void CZDialog::readCudaDevices() {

	int num;

	num = getCudaDeviceNumber();

	for(int i = 0; i < num; i++) {
		struct CZDeviceInfo info;
		if(readCudaDeviceInfo(info, i) == 0) {
			splash->showMessage(tr("Getting information about %1 ...").arg(info.deviceName),
				Qt::AlignLeft | Qt::AlignBottom);
			qApp->processEvents();

//			wait(10000000);
			
//			\todo calcDevicePerformance(info);
			deviceList.append(info);
		}
	}
}

/*!
	\brief Get number of CUDA devices.
*/
int CZDialog::getCudaDeviceNumber() {
	return cudaDeviceFound();
}

/*!
	\brief Get information about CUDA device.
*/
int CZDialog::readCudaDeviceInfo(
	struct CZDeviceInfo &info,
	int dev
) {
	return cudaReadDeviceInfo(&info, dev);
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
void CZDialog::slotShowDevice(int index) {
	setupDeviceInfo(index);
}


/*!
	\brief Place in dialog's tabs information about given device.
*/
void CZDialog::setupDeviceInfo(int dev) {
	setupCoreTab(deviceList[dev]);
	setupMemoryTab(deviceList[dev]);
	setupBandwidthTab(deviceList[dev]);
}

/*!
	\brief Fill tab "Core" with CUDA devices information.
*/
void CZDialog::setupCoreTab(
	struct CZDeviceInfo &info
) {
	QString deviceName(info.deviceName);

	labelNameText->setText(deviceName);
	labelCapabilityText->setText(QString("%1.%2").arg(info.major).arg(info.minor));
	labelClockText->setText(QString(tr("%1 MHz")).arg((double)info.core.clockRate / 1000));
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
	struct CZDeviceInfo &info
) {
	labelTotalGlobalText->setText(QString(tr("%1 MB").arg((double)info.mem.totalGlobal / (1024 * 1024))));
	labelSharedText->setText(QString(tr("%1 KB").arg((double)info.mem.sharedPerBlock / 1024)));
	labelPitchText->setText(QString(tr("%1 KB").arg((double)info.mem.maxPitch / 1024)));
	labelTotalConstText->setText(QString(tr("%1 KB").arg((double)info.mem.totalConst / 1024)));
	labelTextureAlignmentText->setNum(info.mem.textureAlignment);
	labelGpuOverlapText->setText(info.mem.gpuOverlap? tr("Yes"): tr("No"));
}

/*!
	\brief Fill tab "Bandwidth" with CUDA devices information.
*/
void CZDialog::setupBandwidthTab(
	struct CZDeviceInfo &info
) {
	// NIY!
}

/*!
	\brief Fill tab "About" with information about this program.
*/
void CZDialog::setupAboutTab() {
//	labelAppLogo->setPixmap(QPixmap(":/img/icon.png"));
	labelAppName->setText(QString("<b><i><font size=\"+2\">%1</font></i></b><br>%2").arg(CZ_NAME_SHORT).arg(CZ_NAME_LONG));

	QString version = QString(tr("<b>Version</b> %1")).arg(CZ_VERSION);
#ifdef CZ_VER_STATE
	version += QString(tr("<br><b>Built</b> %1 %2").arg(CZ_DATE).arg(CZ_TIME));
#endif//CZ_VER_STATE
	labelAppVersion->setText(version);
	labelAppURL->setText(tr("<b>Mainpage</b> <a href=\"%1\">%1</a><br><b>Project</b> <a href=\"%2\">%2</a>").arg(CZ_ORG_URL_MAINPAGE).arg(CZ_ORG_URL_PROJECT));
	labelAppAuthor->setText(QString(tr("<b>Author</b> %1")).arg(CZ_ORG_NAME));
	labelAppCopy->setText(CZ_COPY_INFO);
}
