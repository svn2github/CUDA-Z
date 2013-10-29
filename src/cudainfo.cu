/*!	\file cudainfo.cu
	\brief CUDA information functions.
	\author Andriy Golovnya <andrew_golovnia@ukr.net> http://ag.embedded.org.ru/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv2 http://www.gnu.org/licenses/gpl-2.0.html
*/

#include <cuda.h>
#include <cuda_runtime.h>
#include <host_defines.h>
#include <string.h>

#if CUDA_VERSION < 5050
#error CUDA 1.x - 5.x are not supported any more! Please use CUDA Toolkit 5.5+ instead.
#endif

#include "log.h"
#include "cudainfo.h"

#if (defined(WIN64) || defined(_WIN64) || defined(__WIN64__)) || (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#define Q_OS_WIN
#elif defined(__linux__) || defined(__linux)
#define Q_OS_LINUX
#elif defined(__APPLE__) && defined(__GNUC__)
#define Q_OS_MAC
#else
#error Unknown/unsupported platform!
#endif

#define CZ_COPY_BUF_SIZE	(16 * (1 << 20))	/*!< Transfer buffer size. */
#define CZ_COPY_LOOPS_NUM	8			/*!< Number of loops to run transfer test to. */

#define CZ_CALC_BLOCK_LOOPS	16			/*!< Number of loops to run calculation loop. */
#define CZ_CALC_BLOCK_SIZE	256			/*!< Size of instruction block. */
#define CZ_CALC_BLOCK_NUM	16			/*!< Number of instruction blocks in loop. */
#define CZ_CALC_OPS_NUM		2			/*!< Number of operations per one loop. */
#define CZ_CALC_LOOPS_NUM	8			/*!< Number of loops to run performance test to. */

#define CZ_DEF_WARP_SIZE	32			/*!< Default warp size value. */
#define CZ_DEF_THREADS_MAX	512			/*!< Default max threads value value. */

#define CZ_VER_STR_LEN		256			/*!< Version string length. */

/*!	\brief Error handling of CUDA RT calls.
*/
#define CZ_CUDA_CALL(funcCall, errProc) \
	{ \
		cudaError_t errCode; \
		if((errCode = (funcCall)) != cudaSuccess) { \
			CZLog(CZLogLevelError, "CUDA Error: %08x %s", errCode, cudaGetErrorString(errCode)); \
			errProc; \
		} \
	}

/*!	\brief Prototype of function \a cuDeviceGetAttribute().
*/
typedef CUresult (CUDAAPI *cuDeviceGetAttribute_t)(int *pi, CUdevice_attribute attrib, CUdevice dev);

/*!	\brief Prototype of function \a cuInit().
*/
typedef CUresult (CUDAAPI *cuInit_t)(unsigned int Flags);

/*!	\brief Pointer to function \a cuDeviceGetAttribute().
	This parameter is initializaed by CZCudaIsInit().
*/
static cuDeviceGetAttribute_t p_cuDeviceGetAttribute = NULL;

/*!	\brief Pointer to function \a cuInit().
	This parameter is initializaed by CZCudaIsInit().
*/
static cuInit_t p_cuInit = NULL;

/*!	\brief Driver version string.
*/
static char drvVersion[CZ_VER_STR_LEN] = "";

/*!	\brief Driver dll version.
*/
static int drvDllVer = 0;

/*!	\brief Driver dll version string.
*/
static char drvDllVerStr[CZ_VER_STR_LEN] = "";

/*!	\brief Runtime dll version.
*/
static int rtDllVer = 0;

/*!	\brief Runtime dll version string.
*/
static char rtDllVerStr[CZ_VER_STR_LEN] = "";

#if defined(Q_OS_WIN)
//#include <windows.h>
#define CZ_DLL_LIST_LEN		64			/*!< Process dll list length. */
#define CZ_DLL_BNAME_LEN	64			/*!< Dll base name length. */
#define CZ_DLL_FNAME_LEN	256			/*!< Dll file name length. */
#define CZ_DLL_FNAME		"nvcuda.dll"		/*!< CUDA dll file name. */

#ifdef __cplusplus
extern "C" {
#endif
#define WINAPI __stdcall
typedef void *HANDLE;
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef HINSTANCE HMODULE;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef short DWORD, *LPDWORD;
typedef unsigned int UINT, *PUINT;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef bool BOOL;
typedef int (WINAPI *FARPROC)();
HMODULE WINAPI LoadLibraryA(__in LPCSTR lpLibFileName);
FARPROC WINAPI GetProcAddress(__in HMODULE hModule, __in LPCSTR lpProcName);
DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle);
BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);
HANDLE WINAPI GetCurrentProcess(void);
BOOL WINAPI EnumProcessModules(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
DWORD WINAPI GetModuleBaseNameA(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize);
DWORD WINAPI GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
#ifdef __cplusplus
}
#endif

/*!	\brief Get version of dll library.
*/
static char *CZGetDllVersion(
	char *name,			/*!<[in] Name of dll file. */
	char *version			/*!<[out] Dll version buffer. */
) {
	DWORD dwVerInfoSize;
	DWORD dwVerHnd = 0;
	LPSTR lpstrVffInfo;
	LPSTR lpVersion = NULL;
	UINT uVersionLen = 0;

	dwVerInfoSize = GetFileVersionInfoSizeA(name, &dwVerHnd);
	if(!dwVerInfoSize) {
		return NULL;
	}

	lpstrVffInfo = (LPSTR)malloc(dwVerInfoSize);
	if(lpstrVffInfo == NULL) {
		return NULL;
	}

	if(!GetFileVersionInfoA(name, dwVerHnd, dwVerInfoSize, lpstrVffInfo)) {
		free(lpstrVffInfo);
		return NULL;
	}

	if(!VerQueryValueA(lpstrVffInfo, (LPSTR)"\\StringFileInfo\\040904E4\\FileVersion",
		(LPVOID*)&lpVersion, (UINT*)&uVersionLen)) {
		free(lpstrVffInfo);
		return NULL;
	}

	strncpy(version, lpVersion, CZ_VER_STR_LEN - 1);

	CZLog(CZLogLevelLow, "Version of %s is %s.", name, version);

	free(lpstrVffInfo);
	return version;
}

