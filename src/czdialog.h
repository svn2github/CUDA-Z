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

class CZUpdateThread;

class CZCudaDeviceInfo: public QObject {
	Q_OBJECT

public:
	CZCudaDeviceInfo(int devNum, QObject *parent = 0);
	~CZCudaDeviceInfo();

	int readInfo();
	int prepareDevice();
	int updateInfo();
	int cleanDevice();

	struct CZDeviceInfo &info();
	CZUpdateThread *thread();

private:
	struct CZDeviceInfo _info;
	CZUpdateThread *_thread;
};

class CZUpdateThread: public QThread {
	Q_OBJECT

public:
	CZUpdateThread(CZCudaDeviceInfo *info, QObject *parent = 0);
	~CZUpdateThread();

	void testPerformance(int index);

signals:
	void testedPerformance(int index);

protected:
	void run();

private:
	QMutex mutex;
	QWaitCondition condition;
	CZCudaDeviceInfo *info;

	int index;
	bool abort;
};

class CZDialog: public QDialog, public Ui::CZDialog {
	Q_OBJECT

public:
	CZDialog(QWidget *parent = 0, Qt::WFlags f = 0);
	~CZDialog();

private:
	QList<CZCudaDeviceInfo*> deviceList;
	QTimer *updateTimer;

	void readCudaDevices();
	void freeCudaDevices();
	int getCudaDeviceNumber();

	void setupDeviceList();
	void setupDeviceInfo(int dev);

	void setupCoreTab(struct CZDeviceInfo &info);
	void setupMemoryTab(struct CZDeviceInfo &info);
	void setupPerformanceTab(struct CZDeviceInfo &info);

	void setupAboutTab();

private slots:
	void slotShowDevice(int index);
	void slotUpdatePerformance(int index);
	void slotUpdateTimer();
	void slotExportToText();
	void slotExportToHTML();
};

#endif//CZ_DIALOG_H
