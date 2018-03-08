/*!	\file czdeviceinfo.h
	\brief CUDA device information header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_DEVICEINFO_H
#define CZ_DEVICEINFO_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "cudainfo.h"

class CZCudaDeviceInfo;

class CZUpdateThread: public QThread {
	Q_OBJECT

public:
	CZUpdateThread(CZCudaDeviceInfo *info, QObject *parent = 0);
	~CZUpdateThread();

	void testPerformance(int index);
	void waitPerformance();

signals:
	void testedPerformance(int index);

protected:
	void run();

private:
	QMutex m_mutex;
	QWaitCondition m_newLoop;
	QWaitCondition m_readyForWork;
	QWaitCondition m_testStart;
	QWaitCondition m_testFinish;
	CZCudaDeviceInfo *m_info;

	int m_index;
	bool m_abort;
	bool m_deviceReady;
	bool m_testRunning;
};

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

	void testPerformance(int index);
	void waitPerformance();

signals:
	void testedPerformance(int index);

private:
	struct CZDeviceInfo m_info;
	CZUpdateThread *m_thread;
};

#endif//CZ_DEVICEINFO_H
