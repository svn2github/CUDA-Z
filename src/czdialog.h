/*!	\file czdialog.h
	\brief Main window implementation header file.
	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html
*/

#ifndef CZ_DIALOG_H
#define CZ_DIALOG_H

#include "qglobal.h"

//#if QT_VERSION < 0x050000
//#define CZ_USE_QHTTP
//#endif//QT_VERSION

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
	QList<CZCudaDeviceInfo*> m_deviceList;
	QTimer *m_updateTimer;
#ifdef CZ_USE_QHTTP
	QHttp *m_http;
	int m_httpId;
#else
	QNetworkAccessManager m_qnam;
	QNetworkReply *m_reply;
#endif /*CZ_USE_QHTTP*/
	QUrl m_url;
	QString m_history;

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
	QString getPlatformString();

	void startGetHistoryHttp();
	void cleanGetHistoryHttp();
	void startHttpRequest(QUrl url);
	void parseHistoryTxt(QString history);

	QString generateTextReport();
	QString generateHTMLReport();

private slots:
	void slotShowDevice(int index);
	void slotUpdatePerformance(int index);
	void slotUpdateTimer();
	void slotExportToText();
	void slotExportToHTML();
	void slotExportToClipboard();
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
