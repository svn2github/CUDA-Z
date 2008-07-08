/*!
	\file czdialog.h
	\brief Main window implementation header file.
	\author AG
*/

#ifndef CZ_DIALOG_H
#define CZ_DIALOG_H

#include <QSplashScreen>

#include "ui_czdialog.h"
#include "cudainfo.h"

extern void wait(int n); // implemented in main.cpp
extern QSplashScreen *splash;

class CZDialog: public QDialog, public Ui::CZDialog {
	Q_OBJECT

public:
	CZDialog(QWidget *parent = 0, Qt::WFlags f = 0);
	~CZDialog();

private:
	QList<struct CZDeviceInfo> deviceList;

	void readCudaDevices();
	void freeCudaDevices();
	int getCudaDeviceNumber();
	int readCudaDeviceInfo(struct CZDeviceInfo &info, int dev);
	int calcCudaDeviceBandwidth(struct CZDeviceInfo &info);

	void setupDeviceList();
	void setupDeviceInfo(int dev);

	void setupCoreTab(struct CZDeviceInfo &info);
	void setupMemoryTab(struct CZDeviceInfo &info);
	void setupBandwidthTab(struct CZDeviceInfo &info);

	void setupAboutTab();

private slots:
	void slotShowDevice(int index);
};

#endif//CZ_DIALOG_H
