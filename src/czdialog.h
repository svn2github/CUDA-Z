/*!
	\file czdialog.h
	\brief Main window implementation header file.
	\author AG
*/

#ifndef CZ_DIALOG_H
#define CZ_DIALOG_H

#include <QSplashScreen>
#include <QTimer>
#include <QHttp>

#include "ui_czdialog.h"
#include "czdeviceinfo.h"
#include "cudainfo.h"

extern void wait(int n); // implemented in main.cpp
extern QSplashScreen *splash;

class CZDialog: public QDialog, public Ui::CZDialog {
	Q_OBJECT

public:
	CZDialog(QWidget *parent = 0, Qt::WFlags f = 0);
	~CZDialog();

private:
	QList<CZCudaDeviceInfo*> deviceList;
	QTimer *updateTimer;
	QHttp *http;

	void readCudaDevices();
	void freeCudaDevices();
	int getCudaDeviceNumber();

	void setupDeviceList();
	void setupDeviceInfo(int dev);

	void setupCoreTab(struct CZDeviceInfo &info);
	void setupMemoryTab(struct CZDeviceInfo &info);
	void setupPerformanceTab(struct CZDeviceInfo &info);

	void setupAboutTab();

	QString getOSVersion();

	void startGetHistoryHttp();
	void cleanGetHistoryHttp();

private slots:
	void slotShowDevice(int index);
	void slotUpdatePerformance(int index);
	void slotUpdateTimer();
	void slotExportToText();
	void slotExportToHTML();
	void slotGetHistoryDone(bool error);
};

#endif//CZ_DIALOG_H
