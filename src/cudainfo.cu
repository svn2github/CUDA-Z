/*!
	\file cudainfo.cu
	\brief CUDA information functions.
	\author AG
*/

#include <cuda.h>
#include <cuda_runtime.h>
#include <string.h>
#include <windows.h>

#include <QDebug>

#include "cudainfo.h"

#define CZ_BAND_BUF_SIZE	(10 * (1 << 20))	/*!< Transfer buffer size. */
#define CZ_BAND_LOOPS_NUM	(10)			/*!< Number of loops to run transfer test to. */

/*!
	\brief Error handling of CUDA RT calls.
*/
#define CZ_CUDA_CALL(funcCall, errProc) \
	{ \
		cudaError_t errCode; \
		if((errCode = (funcCall)) != cudaSuccess) { \
			printf("CUDA Error: %s\n", cudaGetErrorString(errCode)); \
			errProc; \
		} \
	}

/*!
	\brief Prototype of function \a cuDeviceGetAttribute().
*/
typedef CUresult (CUDAAPI *cuDeviceGetAttribute_t)(int *pi, CUdevice_attribute attrib, CUdevice dev);

/*!
	\brief Prototype of function \a cuInit().
*/
typedef CUresult (CUDAAPI *cuInit_t)(unsigned int Flags);

/*!
	\brief Pointer to function \a cuDeviceGetAttribute().
	This parameter is initializaed by #cudaIsInit().
*/
static cuDeviceGetAttribute_t p_cuDeviceGetAttribute = NULL;

/*!
	\brief Pointer to function \a cuInit().
	This parameter is initializaed by #cudaIsInit().
*/
static cuInit_t p_cuInit = NULL;

/*!
	\brief Check if CUDa fully initialized.
	This function loads nvcuda.dll and finds function \a cuDeviceGetAttribute.
	\return \a true in case of success, \a false in case of error.
*/
static bool cudaIsInit(void) {

	HINSTANCE hDll;

	if((p_cuInit == NULL) || (p_cuDeviceGetAttribute == NULL)) {

		hDll = LoadLibrary(L"nvcuda.dll");
		if(hDll == NULL) {
			return false;
		}

		p_cuDeviceGetAttribute = (cuDeviceGetAttribute_t)GetProcAddress(hDll, "cuDeviceGetAttribute");
		if(p_cuDeviceGetAttribute == NULL) {
			return false;
		}

		p_cuInit = (cuInit_t)GetProcAddress(hDll, "cuInit");
		if(p_cuInit == NULL) {
			return false;
		}
	}

	return true;
}

/*!
	\brief Check if CUDA is present here.
*/
bool cudaCheck(void) {

	if(!cudaIsInit())
		return false;

	if(p_cuInit(0) == CUDA_ERROR_NOT_INITIALIZED) {
		return false;
	}

	return true;
}

/*!
	\brief Check how many CUDA-devices are present.
	\return number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
*/
int cudaDeviceFound(void) {

	int count;

	CZ_CUDA_CALL(cudaGetDeviceCount(&count),
		return 0);
	return count;
}

