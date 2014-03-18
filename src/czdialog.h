/*!	\file czdialog.h
	\brief Main window implementation header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://ag.embedded.org.ru/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_DIALOG_H
#define CZ_DIALOG_H

#if QT_VERSION < 0x050000
#define CZ_USE_QHTTP
#endif//QT_VERSION

#include <QSplashScreen>
#include <QTimer>
#ifdef CZ_USE_QHTTP
#include <QHttp>
#else
#include <QNetworkAccessManager>
#include <QNetworkReply>
#endif /*CZ_USE_QHTTP*/
#include <QUrl>

#include "ui_czdialog.h"
#include "czdeviceinfo.h"
#include "cudainfo.h"

class CZSplashScreen: public QSplashScreen {
	Q_OBJECT

public:
	explicit CZSplashScreen(const QPixmap &pixmap = QPixmap(), int maxLines = 1, Qt::WindowFlags f = 0);
	CZSplashScreen(QWidget *parent, const QPixmap &pixmap = QPixmap(), int maxLines = 1, Qt::WindowFlags f = 0);
	virtual ~CZSplashScreen();

	void setMaxLines(int maxLines);
	int maxLines();

public slots:
	void showMessage(const QString &message, int alignment = Qt::AlignLeft, const QColor &color = Qt::black);
	void clearMessage();

private:
	QString m_message;
	int m_maxLines;
	int m_lines;
	int m_alignment;
	QColor m_color;

	void deleteTop(int lines = 1);
};

extern CZSplashScreen *splash;

class CZDialog: public QDialog, public Ui::CZDialog {
	Q_OBJECT

public:
	CZDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	~CZDialog();

private:
	QList<CZCudaDeviceInfo*> deviceList;
	QTimer *updateTimer;
#ifdef CZ_USE_QHTTP
	QHttp *http;
	int httpId;
#else
	QNetworkAccessManager qnam;
	QNetworkReply *reply;
#endif /*CZ_USE_QHTTP*/
	QUrl url;
	QString history;

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
	void startHttpRequest(QUrl url);
	void parseHistoryTxt(QString history);

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

	QString getValue1000(double value, int valuePrefix, QString unitBase);
	QString getValue1024(double value, int valuePrefix, QString unitBase);

private slots:
	void slotShowDevice(int index);
	void slotUpdatePerformance(int index);
	void slotUpdateTimer();
	void slotExportToText();
	void slotExportToHTML();
	void slotUpdateVersion();
#ifdef CZ_USE_QHTTP
	void slotHttpRequestFinished(int id, bool error);
	void slotHttpStateChanged(int state);
#else
	void slotHttpFinished();
	void slotHttpReadyRead();
#endif /*CZ_USE_QHTTP*/
};

#endif//CZ_DIALOG_H
