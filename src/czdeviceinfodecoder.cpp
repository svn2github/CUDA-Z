/*!	\file czdeviceinfodecoder.cpp
	\brief CUDA device information decoder source file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#include <time.h>

#include "version.h"
#include "log.h"
#include "platform.h"
#include "czdeviceinfodecoder.h"

/*!	\class CZCudaDeviceInfoDecoder
	\brief This class implements a decoder for CUDA-device information.
*/

/*!	\brief Creates CUDA-device information decoder.
*/
CZCudaDeviceInfoDecoder::CZCudaDeviceInfoDecoder(
	CZCudaDeviceInfo &info,		/*!<[in] CUDA device information. */
	QObject *parent			/*!<[in,out] Parent of CUDA device information. */
) 	: QObject(parent) {
	m_info = info.info();
}

/*!	\brief Creates CUDA-device information decoder.
*/
CZCudaDeviceInfoDecoder::CZCudaDeviceInfoDecoder(
	struct CZDeviceInfo &info,	/*!<[in] CUDA device information. */
	QObject *parent			/*!<[in,out] Parent of CUDA device information. */
) 	: QObject(parent) {
	m_info = info;
}

/*!	\brief Destroys cuda information decoder container.
*/
CZCudaDeviceInfoDecoder::~CZCudaDeviceInfoDecoder() {
}

typedef const QString (*CZCudaDeviceInfoDecoderFunction)(const struct CZDeviceInfo &info);

struct CZCudaDeviceInfoDecoderInfo {
	int id;
	const QString name;
	CZCudaDeviceInfoDecoderFunction getValue;
};

static const QString funcNull(const struct CZDeviceInfo &info) {
	return QString("--");
}

#define nameTabCore		QT_TR_NOOP("Core")
#define funcTabCore		funcNull

#define nameTabMemory		QT_TR_NOOP("Memory")
#define funcTabMemory		funcNull

#define nameTabPerformance	QT_TR_NOOP("Performance")
#define funcTabPerformance	funcNull

#define nameDrvVersion		QT_TR_NOOP("Driver Version")
static const QString funcDrvVersion(const struct CZDeviceInfo &info) {
	QString version;
	if(strlen(info.drvVersion) != 0) {
		version = QString(info.drvVersion);
		version += info.tccDriver? " (TCC)": "";
	} else {
		version = QObject::tr("Unknown");
	}
	return version;
}

#define nameDrvDllVersion	QT_TR_NOOP("Driver Dll Version")
static const QString funcDrvDllVersion(const struct CZDeviceInfo &info) {
	QString version;
	if(info.drvDllVer == 0) {
		version = QObject::tr("Unknown");
	} else {
		version = QString("%1.%2")
			.arg(info.drvDllVer / 1000)
			.arg(info.drvDllVer % 1000);
	}
	if(strlen(info.drvDllVerStr) != 0) {
		version += " (" + QString(info.drvDllVerStr) + ")";
	}
	return version;
}

#define nameRtDllVersion	QT_TR_NOOP("Runtime Dll Version")
static const QString funcRtDllVersion(const struct CZDeviceInfo &info) {
	QString version;
	if(info.rtDllVer == 0) {
		version = QObject::tr("Unknown");
	} else {
		version = QString("%1.%2")
			.arg(info.rtDllVer / 1000)
			.arg(info.rtDllVer % 1000);
	}
	if(strlen(info.rtDllVerStr) != 0) {
		version += " (" + QString(info.rtDllVerStr) + ")";
	}
	return version;
}

#define nameName		QT_TR_NOOP("Name")
static const QString funcName(const struct CZDeviceInfo &info) {
	return QString(info.deviceName);
}

#define nameCapability		QT_TR_NOOP("Compute Capability")
static const QString funcCapability(const struct CZDeviceInfo &info) {
	QString archName(info.archName);
	if(archName.isEmpty())
		return QString("%1.%2").arg(info.major).arg(info.minor);
	else
		return QString("%1.%2 (%3)").arg(info.major).arg(info.minor).arg(archName);
}