/*!
	\brief Read information about a CUDA-device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaReadDeviceInfo(
	struct CZDeviceInfo *info,	/*!< CUDA-device information. */
	int num				/*!< Number (index) of CUDA-device. */
) {
	cudaDeviceProp prop;
	int overlap;

	if(info == NULL)
		return -1;

	if(!cudaIsInit())
		return -1;

	if(num >= cudaDeviceFound())
		return -1;

	CZ_CUDA_CALL(cudaGetDeviceProperties(&prop, num),
		return -1);

	if(p_cuDeviceGetAttribute(&overlap, CU_DEVICE_ATTRIBUTE_GPU_OVERLAP, num) != CUDA_SUCCESS)
		return -1;

	info->num = num;
	strcpy(info->deviceName, prop.name);
	info->major = prop.major;
	info->minor = prop.minor;

	info->core.regsPerBlock = prop.regsPerBlock;
	info->core.SIMDWidth = prop.warpSize;
	info->core.maxThreadsPerBlock = prop.maxThreadsPerBlock;
	info->core.maxThreadsDim[0] = prop.maxThreadsDim[0];
	info->core.maxThreadsDim[1] = prop.maxThreadsDim[1];
	info->core.maxThreadsDim[2] = prop.maxThreadsDim[2];
	info->core.maxGridSize[0] = prop.maxGridSize[0];
	info->core.maxGridSize[1] = prop.maxGridSize[1];
	info->core.maxGridSize[2] = prop.maxGridSize[2];
	info->core.clockRate = prop.clockRate;

	info->mem.totalGlobal = prop.totalGlobalMem;
	info->mem.sharedPerBlock = prop.sharedMemPerBlock;
	info->mem.maxPitch = prop.memPitch;
	info->mem.totalConst = prop.totalConstMem;
	info->mem.textureAlignment = prop.textureAlignment;
	info->mem.gpuOverlap = overlap;

	return 0;
}

/*!
	\brief Local service data structure for bandwith calulations.
*/
struct CZDeviceInfoBandLocalData {
	void		*memHostPage;	/*!< Pageable host memory. */
	void		*memHostPin;	/*!< Pinned host memory. */
	void		*memDevice1;	/*!< Device memory buffer 1. */
	void		*memDevice2;	/*!< Device memory buffer 2. */
};

