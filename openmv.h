#ifndef OPENMV_H
#define OPENMV_H

#include <QObject>
#include <QtSerialPort>
#include <QQueue>

//Constants and commands
#define FRAME_SIZE_RESPONSE_LEN         12
#define MTU_DEFAULT_SIZE (1024 * 1024 * 1024)

#define USBDBG_CMD                    0x30
#define USBDBG_FW_VERSION             0x80
#define USBDBG_FRAME_SIZE             0x81
#define USBDBG_FRAME_DUMP             0x82
#define USBDBG_ARCH_STR               0x83
#define USBDBG_LEARN_MTU              0x84
#define USBDBG_SCRIPT_EXEC            0x05
#define USBDBG_SCRIPT_STOP            0x06
#define USBDBG_SCRIPT_SAVE            0x07
#define USBDBG_SCRIPT_RUNNING         0x87
#define USBDBG_TEMPLATE_SAVE          0x08
#define USBDBG_DESCRIPTOR_SAVE        0x09
#define USBDBG_ATTR_READ              0x8A
#define USBDBG_ATTR_WRITE             0x0B
#define USBDBG_SYS_RESET              0x0C
#define USBDBG_FB_ENABLE              0x0D
#define USBDBG_JPEG_ENABLE            0x0E
#define USBDBG_TX_BUF_LEN             0x8E
#define USBDBG_TX_BUF                 0x8F

#define OPENMVCAM_VID 0x1209
#define OPENMVCAM_PID 0xABD1

#define OPENMVCAM_BAUD_RATE 12000000
#define OPENMVCAM_BAUD_RATE_2 921600

#define IS_JPG(bpp)                     ((bpp) >= 3)
#define IS_RGB(bpp)                     ((bpp) == 2)
#define IS_GS(bpp)                      ((bpp) == 1)
#define IS_BINARY(bpp)                  ((bpp) == 0)


class OpenMVCommand
{
public:
    explicit OpenMVCommand(const QByteArray &data = QByteArray(), int responseLen = int()) :
        m_data(data), m_responseLen(responseLen) { }
    QByteArray m_data;
    int m_responseLen;
};

class OpenMV : public QObject
{
    Q_OBJECT
public:
    explicit OpenMV(QObject *parent = nullptr);
    ~OpenMV();


    void scriptExec(const QByteArray &data);
    void scriptStop();
    void enableFB(bool enabled);
    void updateFWVersion();
    void sysReset();
    void updateScriptIsRunning();

    void setConnectCam();
    void setDisconnectCam();

    void setTerminate(); //always set to true

    QString getVersion();
    bool getScriptIsRunning();

private:
    QThread *thread;

    QQueue<OpenMVCommand> m_queue;
    QSerialPort *m_serial;

    int m_readTimeOut = 150; //ms

    QMap<qint16, QString> mapaComandos;
    QByteArray m_pixelBuffer;
    QByteArray m_lineBuffer;

    //frame size
    int m_frameSizeW;
    int m_frameSizeH;
    int m_frameSizeBPP;
    int m_mtu;
    QString m_version;

    bool m_terminate = false;
    bool m_disconnectCam = false;
    bool m_connectCam = false;

    bool m_fbEnabled = false;
    bool m_scriptIsRunning = false;

    QMutex m_mutex;  //used to avoid sharing violation when somebody will make a call from other thread.

    bool connectCam();
    void disconnectCam(); //serial cannot be disconnected from other thread than this.

    void getFrameBuffer();
    void getFrameBufferSize();

    void getTextBuffer();
    void getTextBufferSize();

    void processCommand(const OpenMVCommand &command);
    void processCommandResult(unsigned char cmd, QByteArray data);
    QByteArray pasrsePrintData(const QByteArray &data);

signals:
    void sgnlCommandResult(QString result);  //resultado do comando enviado pela serial
    void sgnlFinished();   //finaliza thread
    void sgnlError(QString err);  //erro thread
    void sgnlInfo(QString info); //message info thread
    void sgnlFrameBufferData(const QPixmap &data);
    void sgnlPrintData(const QByteArray &data);


public slots:
    void sltThreadMainLoop();

};

#endif // OPENMV_H






