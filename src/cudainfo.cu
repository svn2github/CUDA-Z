/*!
	\file cudainfo.cu
	\brief CUDA information functions.
	\author AG
*/

#include <cuda.h>
#include <string.h>

#include "cudainfo.h"

/*!
	\brief Check if CUDA is present here.
*/
bool cudaCheck(void) {

	if(cuInit(0) == CUDA_ERROR_NOT_INITIALIZED) {
		return false;
	}
	return true;
}

/*!
	\brief Check how many CUDA-devices are present.
*/
int cudaDeviceFound(void) {

	int count;

	if(cuDeviceGetCount(&count) == CUDA_SUCCESS) {
		return count;
	}
	return 0;
}

/*!
	\brief Read information about a CUDA-device.
*/
int cudaReadDeviceInfo(struct CZDeviceInfo *info, int num) {

	char name[256];
	unsigned int totalMem;
	int major;
	int minor;
	CUdevprop prop;
	int overlap;

	if(info == NULL)
		return -1;

	if(num >= cudaDeviceFound())
		return -1;

	if(cuDeviceGetName(name, 256, num) != CUDA_SUCCESS)
		return -1;

	if(cuDeviceTotalMem(&totalMem, num) != CUDA_SUCCESS)
		return -1;

	if(cuDeviceComputeCapability(&major, &minor, num) != CUDA_SUCCESS)
		return -1;

	if(cuDeviceGetProperties(&prop, num) != CUDA_SUCCESS)
		return -1;

	if(cuDeviceGetAttribute(&overlap, CU_DEVICE_ATTRIBUTE_GPU_OVERLAP, num) != CUDA_SUCCESS)
		return -1;

	info->num = num;
	strcpy(info->deviceName, name);
	info->major = major;
	info->minor = minor;

	info->core.regsPerBlock = prop.regsPerBlock;
	info->core.SIMDWidth = prop.SIMDWidth;
	info->core.maxThreadsPerBlock = prop.maxThreadsPerBlock;
	info->core.maxThreadsDim[0] = prop.maxThreadsDim[0];
	info->core.maxThreadsDim[1] = prop.maxThreadsDim[1];
	info->core.maxThreadsDim[2] = prop.maxThreadsDim[2];
	info->core.maxGridSize[0] = prop.maxGridSize[0];
	info->core.maxGridSize[1] = prop.maxGridSize[1];
	info->core.maxGridSize[2] = prop.maxGridSize[2];
	info->core.clockRate = prop.clockRate;

	info->mem.totalGlobal = totalMem;
	info->mem.sharedPerBlock = prop.sharedMemPerBlock;
	info->mem.maxPitch = prop.memPitch;
	info->mem.totalConst = prop.totalConstantMemory;
	info->mem.textureAlignment = prop.textureAlign;
	info->mem.gpuOverlap = overlap;

	return 0;
}
