/*!	\file czdeviceinfodecoder.h
	\brief CUDA device information decoder header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_DEVICEINFODECODER_H
#define CZ_DEVICEINFODECODER_H

#include "czdeviceinfo.h"

class CZCudaDeviceInfoDecoder: public QObject {
	Q_OBJECT

public:
	CZCudaDeviceInfoDecoder(CZCudaDeviceInfo &info, QObject *parent = 0);
	CZCudaDeviceInfoDecoder(struct CZDeviceInfo &info, QObject *parent = 0);
	~CZCudaDeviceInfoDecoder();

	enum {
		// tabs
		idTabCore,
		idTabMemory,
		idTabPerformance,

		// tab core
		idDrvVersion,
		idDrvDllVersion,
		idRtDllVersion,
		idName,
		idCapability,
		idClock,
		idPCIInfo,
		idMultiProc,
		idThreadsMulti,
		idWarp,
		idRegsBlock,
		idThreadsBlock,
		idThreadsDim,
		idGridDim,
		idWatchdog,
		idIntegrated,
		idConcurrentKernels,
		idComputeMode,
		idStreamPriorities,

		// tab memory
		idTotalGlobal,
		idBusWidth,
		idMemClock,
		idErrorCorrection,
		idL2CasheSize,
		idShared,
		idPitch,
		idTotalConst,
		idTextureAlign,
		idTexture1D,
		idTexture2D,
		idTexture3D,
		idGpuOverlap,
		idMapHostMemory,
		idUnifiedAddressing,
		idAsyncEngine,

		// tab performance
		idMemoryCopy,
		idHostPinnedToDevice,
		idHostPageableToDevice,
		idDeviceToHostPinned,
		idDeviceToHostPageable,
		idDeviceToDevice,
		idCorePerformance,
		idFloatRate,
		idDoubleRate,
		idInt64Rate,
		idInt32Rate,
		idInt24Rate,

		idMax,
	};

	enum {
		prefixNothing = 0,	/*!< No prefix. */

		/* SI units. */
		prefixKilo = 1,		/*!< Kilo prefix 1000^1 = 10^3. */
		prefixMega = 2,		/*!< Mega prefix 1000^2 = 10^6. */
		prefixGiga = 3,		/*!< Giga prefix 1000^3 = 10^9. */
		prefixTera = 4,		/*!< Tera prefix 1000^4 = 10^12. */
		prefixPeta = 5,		/*!< Peta prefix 1000^5 = 10^15. */
		prefixExa = 6,		/*!< Exa prefix 1000^6 = 10^18. */
		prefixZetta = 7,	/*!< Zetta prefix 1000^7 = 10^21. */
		prefixYotta = 8,	/*!< Yotta prefix 1000^8 = 10^24. */
		prefixSiMax = prefixYotta,

		/* IEC 60027 units. */
		prefixKibi = 1,		/*!< Kibi prefix 1024^1 = 2^10. */
		prefixMebi = 2,		/*!< Mebi prefix 1024^2 = 2^20. */
		prefixGibi = 3,		/*!< Gibi prefix 1024^3 = 2^30. */
		prefixTebi = 4,		/*!< Tebi prefix 1024^4 = 2^40. */
		prefixPebi = 5,		/*!< Pebi prefix 1024^5 = 2^50. */
		prefixExbi = 6,		/*!< Exbi prefix 1024^6 = 2^60. */
		prefixZebi = 7,		/*!< Zebi prefix 1024^7 = 2^70. */
		prefixYobi = 8,		/*!< Yobi prefix 1024^8 = 2^80. */
		prefixIecMax = prefixYobi,
	};

	const QString getName(int id) const;
	const QString getValue(int id) const;

	const QString generateTextReport() const;
	const QString generateHTMLReport() const;

	static const QString getValue1000(double value, int valuePrefix, QString unitBase);
	static const QString getValue1024(double value, int valuePrefix, QString unitBase);

private:
	struct CZDeviceInfo m_info;

};

#endif//CZ_DEVICEINFODECODER_H