#define nameClock		QT_TR_NOOP("Clock Rate")
static const QString funcClock(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1000(info.core.clockRate, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("Hz"));
}

#define namePCIInfo		QT_TR_NOOP("PCI Location")
static const QString funcPCIInfo(const struct CZDeviceInfo &info) {
	return QString("%1:%2:%3")
		.arg(info.core.pciDomainID)
		.arg(info.core.pciBusID)
		.arg(info.core.pciDeviceID);
}

#define nameMultiProc		QT_TR_NOOP("Multiprocessors")
static const QString funcMultiProc(const struct CZDeviceInfo &info) {
	QString text;
	if(info.core.muliProcCount == 0)
		text = QObject::tr("Unknown");
	else {
		if(info.core.cudaCores == 0)
			text = QString(info.core.muliProcCount);
		else
			text = QObject::tr("%1 (%2 Cores)")
				.arg(info.core.muliProcCount)
				.arg(info.core.cudaCores);
	}
	return text;
}

#define nameThreadsMulti	QT_TR_NOOP("Threads Per Multiproc.")
static const QString funcThreadsMulti(const struct CZDeviceInfo &info) {
	return QString("%1").arg(info.core.maxThreadsPerMultiProcessor);
}

#define nameWarp		QT_TR_NOOP("Warp Size")
static const QString funcWarp(const struct CZDeviceInfo &info) {
	return QString("%1").arg(info.core.SIMDWidth);
}

#define nameRegsBlock		QT_TR_NOOP("Regs Per Block")
static const QString funcRegsBlock(const struct CZDeviceInfo &info) {
	return QString("%1").arg(info.core.regsPerBlock);
}

#define nameThreadsBlock	QT_TR_NOOP("Threads Per Block")
static const QString funcThreadsBlock(const struct CZDeviceInfo &info) {
	return QString("%1").arg(info.core.maxThreadsPerBlock);
}

#define nameThreadsDim		QT_TR_NOOP("Threads Dimensions")
static const QString funcThreadsDim(const struct CZDeviceInfo &info) {
	return QString("%1 x %2 x %3").arg(info.core.maxThreadsDim[0]).arg(info.core.maxThreadsDim[1]).arg(info.core.maxThreadsDim[2]);
}

#define nameGridDim		QT_TR_NOOP("Grid Dimensions")
static const QString funcGridDim(const struct CZDeviceInfo &info) {
	return QString("%1 x %2 x %3").arg(info.core.maxGridSize[0]).arg(info.core.maxGridSize[1]).arg(info.core.maxGridSize[2]);
}

#define nameWatchdog		QT_TR_NOOP("Watchdog Enabled")
static const QString funcWatchdog(const struct CZDeviceInfo &info) {
	if(info.core.watchdogEnabled == -1)
		return QObject::tr("Unknown");
	else
		return info.core.watchdogEnabled? QObject::tr("Yes"): QObject::tr("No");
}

#define nameIntegrated		QT_TR_NOOP("Integrated GPU")
static const QString funcIntegrated(const struct CZDeviceInfo &info) {
	return info.core.integratedGpu? QObject::tr("Yes"): QObject::tr("No");
}

#define nameConcurrentKernels	QT_TR_NOOP("Concurrent Kernels")
static const QString funcConcurrentKernels(const struct CZDeviceInfo &info) {
	return info.core.concurrentKernels? QObject::tr("Yes"): QObject::tr("No");
}

#define nameComputeMode		QT_TR_NOOP("Compute Mode")
static const QString funcComputeMode(const struct CZDeviceInfo &info) {
	QString text;
	if(info.core.computeMode == CZComputeModeDefault) {
		text = QObject::tr("Default");
	} else if(info.core.computeMode == CZComputeModeExclusive) {
		text = QObject::tr("Compute-exclusive");
	} else if(info.core.computeMode == CZComputeModeProhibited) {
		text = QObject::tr("Compute-prohibited");
	} else {
		text = QObject::tr("Unknown");
	}
	return text;
}

