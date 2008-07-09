/*!
	\file czdialog.h
	\brief Main window implementation header file.
	\author AG
*/

#ifndef CZ_DIALOG_H
#define CZ_DIALOG_H

#include <QSplashScreen>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>

#include "ui_czdialog.h"
#include "cudainfo.h"

extern void wait(int n); // implemented in main.cpp
extern QSplashScreen *splash;

class CZUpdateThread: public QThread {
	Q_OBJECT

public:
	CZUpdateThread(QObject *parent = 0);
	~CZUpdateThread();

	void testBandwidth(struct CZDeviceInfo *info, int index);

signals:
	void testedBandwidth(int index);

protected:
	void run();

private:
	struct CZDeviceInfo *info;
	int index;

	QMutex mutex;
	QWaitCondition condition;
	bool restart;
	bool abort;
};

class CZDialog: public QDialog, public Ui::CZDialog {
	Q_OBJECT

public:
	CZDialog(QWidget *parent = 0, Qt::WFlags f = 0);
	~CZDialog();

private:
	QList<struct CZDeviceInfo> deviceList;
	CZUpdateThread *updateThread;
	QTimer *updateTimer;

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
	void slotUpdateBandwidth(int index);
	void slotUpdateTimer();
};

#endif//CZ_DIALOG_H
