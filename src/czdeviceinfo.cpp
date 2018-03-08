/*!	\file czdeviceinfo.cpp
	\brief CUDA device information source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "log.h"
#include "czdeviceinfo.h"

/*!	\class CZUpdateThread
	\brief This class implements performance data update procedure.
*/

/*!	\brief Creates the performance data update thread.
*/
CZUpdateThread::CZUpdateThread(
	CZCudaDeviceInfo *info,		/*!<[in,out] CUDA device information class. */
	QObject *parent			/*!<[in,out] Parent of the thread. */
)	: QThread(parent) {

	m_abort = false;
	m_testRunning = false;
	m_deviceReady = false;
	m_info = info;
	m_index = -1;

	CZLog(CZLogLevelLow, "Thread created");
}

/*!	\brief Terminates the performance data update thread.
	This function waits util performance test will be over.
*/
CZUpdateThread::~CZUpdateThread() {

	m_mutex.lock();
	m_deviceReady = true;
	m_readyForWork.wakeOne();
	m_abort = true;
	m_newLoop.wakeOne();
	m_testRunning = true;
	m_testStart.wakeAll();
	m_testRunning = false;
	m_testFinish.wakeAll();
	m_mutex.unlock();

	wait();

	CZLog(CZLogLevelLow, "Thread is done");
}

/*!	\brief Push performance test.
*/
void CZUpdateThread::testPerformance(
	int index			/*!<[in] Index of device in list. */
) {
	CZLog(CZLogLevelModerate, "Rising update action for device %d", index);

	m_mutex.lock();
	m_index = index;
	if(!isRunning()) {
		start();
		CZLog(CZLogLevelLow, "Waiting for device is ready...");
	}

	while(!m_deviceReady)
		m_readyForWork.wait(&m_mutex);

	m_newLoop.wakeOne();
	m_mutex.unlock();
}

/*!	\brief Wait for performance test results.
*/
void CZUpdateThread::waitPerformance() {

	testPerformance(-1);

	CZLog(CZLogLevelModerate, "Waiting for results...");

	m_mutex.lock();
	CZLog(CZLogLevelLow, "Waiting for beginnig of test...");
	while(!m_testRunning)
		m_testStart.wait(&m_mutex);
	CZLog(CZLogLevelLow, "Waiting for end of test...");
	while(m_testRunning)
		m_testFinish.wait(&m_mutex);
	m_mutex.unlock();

	CZLog(CZLogLevelModerate, "Got results!");
}

/*!	\brief Main work function of the thread.
*/
void CZUpdateThread::run() {

	CZLog(CZLogLevelLow, "Thread started");

	m_info->prepareDevice();

	m_mutex.lock();
	m_deviceReady = true;
	m_readyForWork.wakeAll();

	forever {
		int index;

		CZLog(CZLogLevelLow, "Waiting for new loop...");
		m_newLoop.wait(&m_mutex);
		index = m_index;
		m_mutex.unlock();

		CZLog(CZLogLevelLow, "Thread loop started");

		if(m_abort) {
			m_mutex.lock();
			break;
		}

		m_mutex.lock();
		m_testRunning = true;
		m_testStart.wakeAll();
		m_mutex.unlock();

		m_info->updateInfo();

		m_mutex.lock();
		m_testRunning = false;
		m_testFinish.wakeAll();
		m_mutex.unlock();

		if(index != -1)
			emit testedPerformance(index);

		if(m_abort) {
			m_mutex.lock();
			break;
		}

		m_mutex.lock();
	}

	m_deviceReady = false;
	m_mutex.unlock();

	m_info->cleanDevice();
}

/*!	\class CZCudaDeviceInfo
	\brief This class implements a container for CUDA-device information.
*/

/*!	\brief Creates CUDA-device information container.
*/
CZCudaDeviceInfo::CZCudaDeviceInfo(
	int devNum,			/*!<[in] Index of device. */
	QObject *parent			/*!<[in,out] Parent of CUDA device information. */
) 	: QObject(parent) {
	memset(&m_info, 0, sizeof(m_info));
	m_info.num = devNum;
	m_info.heavyMode = 0;
	readInfo();
	m_thread = new CZUpdateThread(this, this);
	connect(m_thread, SIGNAL(testedPerformance(int)), this, SIGNAL(testedPerformance(int)));
	m_thread->start();
}

/*!	\brief Destroys cuda information container.
*/
CZCudaDeviceInfo::~CZCudaDeviceInfo() {
	delete m_thread;
}

/*!	\brief This function reads CUDA-device basic information.
	\returns \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::readInfo() {
	return CZCudaReadDeviceInfo(&m_info, m_info.num);
}

/*!	\brief This function prepare some buffers for budwidth tests.
	\returns \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::prepareDevice() {
	if(CZCudaCalcDeviceSelect(&m_info) != 0)
		return 1;
	return CZCudaPrepareDevice(&m_info);
}

/*!	\brief This function updates CUDA-device performance information.
	\returns \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::updateInfo() {
	int r;
	struct CZDeviceInfo info = m_info;

	r = CZCudaCalcDeviceBandwidth(&info);
	if(r != -1)
		r = CZCudaCalcDevicePerformance(&info);

	m_info = info;
	return r;
}

/*!	\brief This function cleans buffers used for bandwidth tests.
	\returns \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaDeviceInfo::cleanDevice() {
	return CZCudaCleanDevice(&m_info);
}

/*!	\brief Returns pointer to inforation structure.
*/
struct CZDeviceInfo &CZCudaDeviceInfo::info() {
	return m_info;
}

/*!	\brief Push performance test in thread.
*/
void CZCudaDeviceInfo::testPerformance(
	int index			/*!<[in] Index of device in list. */
) {
	m_thread->testPerformance(index);
}

/*!	\brief Wait for performance test results.
*/
void CZCudaDeviceInfo::waitPerformance() {
	m_thread->waitPerformance();
}