/*!
	\brief Allocate buffers for bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthAlloc(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {
	CZDeviceInfoBandLocalData *lData;

	if(info == NULL)
		return -1;

	if(info->band.localData == NULL) {

		printf("Selecting %s.\n", info->deviceName);

		CZ_CUDA_CALL(cudaSetDevice(info->num),
			return -1);

		printf("Alloc local buffers for %s.\n", info->deviceName);

		lData = (CZDeviceInfoBandLocalData*)malloc(sizeof(*lData));
		if(lData == NULL) {
			return -1;
		}

		printf("Alloc host pageable for %s.\n", info->deviceName);

		lData->memHostPage = (void*)malloc(CZ_BAND_BUF_SIZE);
		if(lData->memHostPage == NULL) {
			free(lData);
			return -1;
		}

		printf("Host pageable is at 0x%08X.\n", lData->memHostPage);

		printf("Alloc host pinned for %s.\n", info->deviceName);

		CZ_CUDA_CALL(cudaMallocHost((void**)&lData->memHostPin, CZ_BAND_BUF_SIZE),
			free(lData->memHostPage);
			free(lData);
			return -1);

		printf("Host pinned is at 0x%08X.\n", lData->memHostPin);

		printf("Alloc device buffer 1 for %s.\n", info->deviceName);

		CZ_CUDA_CALL(cudaMalloc((void**)&lData->memDevice1, CZ_BAND_BUF_SIZE),
			cudaFreeHost(lData->memHostPin);
			free(lData->memHostPage);
			free(lData);
			return -1);

		printf("Device buffer 1 is at 0x%08X.\n", lData->memDevice1);

		printf("Alloc device buffer 2 for %s.\n", info->deviceName);

		CZ_CUDA_CALL(cudaMalloc((void**)&lData->memDevice2, CZ_BAND_BUF_SIZE),
			cudaFree(lData->memDevice1);
			cudaFreeHost(lData->memHostPin);
			free(lData->memHostPage);
			free(lData);
			return -1);

		printf("Device buffer 2 is at 0x%08X.\n", lData->memDevice2);

		info->band.localData = (void*)lData;
	}

	return 0;
}

/*!
	\brief Free buffers for bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthFree(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {
	CZDeviceInfoBandLocalData *lData;

	if(info == NULL)
		return -1;

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;
	if(lData != NULL) {

		printf("Selecting %s.\n", info->deviceName);

		CZ_CUDA_CALL(cudaSetDevice(info->num),
			return -1);

		printf("Free host pageable for %s.\n", info->deviceName);

		if(lData->memHostPage != NULL)
			free(lData->memHostPage);

		printf("Free host pinned for %s.\n", info->deviceName);

		if(lData->memHostPin != NULL)
			cudaFreeHost(lData->memHostPin);

		printf("Free device buffer 1 for %s.\n", info->deviceName);

		if(lData->memDevice1 != NULL)
			cudaFree(lData->memDevice1);

		printf("Free device buffer 2 for %s.\n", info->deviceName);

		if(lData->memDevice2 != NULL)
			cudaFree(lData->memDevice2);

		printf("Free local buffers for %s.\n", info->deviceName);

		free(lData);
	}
	info->band.localData = NULL;

	return 0;
}

/*!
	\brief Reset results of bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthReset(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	info->band.copyHDPage = 0;
	info->band.copyHDPin = 0;
	info->band.copyDHPage = 0;
	info->band.copyDHPin = 0;
	info->band.copyDD = 0;

	return 0;
}

/*!
	\brief Run host to device data transfer bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthTestHD (
	struct CZDeviceInfo *info,	/*!< CUDA-device information. */
	int pinned			/*!< Use pinned \a (=1) memory buffer instead of pagable \a (=0). */
) {
	CZDeviceInfoBandLocalData *lData;
	float timeMs = 0.0;
	float bandwidthKBs = 0.0;
	cudaEvent_t start;
	cudaEvent_t stop;
	void *memHost;
	void *memDevice;
	int i;

	if(info == NULL)
		return 0;

	printf("Selecting %s.\n", info->deviceName);

	CZ_CUDA_CALL(cudaSetDevice(info->num),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&start),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&stop),
		cudaEventDestroy(start);
		return 0);

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;

	memHost = pinned? lData->memHostPin: lData->memHostPage;
	memDevice = lData->memDevice1;

	printf("Starting H(0x%08X)->D(0x%08X) test (%s) on %s.\n",
		memHost, memDevice,
		pinned? "pinned": "pageable",
		info->deviceName);

	CZ_CUDA_CALL(cudaEventRecord(start, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	for(i = 0; i < CZ_BAND_LOOPS_NUM; i++) {
		CZ_CUDA_CALL(cudaMemcpy(memDevice, memHost, CZ_BAND_BUF_SIZE, cudaMemcpyHostToDevice),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);
	}

	CZ_CUDA_CALL(cudaEventRecord(stop, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventSynchronize(stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventElapsedTime(&timeMs, start, stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	printf("Test complete in %f ms.\n", timeMs);

	bandwidthKBs = (1000 * (float)CZ_BAND_BUF_SIZE * (float)CZ_BAND_LOOPS_NUM) / (timeMs * (float)(1 << 10));

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	return (int)bandwidthKBs;
}

/*!
	\brief Run device to host data transfer bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthTestDH (
	struct CZDeviceInfo *info,	/*!< CUDA-device information. */
	int pinned			/*!< Use pinned \a (=1) memory buffer instead of pagable \a (=0). */
) {
	CZDeviceInfoBandLocalData *lData;
	float timeMs = 0.0;
	float bandwidthKBs = 0.0;
	cudaEvent_t start;
	cudaEvent_t stop;
	void *memHost;
	void *memDevice;
	int i;

	if(info == NULL)
		return 0;

	printf("Selecting %s.\n", info->deviceName);

	CZ_CUDA_CALL(cudaSetDevice(info->num),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&start),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&stop),
		cudaEventDestroy(start);
		return 0);

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;

	memHost = pinned? lData->memHostPin: lData->memHostPage;
	memDevice = lData->memDevice2;

	printf("Starting D(0x%08X)->H(0x%08X) test (%s) on %s.\n",
		memDevice, memHost,
		pinned? "pinned": "pageable",
		info->deviceName);

	CZ_CUDA_CALL(cudaEventRecord(start, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	for(i = 0; i < CZ_BAND_LOOPS_NUM; i++) {
		CZ_CUDA_CALL(cudaMemcpy(memDevice, memHost, CZ_BAND_BUF_SIZE, cudaMemcpyHostToDevice),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);
	}

	CZ_CUDA_CALL(cudaEventRecord(stop, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventSynchronize(stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventElapsedTime(&timeMs, start, stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	printf("Test complete in %f ms.\n", timeMs);

	bandwidthKBs = (1000 * (float)CZ_BAND_BUF_SIZE * (float)CZ_BAND_LOOPS_NUM) / (timeMs * (float)(1 << 10));

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	return (int)bandwidthKBs;
}

/*!
	\brief Run device to device data transfer bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthTestDD (
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {
	CZDeviceInfoBandLocalData *lData;
	float timeMs = 0.0;
	float bandwidthKBs = 0.0;
	cudaEvent_t start;
	cudaEvent_t stop;
	void *memDevice1;
	void *memDevice2;
	int i;

	if(info == NULL)
		return 0;

	printf("Selecting %s.\n", info->deviceName);

	CZ_CUDA_CALL(cudaSetDevice(info->num),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&start),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&stop),
		cudaEventDestroy(start);
		return 0);

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;

	memDevice1 = lData->memDevice1;
	memDevice2 = lData->memDevice2;

	printf("Starting D(0x%08X)->D(0x%08X) test on %s.\n",
		memDevice1, memDevice2,
		info->deviceName);

	CZ_CUDA_CALL(cudaEventRecord(start, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	for(i = 0; i < CZ_BAND_LOOPS_NUM; i++) {
		CZ_CUDA_CALL(cudaMemcpy(memDevice2, memDevice1, CZ_BAND_BUF_SIZE, cudaMemcpyDeviceToDevice),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);
	}

	CZ_CUDA_CALL(cudaEventRecord(stop, 0),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventSynchronize(stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	CZ_CUDA_CALL(cudaEventElapsedTime(&timeMs, start, stop),
		cudaEventDestroy(start);
		cudaEventDestroy(stop);
		return 0);

	printf("Test complete in %f ms.\n", timeMs);

	bandwidthKBs = (1000 * (float)CZ_BAND_BUF_SIZE * (float)CZ_BAND_LOOPS_NUM) / (timeMs * (float)(1 << 10));

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	return (int)bandwidthKBs;
}

/*!
	\brief Run several bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidthTest(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {

	info->band.copyHDPage = cudaCalcDeviceBandwidthTestHD(info, 0);
	info->band.copyHDPin = cudaCalcDeviceBandwidthTestHD(info, 1);
	info->band.copyDHPage = cudaCalcDeviceBandwidthTestDH(info, 0);
	info->band.copyDHPin = cudaCalcDeviceBandwidthTestDH(info, 1);
	info->band.copyDD = cudaCalcDeviceBandwidthTestDD(info);

	return 0;
}

/*!
	\brief Prepare buffers bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaPrepareDevice(
	struct CZDeviceInfo *info
) {

	if(info == NULL)
		return -1;

	if(!cudaIsInit())
		return -1;

	if(cudaCalcDeviceBandwidthAlloc(info) != 0)
		return -1;

	return 0;
}

/*!
	\brief Calculate bandwidth information about CUDA-device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCalcDeviceBandwidth(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {

//	printf("cudaCalcDeviceBandwidth called!\n");

	if(info == NULL)
		return -1;

	if(cudaCalcDeviceBandwidthReset(info) != 0)
		return -1;

	if(!cudaIsInit())
		return -1;

	if(cudaCalcDeviceBandwidthAlloc(info) != 0)
		return -1;

	if(cudaCalcDeviceBandwidthTest(info) != 0)
		return -1;

	return 0;
}

/*!
	\brief Cleanup after test and bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int cudaCleanDevice(
	struct CZDeviceInfo *info	/*!< CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	if(cudaCalcDeviceBandwidthFree(info) != 0)
		return -1;

	return 0;
}