#define nameStreamPriorities	QT_TR_NOOP("Stream Priorities")
static const QString funcStreamPriorities(const struct CZDeviceInfo &info) {
	return info.core.streamPrioritiesSupported? QObject::tr("Yes"): QObject::tr("No");
}

#define nameTotalGlobal		QT_TR_NOOP("Total Global")
static const QString funcTotalGlobal(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1024(info.mem.totalGlobal, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B"));
}

#define nameBusWidth		QT_TR_NOOP("Bus Width")
static const QString funcBusWidth(const struct CZDeviceInfo &info) {
	return QObject::tr("%1 bits").arg(info.mem.memoryBusWidth);
}

#define nameMemClock		QT_TR_NOOP("Clock Rate")
static const QString funcMemClock(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1000(info.mem.memoryClockRate, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("Hz"));
}

#define nameErrorCorrection	QT_TR_NOOP("Error Correction")
static const QString funcErrorCorrection(const struct CZDeviceInfo &info) {
	return info.mem.errorCorrection? QObject::tr("Yes"): QObject::tr("No");
}

#define nameL2CasheSize		QT_TR_NOOP("L2 Cache Size")
static const QString funcL2CasheSize(const struct CZDeviceInfo &info) {
	return info.mem.l2CacheSize? CZCudaDeviceInfoDecoder::getValue1024(info.mem.l2CacheSize, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B")): QObject::tr("No");
}

#define nameShared		QT_TR_NOOP("Shared Per Block")
static const QString funcShared(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1024(info.mem.sharedPerBlock, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B"));
}

#define namePitch		QT_TR_NOOP("Pitch")
static const QString funcPitch(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1024(info.mem.maxPitch, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B"));
}

#define nameTotalConst		QT_TR_NOOP("Total Constant")
static const QString funcTotalConst(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1024(info.mem.totalConst, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B"));
}

#define nameTextureAlign	QT_TR_NOOP("Texture Alignment")
static const QString funcTextureAlign(const struct CZDeviceInfo &info) {
	return CZCudaDeviceInfoDecoder::getValue1024(info.mem.textureAlignment, CZCudaDeviceInfoDecoder::prefixNothing, QObject::tr("B"));
}

#define nameTexture1D		QT_TR_NOOP("Texture 1D Size")
static const QString funcTexture1D(const struct CZDeviceInfo &info) {
	return QString("%1")
		.arg((double)info.mem.texture1D[0]);
}

#define nameTexture2D		QT_TR_NOOP("Texture 2D Size")
static const QString funcTexture2D(const struct CZDeviceInfo &info) {
	return QString("%1 x %2")
		.arg((double)info.mem.texture2D[0])
		.arg((double)info.mem.texture2D[1]);
}

#define nameTexture3D		QT_TR_NOOP("Texture 3D Size")
static const QString funcTexture3D(const struct CZDeviceInfo &info) {
	return QString("%1 x %2 x %3")
		.arg((double)info.mem.texture3D[0])
		.arg((double)info.mem.texture3D[1])
		.arg((double)info.mem.texture3D[2]);
}

#define nameGpuOverlap		QT_TR_NOOP("GPU Overlap")
static const QString funcGpuOverlap(const struct CZDeviceInfo &info) {
	return info.mem.gpuOverlap? QObject::tr("Yes"): QObject::tr("No");
}

#define nameMapHostMemory	QT_TR_NOOP("Map Host Memory")
static const QString funcMapHostMemory(const struct CZDeviceInfo &info) {
	return info.mem.mapHostMemory? QObject::tr("Yes"): QObject::tr("No");
}

#define nameUnifiedAddressing	QT_TR_NOOP("Unified Addressing")
static const QString funcUnifiedAddressing(const struct CZDeviceInfo &info) {
	return info.mem.unifiedAddressing? QObject::tr("Yes"): QObject::tr("No");
}

#define nameAsyncEngine		QT_TR_NOOP("Async Engine")
static const QString funcAsyncEngine(const struct CZDeviceInfo &info) {
	return (info.mem.asyncEngineCount == 0)? QObject::tr("%1 No").arg(info.mem.asyncEngineCount):
		(info.mem.asyncEngineCount == 1)? QObject::tr("%1 Yes, Unidirectional").arg(info.mem.asyncEngineCount):
		QObject::tr("%1 Yes, Bidirectional").arg(info.mem.asyncEngineCount);
}

#define nameMemoryCopy		QT_TR_NOOP("Memory Copy")
#define funcMemoryCopy		funcNull

#define nameHostPinnedToDevice	QT_TR_NOOP("Host Pinned to Device")
static const QString funcHostPinnedToDevice(const struct CZDeviceInfo &info) {
	if(info.band.copyHDPin == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1024(info.band.copyHDPin, CZCudaDeviceInfoDecoder::prefixKibi, QObject::tr("B/s"));
}

#define nameHostPageableToDevice	QT_TR_NOOP("Host Pageable to Device")
static const QString funcHostPageableToDevice(const struct CZDeviceInfo &info) {
	if(info.band.copyHDPage == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1024(info.band.copyHDPage, CZCudaDeviceInfoDecoder::prefixKibi, QObject::tr("B/s"));
}

#define nameDeviceToHostPinned	QT_TR_NOOP("Device to Host Pinned")
static const QString funcDeviceToHostPinned(const struct CZDeviceInfo &info) {
	if(info.band.copyDHPin == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1024(info.band.copyDHPin, CZCudaDeviceInfoDecoder::prefixKibi, QObject::tr("B/s"));
}

#define nameDeviceToHostPageable	QT_TR_NOOP("Device to Host Pageable")
static const QString funcDeviceToHostPageable(const struct CZDeviceInfo &info) {
	if(info.band.copyDHPage == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1024(info.band.copyDHPage, CZCudaDeviceInfoDecoder::prefixKibi, QObject::tr("B/s"));
}

#define nameDeviceToDevice	QT_TR_NOOP("Device to Device")
static const QString funcDeviceToDevice(const struct CZDeviceInfo &info) {
	if(info.band.copyDD == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1024(info.band.copyDD, CZCudaDeviceInfoDecoder::prefixKibi, QObject::tr("B/s"));
}

#define nameCorePerformance	QT_TR_NOOP("GPU Core Performance")
#define funcCorePerformance	funcNull

#define nameFloatRate		QT_TR_NOOP("Single-precision Float")
static const QString funcFloatRate(const struct CZDeviceInfo &info) {
	if(info.perf.calcFloat == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1000(info.perf.calcFloat, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("flop/s"));
}

#define nameDoubleRate		QT_TR_NOOP("Double-precision Float")
static const QString funcDoubleRate(const struct CZDeviceInfo &info) {
	if(((info.major > 1)) ||
		((info.major == 1) && (info.minor >= 3))) {
		if(info.perf.calcDouble == 0)
			return QString("--");
		else
			return CZCudaDeviceInfoDecoder::getValue1000(info.perf.calcDouble, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("flop/s"));
	} else {
		return QObject::tr("Not Supported");
	}
}

#define nameInt64Rate		QT_TR_NOOP("64-bit Integer")
static const QString funcInt64Rate(const struct CZDeviceInfo &info) {
	if(info.perf.calcInteger64 == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1000(info.perf.calcInteger64, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("iop/s"));
}

#define nameInt32Rate		QT_TR_NOOP("32-bit Integer")
static const QString funcInt32Rate(const struct CZDeviceInfo &info) {
	if(info.perf.calcInteger32 == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1000(info.perf.calcInteger32, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("iop/s"));
}

#define nameInt24Rate		QT_TR_NOOP("24-bit Integer")
static const QString funcInt24Rate(const struct CZDeviceInfo &info) {
	if(info.perf.calcInteger24 == 0)
		return QString("--");
	else
		return CZCudaDeviceInfoDecoder::getValue1000(info.perf.calcInteger24, CZCudaDeviceInfoDecoder::prefixKilo, QObject::tr("iop/s"));
}

#define INFO(_id_)		{CZCudaDeviceInfoDecoder::id ## _id_, name ## _id_, func ## _id_}
static const CZCudaDeviceInfoDecoderInfo s_infoTab[] = {
	INFO(TabCore),
	INFO(TabMemory),
	INFO(TabPerformance),

	INFO(DrvVersion),
	INFO(DrvDllVersion),
	INFO(RtDllVersion),
	INFO(Name),
	INFO(Capability),
	INFO(Clock),
	INFO(PCIInfo),
	INFO(MultiProc),
	INFO(ThreadsMulti),
	INFO(Warp),
	INFO(RegsBlock),
	INFO(ThreadsBlock),
	INFO(ThreadsDim),
	INFO(GridDim),
	INFO(Watchdog),
	INFO(Integrated),
	INFO(ConcurrentKernels),
	INFO(ComputeMode),
	INFO(StreamPriorities),

	INFO(TotalGlobal),
	INFO(BusWidth),
	INFO(MemClock),
	INFO(ErrorCorrection),
	INFO(L2CasheSize),
	INFO(Shared),
	INFO(Pitch),
	INFO(TotalConst),
	INFO(TextureAlign),
	INFO(Texture1D),
	INFO(Texture2D),
	INFO(Texture3D),
	INFO(GpuOverlap),
	INFO(MapHostMemory),
	INFO(UnifiedAddressing),
	INFO(AsyncEngine),

	INFO(MemoryCopy),
	INFO(HostPinnedToDevice),
	INFO(HostPageableToDevice),
	INFO(DeviceToHostPinned),
	INFO(DeviceToHostPageable),
	INFO(DeviceToDevice),
	INFO(CorePerformance),
	INFO(FloatRate),
	INFO(DoubleRate),
	INFO(Int64Rate),
	INFO(Int32Rate),
	INFO(Int24Rate),
};

/*!	\brief Get a name for the field of information
	\returns Name of cuda information field 
*/
const QString CZCudaDeviceInfoDecoder::getName(
	int id				/*!<[in] Field id. */
) const {

	if(id >= idMax)
		return funcNull(m_info);

	for(int i = 0; i < sizeof(s_infoTab) / sizeof(s_infoTab[0]); i++) {
		if(s_infoTab[i].id == id) {
			return QString(s_infoTab[i].name);
		}
	}

	return funcNull(m_info);
}

/*!	\brief Get a value for the field of information
	\returns Value of cuda information field 
*/
const QString CZCudaDeviceInfoDecoder::getValue(
	int id				/*!<[in] Field id. */
) const {

	if(id >= idMax)
		return funcNull(m_info);

	for(int i = 0; i < sizeof(s_infoTab) / sizeof(s_infoTab[0]); i++) {
		if(s_infoTab[i].id == id) {
			return s_infoTab[i].getValue(m_info);
		}
	}

	return funcNull(m_info);
}

/*!	\brief This function returns value and unit in SI format.
*/
const QString CZCudaDeviceInfoDecoder::getValue1000(
	double value,			/*!<[in] Value to print. */
	int valuePrefix,		/*!<[in] Value current prefix. */
	QString unitBase		/*!<[in] Unit base string. */
) {
	const int prefixBase = 1000;
	int resPrefix = valuePrefix;

	static const char *prefixTab[prefixSiMax + 1] = {
		"",	/* prefixNothing */
		"k",	/* prefixKilo */
		"M",	/* prefixMega */
		"G",	/* prefixGiga */
		"T",	/* prefixTera */
		"P",	/* prefixPeta */
		"E",	/* prefixExa */
		"Z",	/* prefixZetta */
		"Y",	/* prefixYotta */
	};

	while((value > (10 * prefixBase)) && (resPrefix < prefixSiMax)) {
		value /= prefixBase;
		resPrefix++;
	}

	return QString("%1 %2%3").arg(value).arg(prefixTab[resPrefix]).arg(unitBase);
}

/*!	\brief This function returns value and unit in IEC 60027 format.
*/
const QString CZCudaDeviceInfoDecoder::getValue1024(
	double value,			/*!<[in] Value to print. */
	int valuePrefix,		/*!<[in] Value current prefix. */
	QString unitBase		/*!<[in] Unit base string. */
) {
	const int prefixBase = 1024;
	int resPrefix = valuePrefix;

	static const char *prefixTab[prefixIecMax + 1] = {
		"",	/* prefixNothing */
		"Ki",	/* prefixKibi */
		"Mi",	/* prefixMebi */
		"Gi",	/* prefixGibi */
		"Ti",	/* prefixTebi */
		"Pi",	/* prefixPebi */
		"Ei",	/* prefixExbi */
		"Zi",	/* prefixZebi */
		"Yi",	/* prefixYobi */
	};

	while((value > (10 * prefixBase)) && (resPrefix < prefixIecMax)) {
		value /= prefixBase;
		resPrefix++;
	}

	return QString("%1 %2%3").arg(value).arg(prefixTab[resPrefix]).arg(unitBase);
}

#define CZ_TXT_EXPORT(_id_)		out += getName(id ## _id_) + ": " + getValue(id ## _id_) + "\n"
#define CZ_TXT_EXPORT_TAB(_id_)		out += "\t" + getName(id ## _id_) + ": " + getValue(id ## _id_) + "\n"

/*!	\brief Generate plane text report.
*/
const QString CZCudaDeviceInfoDecoder::generateTextReport() const {

	QString out;
	QString title = tr(CZ_NAME_SHORT " Report");
	QString subtitle;

	out += title;
	out += "\n";
	for(int i = 0; i < title.size(); i++)
		out += "=";
	out += "\n";
	out += tr("Version") + ": " CZ_VERSION + " " + QString::number(QSysInfo::WordSize) + " bit";
#ifdef CZ_VER_STATE
	out += " " + tr("Built") + " " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out += " " CZ_ORG_URL_MAINPAGE "\n";
	out += tr("OS Version") + ": " + getOSVersion() + "\n";

	CZ_TXT_EXPORT(DrvVersion);
	CZ_TXT_EXPORT(DrvDllVersion);
	CZ_TXT_EXPORT(RtDllVersion);
	out += "\n";

	subtitle = tr("Core Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
	CZ_TXT_EXPORT_TAB(Name);
	CZ_TXT_EXPORT_TAB(Capability);
	CZ_TXT_EXPORT_TAB(Clock);
	CZ_TXT_EXPORT_TAB(PCIInfo);
	CZ_TXT_EXPORT_TAB(MultiProc);
	CZ_TXT_EXPORT_TAB(ThreadsMulti);
	CZ_TXT_EXPORT_TAB(Warp);
	CZ_TXT_EXPORT_TAB(RegsBlock);
	CZ_TXT_EXPORT_TAB(ThreadsBlock);
	CZ_TXT_EXPORT_TAB(ThreadsDim);
	CZ_TXT_EXPORT_TAB(GridDim);
	CZ_TXT_EXPORT_TAB(Watchdog);
	CZ_TXT_EXPORT_TAB(Integrated);
	CZ_TXT_EXPORT_TAB(ConcurrentKernels);
	CZ_TXT_EXPORT_TAB(ComputeMode);
	CZ_TXT_EXPORT_TAB(StreamPriorities);
	out += "\n";

	subtitle = tr("Memory Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
	CZ_TXT_EXPORT_TAB(TotalGlobal);
	CZ_TXT_EXPORT_TAB(BusWidth);
	CZ_TXT_EXPORT_TAB(MemClock);
	CZ_TXT_EXPORT_TAB(ErrorCorrection);
	CZ_TXT_EXPORT_TAB(L2CasheSize);
	CZ_TXT_EXPORT_TAB(Shared);
	CZ_TXT_EXPORT_TAB(Pitch);
	CZ_TXT_EXPORT_TAB(TotalConst);
	CZ_TXT_EXPORT_TAB(TextureAlign);
	CZ_TXT_EXPORT_TAB(Texture1D);
	CZ_TXT_EXPORT_TAB(Texture2D);
	CZ_TXT_EXPORT_TAB(Texture3D);
	CZ_TXT_EXPORT_TAB(GpuOverlap);
	CZ_TXT_EXPORT_TAB(MapHostMemory);
	CZ_TXT_EXPORT_TAB(UnifiedAddressing);
	CZ_TXT_EXPORT_TAB(AsyncEngine);
	out += "\n";

	subtitle = tr("Performance Information");
	out += subtitle + "\n";
	for(int i = 0; i < subtitle.size(); i++)
		out += "-";
	out += "\n";
	out += tr("Memory Copy") + "\n";
	CZ_TXT_EXPORT_TAB(HostPinnedToDevice);
	CZ_TXT_EXPORT_TAB(HostPageableToDevice);
	CZ_TXT_EXPORT_TAB(DeviceToHostPinned);
	CZ_TXT_EXPORT_TAB(DeviceToHostPageable);
	CZ_TXT_EXPORT_TAB(DeviceToDevice);
	out += tr("GPU Core Performance") + "\n";
	CZ_TXT_EXPORT_TAB(FloatRate);
	CZ_TXT_EXPORT_TAB(DoubleRate);
	CZ_TXT_EXPORT_TAB(Int64Rate);
	CZ_TXT_EXPORT_TAB(Int32Rate);
	CZ_TXT_EXPORT_TAB(Int24Rate);
	out += "\n";

	time_t t;
	time(&t);
	out += QString("%1: %2").arg(tr("Generated")).arg(ctime(&t)) + "\n";

	return out;
}

#define CZ_HTML_EXPORT(_id_)		out += "<b>" + getName(id ## _id_) + "</b>: " + getValue(id ## _id_) + "<br/>\n"
#define CZ_HTML_EXPORT_TAB(_id_)	out += "<tr><th>" + getName(id ## _id_) + "</th><td>" + getValue(id ## _id_) + "</td></tr>\n"

/*!	\brief Generate HTML v5 report.
*/
const QString CZCudaDeviceInfoDecoder::generateHTMLReport() const {

	QString out;
	QString title = tr(CZ_NAME_SHORT " Report");

	out += "<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<title>" + title + "</title>\n"
		"<meta charset=\"utf-8\">\n"
		"<style type=\"text/css\">\n"

		"@charset \"utf-8\";\n"
		"body { font-size: 12px; font-family: Verdana, Arial, Helvetica, sans-serif; font-weight: normal; font-style: normal; background: #fff; }\n"
		"h1 { font-size: 15px; color: #690; }\n"
		"h2 { font-size: 13px; color: #690; }\n"
		"table, td, th { border: 1px solid #000; }\n"
		"table { border-collapse: collapse; width: 500px; }\n"
		"th { background-color: #deb; text-align: left; }\n"
		"td { width: 50%; }\n"
		"a:link { color: #9c3; text-decoration: none; }\n"
		"a:visited { color: #690; text-decoration: none; }\n"
		"a:hover { color: #9c3; text-decoration: underline; }\n"
		"a:active { color: #9c3; text-decoration: underline; }\n"

		"</style>\n"
		"</head>\n"
		"<body>\n";

	out += "<h1>" + title + "</h1>\n";
	out += "<p><small>";
	out += "<b>" + tr("Version")+ ":</b> " CZ_VERSION " " + QString::number(QSysInfo::WordSize) + " bit";
#ifdef CZ_VER_STATE
	out += " <b>" + tr("Built") + "</b> " CZ_DATE " " CZ_TIME;
#endif//CZ_VER_STATE
	out += " <a href=\"" CZ_ORG_URL_MAINPAGE "\">" CZ_ORG_URL_MAINPAGE "</a><br/>\n";
	out += "<b>" + tr("OS Version") + ":</b> " + getOSVersion() + "<br/>\n";

	CZ_HTML_EXPORT(DrvVersion);
	CZ_HTML_EXPORT(DrvDllVersion);
	CZ_HTML_EXPORT(RtDllVersion);
	out += "</small></p>\n";

	out += "<h2>" + tr("Core Information") + "</h2>\n";
	out += "<table>\n";
	CZ_HTML_EXPORT_TAB(Name);
	CZ_HTML_EXPORT_TAB(Capability);
	CZ_HTML_EXPORT_TAB(Clock);
	CZ_HTML_EXPORT_TAB(PCIInfo);
	CZ_HTML_EXPORT_TAB(MultiProc);
	CZ_HTML_EXPORT_TAB(ThreadsMulti);
	CZ_HTML_EXPORT_TAB(Warp);
	CZ_HTML_EXPORT_TAB(RegsBlock);
	CZ_HTML_EXPORT_TAB(ThreadsBlock);
	CZ_HTML_EXPORT_TAB(ThreadsDim);
	CZ_HTML_EXPORT_TAB(GridDim);
	CZ_HTML_EXPORT_TAB(Watchdog);
	CZ_HTML_EXPORT_TAB(Integrated);
	CZ_HTML_EXPORT_TAB(ConcurrentKernels);
	CZ_HTML_EXPORT_TAB(ComputeMode);
	CZ_HTML_EXPORT_TAB(StreamPriorities);
	out += "</table>\n";

	out += "<h2>" + tr("Memory Information") + "</h2>\n";
	out += "<table>\n";
	CZ_HTML_EXPORT_TAB(TotalGlobal);
	CZ_HTML_EXPORT_TAB(BusWidth);
	CZ_HTML_EXPORT_TAB(MemClock);
	CZ_HTML_EXPORT_TAB(ErrorCorrection);
	CZ_HTML_EXPORT_TAB(L2CasheSize);
	CZ_HTML_EXPORT_TAB(Shared);
	CZ_HTML_EXPORT_TAB(Pitch);
	CZ_HTML_EXPORT_TAB(TotalConst);
	CZ_HTML_EXPORT_TAB(TextureAlign);
	CZ_HTML_EXPORT_TAB(Texture1D);
	CZ_HTML_EXPORT_TAB(Texture2D);
	CZ_HTML_EXPORT_TAB(Texture3D);
	CZ_HTML_EXPORT_TAB(GpuOverlap);
	CZ_HTML_EXPORT_TAB(MapHostMemory);
	CZ_HTML_EXPORT_TAB(UnifiedAddressing);
	CZ_HTML_EXPORT_TAB(AsyncEngine);
	out += "</table>\n";

	out += "<h2>" + tr("Performance Information") + "</h2>\n";
	out += "<table>\n";
	out += "<tr><th colspan=\"2\">" + tr("Memory Copy") + "</th></tr>\n";
	CZ_HTML_EXPORT_TAB(HostPinnedToDevice);
	CZ_HTML_EXPORT_TAB(HostPageableToDevice);
	CZ_HTML_EXPORT_TAB(DeviceToHostPinned);
	CZ_HTML_EXPORT_TAB(DeviceToHostPageable);
	CZ_HTML_EXPORT_TAB(DeviceToDevice);
	out += "<tr><th colspan=\"2\">" + tr("GPU Core Performance") + "</th></tr>\n";
	CZ_HTML_EXPORT_TAB(FloatRate);
	CZ_HTML_EXPORT_TAB(DoubleRate);
	CZ_HTML_EXPORT_TAB(Int64Rate);
	CZ_HTML_EXPORT_TAB(Int32Rate);
	CZ_HTML_EXPORT_TAB(Int24Rate);
	out += "</table>\n";

	time_t t;
	time(&t);
	out +=	"<p><small><b>" + tr("Generated") + ":</b> " + ctime(&t) + "</small></p>\n";

	out +=	"<p><a href=\"http://cuda-z.sourceforge.net/\"><img src=\"http://cuda-z.sourceforge.net/img/web-button.png\" alt=\"CUDA-Z\" title=\"CUDA-Z\" /></a></p>\n";

	out +=	"</body>\n"
		"</html>\n";

	return out;
}