/*!	\brief Get description of dll library.
*/
static char *CZGetDllDescription(
	char *name,			/*!<[in] Name of dll file. */
	char *description		/*!<[out] Dll description buffer. */
) {
	DWORD dwVerInfoSize;
	DWORD dwVerHnd = 0;
	LPSTR lpstrVffInfo;
	LPSTR lpDescription = NULL;
	UINT uDescriptionLen = 0;

	dwVerInfoSize = GetFileVersionInfoSizeA(name, &dwVerHnd);
	if(!dwVerInfoSize) {
		return NULL;
	}

	lpstrVffInfo = (LPSTR)malloc(dwVerInfoSize);
	if(lpstrVffInfo == NULL) {
		return NULL;
	}

	if(!GetFileVersionInfoA(name, dwVerHnd, dwVerInfoSize, lpstrVffInfo)) {
		free(lpstrVffInfo);
		return NULL;
	}

	if(!VerQueryValueA(lpstrVffInfo, (LPSTR)"\\StringFileInfo\\040904E4\\FileDescription",
		(LPVOID*)&lpDescription, (UINT*)&uDescriptionLen)) {
		free(lpstrVffInfo);
		return NULL;
	}

	strncpy(description, lpDescription, CZ_VER_STR_LEN - 1);

	CZLog(CZLogLevelLow, "Description of %s is %s.", name, description);

	free(lpstrVffInfo);
	return description;
}

/*!	\brief Check if CUDA fully initialized.
	This function loads nvcuda.dll and finds functions \a cuInit()
	and \a cuDeviceGetAttribute().
	\return \a true in case of success, \a false in case of error.
*/
static bool CZCudaIsInit(void) {

	HINSTANCE hDll;
	HMODULE hModule[CZ_DLL_LIST_LEN];
	DWORD cbRet = 0;
	char description[CZ_VER_STR_LEN] = "";

	if((p_cuInit == NULL) || (p_cuDeviceGetAttribute == NULL)) {

		hDll = LoadLibraryA(CZ_DLL_FNAME);
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

		CZGetDllVersion(CZ_DLL_FNAME, drvDllVerStr);

		if(CZGetDllDescription(CZ_DLL_FNAME, description) != NULL) {
			char *p = NULL;
			char *version = "version";
			strlwr(description);
			if((p = strstr(description, version)) != NULL) {
				p += strlen(version);
				while(*p == ' ')
					p++;
				strncpy(drvVersion, p, CZ_VER_STR_LEN - 1);
				p = drvVersion + strlen(drvVersion) - 1;
				while(*p == ' ') {
					*p = 0;
					p--;
				}
			}
		}

		if(EnumProcessModules(GetCurrentProcess(), hModule, sizeof(hModule), &cbRet) == true) {
			UINT i;
			char bname[CZ_DLL_BNAME_LEN];
			char fname[CZ_DLL_FNAME_LEN];
			for(i = 0; i < (cbRet / sizeof(HMODULE)); i++) {
				bname[0] = 0;
				fname[0] = 0;
				GetModuleBaseNameA(GetCurrentProcess(), hModule[i], bname, CZ_DLL_BNAME_LEN - 1);
				strlwr(bname);
				if(strstr(bname, "cudart") != NULL) {
					GetModuleFileNameA(hModule[i], fname, CZ_DLL_FNAME_LEN - 1);
					CZGetDllVersion(fname, rtDllVerStr);
					break;
				}
			}
		}
	}

	return true;
}

#elif defined(Q_OS_LINUX)
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define CZ_FILE_STR_LEN		256			/*!< Version file string length. */
#define CZ_VER_FILE_NAME	"/proc/driver/nvidia/version"	/*!< Driver version file name. */
#define CZ_PROC_MAP_NAME	"/proc/self/maps"	/*!< Process memory map file. */
#define CZ_DLL_FNAME		"libcuda.so"		/*!< CUDA dll file name. */
#define CZ_DLL_FNAME_RT		"libcudart.so"		/*!< CUDA RT dll file name. */
#define CZ_LD_SO_CONF		"/etc/ld.do.conf"	/*!< ld.so configuration file. */
#define CZ_LD_SO_DIR		"/etc/ld.do.conf.d/"	/*!< ld.so configuration directory. */
#define CZ_LD_SO_LINE_MAX	100			/*!< ld.so configuration line length. */

/*!	\brief Get version of shared library.
*/
static char *CZGetSoVersion(
	char *name,			/*!<[in] Name of so file. E.g. "libcuda.so". */
	char *version			/*!<[out] Library version buffer. */
) {
	FILE *fp = NULL;
	char str[CZ_FILE_STR_LEN];
	int found = 0;

	fp = fopen(CZ_PROC_MAP_NAME, "r");
	if(fp == NULL) {
		return NULL;
	}

	while(fgets(str, CZ_FILE_STR_LEN - 1, fp) != NULL) {
		if(strstr(str, name) != NULL) {
			char *p = NULL;
			char fname[CZ_FILE_STR_LEN];

			p = str + strlen(str) - 1;
			while((p >= str) && ((*p == ' ') || (*p == '\n') || (*p == '\r') || (*p == '\t') || (*p == 0))) {
				*p = 0;
				p--;
			}

			while((p >= str) && ((*p != ' ') && (*p != '\n') && (*p != '\r') && (*p != '\t') && (*p != 0))) {
				p--;
			}

			strncpy(fname, p, CZ_FILE_STR_LEN - 1);
			p = basename(fname);
			if(p == NULL)
				continue;

			if(strstr(p, name) != p) {
				continue;
			}

			p = p + strlen(name);
			if(*p != '.')
				continue;

			strncpy(version, p + 1, CZ_VER_STR_LEN - 1);

			found++;
			break;
		}
	}

	fclose(fp);

	if(found) {
		CZLog(CZLogLevelLow, "Version of %s is %s.", name, version);
		return version;
	} else {
		return NULL;
	}
}

/*!	\brief Check if CUDA fully initialized.
	This function loads libcuda.so and finds functions \a cuInit()
	and \a cuDeviceGetAttribute().
	\return \a true in case of success, \a false in case of error.
*/
static bool CZCudaIsInit(void) {

	void *hDll = NULL;

	if((p_cuInit == NULL) || (p_cuDeviceGetAttribute == NULL)) {

		if(hDll == NULL) {
			hDll = dlopen(CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib/nvidia-current/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib32/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib32/nvidia-current/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib64/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib64/nvidia-current/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib128/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/lib128/nvidia-current/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		// Try to check ld.so.conf & Co
		if(hDll == NULL) {
			char buf[CZ_LD_SO_LINE_MAX + sizeof(CZ_DLL_FNAME) + 1];
			char *p;
			FILE *f;

			f = popen("cat " CZ_LD_SO_CONF " " CZ_LD_SO_DIR "/*", "r");
			if(f != NULL) {
				while(fgets(buf, CZ_LD_SO_LINE_MAX, f) != NULL) {
					if((p = strchr(buf, '\n')) != NULL) *p = 0;
					if((p = strchr(buf, '#')) != NULL) *p = 0;
					if(strlen(buf) > 0) {
						strcat(buf, "/" CZ_DLL_FNAME);
						if(hDll == NULL) {
							hDll = dlopen(buf, RTLD_LAZY);
						}
					}
				}
				pclose(f);
			}
		}

		if(hDll == NULL) {
			CZLog(CZLogLevelError, "Can't load CUDA driver.");
			return false;
		}

		p_cuDeviceGetAttribute = (cuDeviceGetAttribute_t)dlsym(hDll, "cuDeviceGetAttribute");
		if(p_cuDeviceGetAttribute == NULL) {
			return false;
		}

		p_cuInit = (cuInit_t)dlsym(hDll, "cuInit");
		if(p_cuInit == NULL) {
			return false;
		}

		CZGetSoVersion(CZ_DLL_FNAME, drvDllVerStr);
		CZGetSoVersion(CZ_DLL_FNAME_RT, rtDllVerStr);

		if(access(CZ_VER_FILE_NAME, R_OK) == 0) {
			FILE *fp = NULL;
			char str[CZ_FILE_STR_LEN];
			fp = fopen(CZ_VER_FILE_NAME, "r");
			if(fp != NULL) {
				while(fgets(str, CZ_FILE_STR_LEN - 1, fp) != NULL) {
					char *p = NULL;
					char *kernel_module = "Kernel Module";
					if((p = strstr(str, kernel_module)) != NULL) {
						p += strlen(kernel_module);
						while(*p == ' ')
							p++;
						strncpy(drvVersion, p, CZ_VER_STR_LEN - 1);
						p = drvVersion;
						while((*p != ' ') && (*p != '\n') && (*p != '\r') && (*p != '\t') && (*p != 0)) {
							p++;
						}
						*p = 0;
						break;
					}
				}
				fclose(fp);
			}
		}

	}
	return true;
}

#elif defined(Q_OS_MAC)
#include <dlfcn.h>
#include <stdio.h>
#include "plist.h"
#define CZ_FILE_STR_LEN		256			/*!< Version file string length. */
#define CZ_PLIST_PATH		"/Contents/Info.plist"	/*!< Path to Info.plist inside of kext/app. */
#define CZ_KEXT_SYSTEM_PATH	"/System/Library/Extensions/"	/*!< Main kext path. */
#define CZ_KEXT_EXTRA_PATH	"/Extra/Extensions/"	/*!< Alternative kext path. */
#define CZ_PLIST_GETINFOSTR	"CFBundleGetInfoString"	/*!< Most informative property name. */
#define CZ_PLIST_SHORTVERSTR	"CFBundleShortVersionString"	/*!< Less informative property name. */
#define CZ_DLL_FNAME		"libcuda.dylib"		/*!< CUDA dll file name. */

/*!	\brief Get version of Kext driver.
*/
static char *CZGetKextVersion(
	char *name,			/*!<[in] Name of kext file. E.g. "GeForce". */
	char *version			/*!<[out] Kext version buffer. */
) {
	char plist[CZ_FILE_STR_LEN];
	char str[CZ_FILE_STR_LEN];
	char *p;

	sprintf(plist, CZ_KEXT_SYSTEM_PATH "%s.kext" CZ_PLIST_PATH, name);

	if(CZPlistGet(plist, CZ_PLIST_GETINFOSTR, str, sizeof(str)) == 0) {
		p = strstr(str, name);
		if(p != NULL) {
			p = p + strlen(name);
			while(*p == ' ')
				p++;
		} else {
			p = str;
		}
		strcpy(version, p);
		return version;
	}

	if(CZPlistGet(plist, CZ_PLIST_SHORTVERSTR, str, sizeof(str)) == 0) {
		p = strstr(str, name);
		if(p != NULL) {
			p = p + strlen(name);
			while(*p == ' ')
				p++;
		} else {
			p = str;
		}
		strcpy(version, p);
		return version;
	}

	sprintf(plist, CZ_KEXT_EXTRA_PATH "%s.kext" CZ_PLIST_PATH, name);

	if(CZPlistGet(plist, CZ_PLIST_GETINFOSTR, str, sizeof(str)) == 0) {
		p = strstr(str, name);
		if(p != NULL) {
			p = p + strlen(name);
			while(*p == ' ')
				p++;
		} else {
			p = str;
		}
		strcpy(version, p);
		return version;
	}

	if(CZPlistGet(plist, CZ_PLIST_SHORTVERSTR, str, sizeof(str)) == 0) {
		p = strstr(str, name);
		if(p != NULL) {
			p = p + strlen(name);
			while(*p == ' ')
				p++;
		} else {
			p = str;
		}
		strcpy(version, p);
		return version;
	}

	return NULL;
}

/*!	\brief Check if CUDA fully initialized.
	This function loads libcuda.dylib and finds functions \a cuInit()
	and \a cuDeviceGetAttribute().
	\return \a true in case of success, \a false in case of error.
*/
static bool CZCudaIsInit(void) {
	void *hDll = NULL;

	if((p_cuInit == NULL) || (p_cuDeviceGetAttribute == NULL)) {

		if(hDll == NULL) {
			hDll = dlopen(CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("@rpath/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("@executable_path/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			hDll = dlopen("/usr/local/cuda/lib/" CZ_DLL_FNAME, RTLD_LAZY);
		}

		if(hDll == NULL) {
			CZLog(CZLogLevelError, "Can't load CUDA driver.");
			return false;
		}

		p_cuDeviceGetAttribute = (cuDeviceGetAttribute_t)dlsym(hDll, "cuDeviceGetAttribute");
		if(p_cuDeviceGetAttribute == NULL) {
			return false;
		}

		p_cuInit = (cuInit_t)dlsym(hDll, "cuInit");
		if(p_cuInit == NULL) {
			return false;
		}

		CZGetKextVersion("GeForce", drvVersion);
	}
	return true;
}

#else//!Q_OS_WIN && !Q_OS_LINUX && !Q_OS_MAC
#error Function CZCudaIsInit() is not implemented for your platform!
#endif//Q_OS_WIN

/*!	\brief Check if CUDA is present here.
*/
bool CZCudaCheck(void) {

	if(!CZCudaIsInit())
		return false;

	if(p_cuInit(0) == CUDA_ERROR_NOT_INITIALIZED) {
		return false;
	}

	CZ_CUDA_CALL(cudaDriverGetVersion(&drvDllVer),
		drvDllVer = 0);

	CZLog(CZLogLevelLow, "Driver version %d.", drvDllVer);

	CZ_CUDA_CALL(cudaRuntimeGetVersion(&rtDllVer),
		rtDllVer = 0);

	CZLog(CZLogLevelLow, "Runtime version %d.", rtDllVer);

	return true;
}

/*!	\brief Check how many CUDA-devices are present.
	\return number of CUDA-devices in case of success, \a 0 if no CUDA-devies were found.
*/
int CZCudaDeviceFound(void) {

	int count;

	CZ_CUDA_CALL(cudaGetDeviceCount(&count),
		return 0);

	return count;
}

/*!	\def ConvertSMVer2Cores(major, minor)
	\brief Get number of CUDA cores per multiprocessor.
	\arg[in] major GPU Architecture major version.
	\arg[in] minor GPU Architecture minor version.
	\returns 0 if GPU Architecture is unknown, or number of CUDA cores per multiprocessor.
*/
#define ConvertSMVer2Cores(major, minor) \
	(((major) == 1)? ( \
		((minor) == 0)? 8: /* G80*/ \
		((minor) == 1)? 8: /* G8x */ \
		((minor) == 2)? 8: /* G9x */ \
		((minor) == 3)? 8: /* GT200 */ \
		0): \
	((major) == 2)? ( \
		((minor) == 0)? 32: /* GF100 */ \
		((minor) == 1)? 48: /* GF10x */ \
		0): \
	((major) == 3)? ( \
		((minor) == 0)? 192: /* GK10x */ \
		((minor) == 5)? 192: /* GK11x */ \
		0): \
	0)

/*!	\def COMPILE_ASSERT(cond)
	\arg[in] cond Static condition.
	\brief Compile time assert() for constant conditions.
*/
#define COMPILE_ASSERT(cond)	{typedef char compile_assert_error[(cond)? 1: -1];}

/*!	\brief Read information about a CUDA-device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaReadDeviceInfo(
	struct CZDeviceInfo *info,	/*!<[in,out] CUDA-device information. */
	int num				/*!<[in] Number (index) of CUDA-device. */
) {
	cudaDeviceProp prop;
//	int ecc;

	COMPILE_ASSERT(ConvertSMVer2Cores(0, 0) == 0);
	COMPILE_ASSERT(ConvertSMVer2Cores(1, 0) == 8);
	COMPILE_ASSERT(ConvertSMVer2Cores(1, 1) == 8);
	COMPILE_ASSERT(ConvertSMVer2Cores(1, 2) == 8);
	COMPILE_ASSERT(ConvertSMVer2Cores(1, 3) == 8);
	COMPILE_ASSERT(ConvertSMVer2Cores(1, 4) == 0);
	COMPILE_ASSERT(ConvertSMVer2Cores(2, 0) == 32);
	COMPILE_ASSERT(ConvertSMVer2Cores(2, 1) == 48);
	COMPILE_ASSERT(ConvertSMVer2Cores(2, 2) == 0);
	COMPILE_ASSERT(ConvertSMVer2Cores(3, 0) == 192);
	COMPILE_ASSERT(ConvertSMVer2Cores(3, 1) == 0);
	COMPILE_ASSERT(ConvertSMVer2Cores(3, 5) == 192);
	COMPILE_ASSERT(ConvertSMVer2Cores(3, 6) == 0);
	COMPILE_ASSERT(ConvertSMVer2Cores(4, 0) == 0);

	if(info == NULL)
		return -1;

	if(!CZCudaIsInit())
		return -1;

	if(num >= CZCudaDeviceFound())
		return -1;

	CZ_CUDA_CALL(cudaGetDeviceProperties(&prop, num),
		return -1);

	info->num = num;
	strcpy(info->deviceName, prop.name);
	info->major = prop.major;
	info->minor = prop.minor;
	info->drvVersion = drvVersion;
	info->drvDllVer = drvDllVer;
	info->drvDllVerStr = drvDllVerStr;
	info->rtDllVer = rtDllVer;
	info->rtDllVerStr = rtDllVerStr;
	info->tccDriver = prop.tccDriver;

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
	info->core.muliProcCount = prop.multiProcessorCount;
	info->core.watchdogEnabled = prop.kernelExecTimeoutEnabled;
	info->core.integratedGpu = prop.integrated;
	info->core.concurrentKernels = prop.concurrentKernels;
	info->core.computeMode =
		(prop.computeMode == cudaComputeModeDefault)? CZComputeModeDefault:
		(prop.computeMode == cudaComputeModeExclusive)? CZComputeModeExclusive:
		(prop.computeMode == cudaComputeModeProhibited)? CZComputeModeProhibited:
		CZComputeModeUnknown;
	info->core.pciBusID = prop.pciBusID;
	info->core.pciDeviceID = prop.pciDeviceID;
	info->core.pciDomainID = prop.pciDomainID;
	info->core.maxThreadsPerMultiProcessor = prop.maxThreadsPerMultiProcessor;
	info->core.cudaCores = ConvertSMVer2Cores(prop.major, prop.minor) * prop.multiProcessorCount;
	info->core.streamPrioritiesSupported = prop.streamPrioritiesSupported;

	info->mem.totalGlobal = prop.totalGlobalMem;
	info->mem.sharedPerBlock = prop.sharedMemPerBlock;
	info->mem.maxPitch = prop.memPitch;
	info->mem.totalConst = prop.totalConstMem;
	info->mem.textureAlignment = prop.textureAlignment;
	info->mem.texture1D[0] = prop.maxTexture1D;
	info->mem.texture2D[0] = prop.maxTexture2D[0];
	info->mem.texture2D[1] = prop.maxTexture2D[1];
	info->mem.texture3D[0] = prop.maxTexture3D[0];
	info->mem.texture3D[1] = prop.maxTexture3D[1];
	info->mem.texture3D[2] = prop.maxTexture3D[2];
	info->mem.gpuOverlap = prop.deviceOverlap;
	info->mem.mapHostMemory = prop.canMapHostMemory;
        info->mem.errorCorrection = prop.ECCEnabled;
	info->mem.asyncEngineCount = prop.asyncEngineCount;
	info->mem.unifiedAddressing = prop.unifiedAddressing;
	info->mem.memoryClockRate = prop.memoryClockRate;
	info->mem.memoryBusWidth = prop.memoryBusWidth;
	info->mem.l2CacheSize = prop.l2CacheSize;

/*	if(p_cuDeviceGetAttribute(&ecc, CU_DEVICE_ATTRIBUTE_ECC_ENABLED, num) != CUDA_SUCCESS)
		return -1;
	info->mem.errorCorrection = ecc;*/

	return 0;
}

/*!	\brief Local service data structure for bandwith calulations.
*/
struct CZDeviceInfoBandLocalData {
	void		*memHostPage;	/*!< Pageable host memory. */
	void		*memHostPin;	/*!< Pinned host memory. */
	void		*memDevice1;	/*!< Device memory buffer 1. */
	void		*memDevice2;	/*!< Device memory buffer 2. */
};

/*!	\brief Set device for current thread.
*/
int CZCudaCalcDeviceSelect(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	CZLog(CZLogLevelLow, "Selecting %s.", info->deviceName);

	CZ_CUDA_CALL(cudaSetDevice(info->num),
		return -1);

	return 0;
}

/*!	\brief Allocate buffers for bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static int CZCudaCalcDeviceBandwidthAlloc(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {
	CZDeviceInfoBandLocalData *lData;

	if(info == NULL)
		return -1;

	if(info->band.localData == NULL) {

		CZLog(CZLogLevelLow, "Alloc local buffers for %s.", info->deviceName);

		lData = (CZDeviceInfoBandLocalData*)malloc(sizeof(*lData));
		if(lData == NULL) {
			return -1;
		}

		CZLog(CZLogLevelLow, "Alloc host pageable for %s.", info->deviceName);

		lData->memHostPage = (void*)malloc(CZ_COPY_BUF_SIZE);
		if(lData->memHostPage == NULL) {
			free(lData);
			return -1;
		}

		CZLog(CZLogLevelLow, "Host pageable is at 0x%08X.", lData->memHostPage);

		CZLog(CZLogLevelLow, "Alloc host pinned for %s.", info->deviceName);

		CZ_CUDA_CALL(cudaMallocHost((void**)&lData->memHostPin, CZ_COPY_BUF_SIZE),
			free(lData->memHostPage);
			free(lData);
			return -1);

		CZLog(CZLogLevelLow, "Host pinned is at 0x%08X.", lData->memHostPin);

		CZLog(CZLogLevelLow, "Alloc device buffer 1 for %s.", info->deviceName);

		CZ_CUDA_CALL(cudaMalloc((void**)&lData->memDevice1, CZ_COPY_BUF_SIZE),
			cudaFreeHost(lData->memHostPin);
			free(lData->memHostPage);
			free(lData);
			return -1);

		CZLog(CZLogLevelLow, "Device buffer 1 is at 0x%08X.", lData->memDevice1);

		CZLog(CZLogLevelLow, "Alloc device buffer 2 for %s.", info->deviceName);

		CZ_CUDA_CALL(cudaMalloc((void**)&lData->memDevice2, CZ_COPY_BUF_SIZE),
			cudaFree(lData->memDevice1);
			cudaFreeHost(lData->memHostPin);
			free(lData->memHostPage);
			free(lData);
			return -1);

		CZLog(CZLogLevelLow, "Device buffer 2 is at 0x%08X.", lData->memDevice2);

		info->band.localData = (void*)lData;
	}

	return 0;
}

/*!	\brief Free buffers for bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static int CZCudaCalcDeviceBandwidthFree(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {
	CZDeviceInfoBandLocalData *lData;

	if(info == NULL)
		return -1;

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;
	if(lData != NULL) {

		CZLog(CZLogLevelLow, "Free host pageable for %s.", info->deviceName);

		if(lData->memHostPage != NULL)
			free(lData->memHostPage);

		CZLog(CZLogLevelLow, "Free host pinned for %s.", info->deviceName);

		if(lData->memHostPin != NULL)
			cudaFreeHost(lData->memHostPin);

		CZLog(CZLogLevelLow, "Free device buffer 1 for %s.", info->deviceName);

		if(lData->memDevice1 != NULL)
			cudaFree(lData->memDevice1);

		CZLog(CZLogLevelLow, "Free device buffer 2 for %s.", info->deviceName);

		if(lData->memDevice2 != NULL)
			cudaFree(lData->memDevice2);

		CZLog(CZLogLevelLow, "Free local buffers for %s.", info->deviceName);

		free(lData);
	}
	info->band.localData = NULL;

	return 0;
}

/*!	\brief Reset results of bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static int CZCudaCalcDeviceBandwidthReset(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
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

#define CZ_COPY_MODE_H2D	0	/*!< Host to device data copy mode. */
#define CZ_COPY_MODE_D2H	1	/*!< Device to host data copy mode. */
#define CZ_COPY_MODE_D2D	2	/*!< Device to device data copy mode. */

/*!	\brief Run data transfer bandwidth tests.
	\return \a 0 in case of success, \a other is value in KiB/s.
*/
static float CZCudaCalcDeviceBandwidthTestCommon (
	struct CZDeviceInfo *info,	/*!<[in,out] CUDA-device information. */
	int mode,			/*!<[in] Run bandwidth test in one of modes. */
	int pinned			/*!<[in] Use pinned \a (=1) memory buffer instead of pagable \a (=0). */
) {
	CZDeviceInfoBandLocalData *lData;
	float timeMs = 0.0;
	float bandwidthKiBs = 0.0;
	cudaEvent_t start;
	cudaEvent_t stop;
	void *memHost;
	void *memDevice1;
	void *memDevice2;
	int i;

	if(info == NULL)
		return 0;

	CZ_CUDA_CALL(cudaEventCreate(&start),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&stop),
		cudaEventDestroy(start);
		return 0);

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;

	memHost = pinned? lData->memHostPin: lData->memHostPage;
	memDevice1 = lData->memDevice1;
	memDevice2 = lData->memDevice2;

	CZLog(CZLogLevelLow, "Starting %s test (%s) on %s.",
		(mode == CZ_COPY_MODE_H2D)? "host to device":
		(mode == CZ_COPY_MODE_D2H)? "device to host":
		(mode == CZ_COPY_MODE_D2D)? "device to device": "unknown",
		pinned? "pinned": "pageable",
		info->deviceName);

	for(i = 0; i < CZ_COPY_LOOPS_NUM; i++) {

		float loopMs = 0.0;

		CZ_CUDA_CALL(cudaEventRecord(start, 0),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		switch(mode) {
		case CZ_COPY_MODE_H2D:
			CZ_CUDA_CALL(cudaMemcpy(memDevice1, memHost, CZ_COPY_BUF_SIZE, cudaMemcpyHostToDevice),
				cudaEventDestroy(start);
				cudaEventDestroy(stop);
				return 0);
			break;

		case CZ_COPY_MODE_D2H:
			CZ_CUDA_CALL(cudaMemcpy(memHost, memDevice2, CZ_COPY_BUF_SIZE, cudaMemcpyDeviceToHost),
				cudaEventDestroy(start);
				cudaEventDestroy(stop);
				return 0);
			break;

		case CZ_COPY_MODE_D2D:
			CZ_CUDA_CALL(cudaMemcpy(memDevice2, memDevice1, CZ_COPY_BUF_SIZE, cudaMemcpyDeviceToDevice),
				cudaEventDestroy(start);
				cudaEventDestroy(stop);
				return 0);
			break;

		default: // WTF!
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0;
		}

		CZ_CUDA_CALL(cudaEventRecord(stop, 0),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		CZ_CUDA_CALL(cudaEventSynchronize(stop),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		CZ_CUDA_CALL(cudaEventElapsedTime(&loopMs, start, stop),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		timeMs += loopMs;
	}

	CZLog(CZLogLevelLow, "Test complete in %f ms.", timeMs);

	bandwidthKiBs = (
		1000 *
		(float)CZ_COPY_BUF_SIZE *
		(float)CZ_COPY_LOOPS_NUM
	) / (
		timeMs *
		(float)(1 << 10)
	);

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	return bandwidthKiBs;
}

/*!	\brief Run several bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static int CZCudaCalcDeviceBandwidthTest(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	info->band.copyHDPage = CZCudaCalcDeviceBandwidthTestCommon(info, CZ_COPY_MODE_H2D, 0);
	info->band.copyHDPin = CZCudaCalcDeviceBandwidthTestCommon(info, CZ_COPY_MODE_H2D, 1);
	info->band.copyDHPage = CZCudaCalcDeviceBandwidthTestCommon(info, CZ_COPY_MODE_D2H, 0);
	info->band.copyDHPin = CZCudaCalcDeviceBandwidthTestCommon(info, CZ_COPY_MODE_D2H, 1);
	info->band.copyDD = CZCudaCalcDeviceBandwidthTestCommon(info, CZ_COPY_MODE_D2D, 0);

	return 0;
}

/*!	\brief Prepare buffers bandwidth tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaPrepareDevice(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	if(!CZCudaIsInit())
		return -1;

	if(CZCudaCalcDeviceBandwidthAlloc(info) != 0)
		return -1;

	return 0;
}

/*!	\brief Calculate bandwidth information about CUDA-device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaCalcDeviceBandwidth(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	if(CZCudaCalcDeviceBandwidthReset(info) != 0)
		return -1;

	if(!CZCudaIsInit())
		return -1;

	if(CZCudaCalcDeviceBandwidthAlloc(info) != 0)
		return -1;

	if(CZCudaCalcDeviceBandwidthTest(info) != 0)
		return -1;

	return 0;
}

/*!	\brief Cleanup after test and bandwidth calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaCleanDevice(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	if(CZCudaCalcDeviceBandwidthFree(info) != 0)
		return -1;

	return 0;
}

/*!	\brief Reset results of preformance calculations.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static int CZCudaCalcDevicePerformanceReset(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	info->perf.calcFloat = 0;
	info->perf.calcDouble = 0;
	info->perf.calcInteger32 = 0;
	info->perf.calcInteger24 = 0;

	return 0;
}

/*!	\brief 16 MAD instructions for float point test.
*/
#define CZ_CALC_FMAD_16(a, b) \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \

/*!	\brief 256 MAD instructions for float point test.
*/
#define CZ_CALC_FMAD_256(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \
	CZ_CALC_FMAD_16(a, b) CZ_CALC_FMAD_16(a, b) \

/*!	\brief 16 DMAD instructions for double-precision test.
*/
#define CZ_CALC_DFMAD_16(a, b) \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \

/*	a = fma(b, a, b); b = fma(a, b, a); a = fma(b, a, b); b = fma(a, b, a); \
	a = fma(b, a, b); b = fma(a, b, a); a = fma(b, a, b); b = fma(a, b, a); \
	a = fma(b, a, b); b = fma(a, b, a); a = fma(b, a, b); b = fma(a, b, a); \
	a = fma(b, a, b); b = fma(a, b, a); a = fma(b, a, b); b = fma(a, b, a); \*/

/*!	\brief 256 MAD instructions for float point test.
*/
#define CZ_CALC_DFMAD_256(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \
	CZ_CALC_DFMAD_16(a, b) CZ_CALC_DFMAD_16(a, b) \

/*!	\brief 16 MAD instructions for 32-bit integer test.
*/
#define CZ_CALC_IMAD32_16(a, b) \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \
	a = b * a + b; b = a * b + a; a = b * a + b; b = a * b + a; \

/*!	\brief 256 MAD instructions for 32-bit integer test.
*/
#define CZ_CALC_IMAD32_256(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \
	CZ_CALC_IMAD32_16(a, b) CZ_CALC_IMAD32_16(a, b) \

/*!	\brief 16 MAD instructions for 24-bit integer test.
*/
#define CZ_CALC_IMAD24_16(a, b) \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \
	a = __umul24(b, a) + b; b = __umul24(a, b) + a; \

/*!	\brief 256 MAD instructions for 24-bit integer test.
*/
#define CZ_CALC_IMAD24_256(a, b) \
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\
	CZ_CALC_IMAD24_16(a, b) CZ_CALC_IMAD24_16(a, b)\

#define CZ_CALC_MODE_FLOAT	0	/*!< Single-precision float point test mode. */
#define CZ_CALC_MODE_DOUBLE	1	/*!< Double-precision float point test mode. */
#define CZ_CALC_MODE_INTEGER32	2	/*!< 32-bit integer test mode. */
#define CZ_CALC_MODE_INTEGER24	3	/*!< 24-bit integer test mode. */

/*!	\brief GPU code for float point test.
*/
__global__ void CZCudaCalcKernelFloat(
	void *buf			/*!<[in] Data buffer. */
) {
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	float *arr = (float*)buf;
	float val1 = index;
	float val2 = arr[index];
	int i;

	for(i = 0; i < CZ_CALC_BLOCK_LOOPS; i++) {
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
		CZ_CALC_FMAD_256(val1, val2);
	}

	arr[index] = val1 + val2;
}

/*!	\brief GPU code for double-precision test.
*/
__global__ void CZCudaCalcKernelDouble(
	void *buf			/*!<[in] Data buffer. */
) {
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	double *arr = (double*)buf;
	double val1 = index;
	double val2 = arr[index];
	int i;

	for(i = 0; i < CZ_CALC_BLOCK_LOOPS; i++) {
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
		CZ_CALC_DFMAD_256(val1, val2);
	}

	arr[index] = val1 + val2;
}

/*!	\brief GPU code for 32-bit integer test.
*/
__global__ void CZCudaCalcKernelInteger32(
	void *buf			/*!<[in] Data buffer. */
) {
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int *arr = (int*)buf;
	int val1 = index;
	int val2 = arr[index];
	int i;

	for(i = 0; i < CZ_CALC_BLOCK_LOOPS; i++) {
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
		CZ_CALC_IMAD32_256(val1, val2);
	}

	arr[index] = val1 + val2;
}

/*!	\brief GPU code for 24-bit integer test.
*/
__global__ void CZCudaCalcKernelInteger24(
	void *buf			/*!<[in] Data buffer. */
) {
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int *arr = (int*)buf;
	int val1 = index;
	int val2 = arr[index];
	int i;

	for(i = 0; i < CZ_CALC_BLOCK_LOOPS; i++) {
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
		CZ_CALC_IMAD24_256(val1, val2);
	}

	arr[index] = val1 + val2;
}

/*!	\brief Run GPU calculation performace tests.
	\return \a 0 in case of success, \a -1 in case of error.
*/
static float CZCudaCalcDevicePerformanceTest(
	struct CZDeviceInfo *info,	/*!<[in,out] CUDA-device information. */
	int mode			/*!<[in] Run performance test in one of modes. */
) {
	CZDeviceInfoBandLocalData *lData;
	float timeMs = 0.0;
	float performanceKOPs = 0.0;
	cudaEvent_t start;
	cudaEvent_t stop;
	int blocksNum = info->heavyMode? info->core.muliProcCount: 1;
	int i;

	if(info == NULL)
		return 0;

	CZ_CUDA_CALL(cudaEventCreate(&start),
		return 0);

	CZ_CUDA_CALL(cudaEventCreate(&stop),
		cudaEventDestroy(start);
		return 0);

	lData = (CZDeviceInfoBandLocalData*)info->band.localData;

	int threadsNum = info->core.maxThreadsPerBlock;
	if(threadsNum == 0) {
		int warpSize = info->core.SIMDWidth;
		if(warpSize == 0)
			warpSize = CZ_DEF_WARP_SIZE;
		threadsNum = warpSize * 2;
		if(threadsNum > CZ_DEF_THREADS_MAX)
			threadsNum = CZ_DEF_THREADS_MAX;
	}

	CZLog(CZLogLevelLow, "Starting %s test on %s on %d block(s) %d thread(s) each.",
		(mode == CZ_CALC_MODE_FLOAT)? "single-precision float":
		(mode == CZ_CALC_MODE_DOUBLE)? "double-precision float":
		(mode == CZ_CALC_MODE_INTEGER32)? "32-bit integer":
		(mode == CZ_CALC_MODE_INTEGER24)? "24-bit integer": "unknown",
		info->deviceName,
		blocksNum,
		threadsNum);

	for(i = 0; i < CZ_CALC_LOOPS_NUM; i++) {

		float loopMs = 0.0;

		CZ_CUDA_CALL(cudaEventRecord(start, 0),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		switch(mode) {
		case CZ_CALC_MODE_FLOAT:
			CZCudaCalcKernelFloat<<<blocksNum, threadsNum>>>(lData->memDevice1);
			break;

		case CZ_CALC_MODE_DOUBLE:
			CZCudaCalcKernelDouble<<<blocksNum, threadsNum>>>(lData->memDevice1);
			break;

		case CZ_CALC_MODE_INTEGER32:
			CZCudaCalcKernelInteger32<<<blocksNum, threadsNum>>>(lData->memDevice1);
			break;

		case CZ_CALC_MODE_INTEGER24:
			CZCudaCalcKernelInteger24<<<blocksNum, threadsNum>>>(lData->memDevice1);
			break;

		default: // WTF!
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0;
		}

		CZ_CUDA_CALL(cudaGetLastError(),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		CZ_CUDA_CALL(cudaEventRecord(stop, 0),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		CZ_CUDA_CALL(cudaEventSynchronize(stop),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		CZ_CUDA_CALL(cudaEventElapsedTime(&loopMs, start, stop),
			cudaEventDestroy(start);
			cudaEventDestroy(stop);
			return 0);

		timeMs += loopMs;
	}

	CZLog(CZLogLevelLow, "Test complete in %f ms.", timeMs);

	performanceKOPs = (
		(float)info->core.muliProcCount *
		(float)CZ_CALC_LOOPS_NUM *
		(float)threadsNum *
		(float)CZ_CALC_BLOCK_LOOPS *
		(float)CZ_CALC_OPS_NUM *
		(float)CZ_CALC_BLOCK_SIZE *
		(float)CZ_CALC_BLOCK_NUM
	) / (float)timeMs;

	cudaEventDestroy(start);
	cudaEventDestroy(stop);

	return performanceKOPs;
}

/*!	\brief Calculate performance information about CUDA-device.
	\return \a 0 in case of success, \a -1 in case of error.
*/
int CZCudaCalcDevicePerformance(
	struct CZDeviceInfo *info	/*!<[in,out] CUDA-device information. */
) {

	if(info == NULL)
		return -1;

	if(CZCudaCalcDevicePerformanceReset(info) != 0)
		return -1;

	if(!CZCudaIsInit())
		return -1;

	info->perf.calcFloat = CZCudaCalcDevicePerformanceTest(info, CZ_CALC_MODE_FLOAT);
	if(((info->major > 1)) ||
		((info->major == 1) && (info->minor >= 3)))
		info->perf.calcDouble = CZCudaCalcDevicePerformanceTest(info, CZ_CALC_MODE_DOUBLE);
	info->perf.calcInteger32 = CZCudaCalcDevicePerformanceTest(info, CZ_CALC_MODE_INTEGER32);
	info->perf.calcInteger24 = CZCudaCalcDevicePerformanceTest(info, CZ_CALC_MODE_INTEGER24);

	return 0;
}
