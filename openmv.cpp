#include "openmv.h"
#include <QDebug>
#include <QTime>
#include <QPixmap>

void serializeByte(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 1);
}

void serializeWord(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 2);
}

void serializeLong(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 4);
}

int deserializeByte(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 1);
    buffer = buffer.mid(1);
    return r;
}

int deserializeWord(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 2);
    buffer = buffer.mid(2);
    return r;
}

int deserializeLong(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 4);
    buffer = buffer.mid(4);
    return r;
}

static QByteArray byteSwap(QByteArray buffer, bool ok)
{
    if(ok)
    {
        for(int i = 0, j = (buffer.size() / 2) * 2; i < j; i += 2)
        {
            char temp = buffer.data()[i];
            buffer.data()[i] = buffer.data()[i+1];
            buffer.data()[i+1] = temp;
        }
    }

    return buffer;
}

int getImageSize(int w, int h, int bpp)
{
    return IS_JPG(bpp) ? bpp : ((IS_RGB(bpp) || IS_GS(bpp)) ? (w * h * bpp) : (IS_BINARY(bpp) ? (((w + 31) / 32) * h) : int()));
}

QPixmap getImageFromData(QByteArray data, int w, int h, int bpp)
{
    QPixmap pixmap = getImageSize(w, h, bpp) ? (QPixmap::fromImage(IS_JPG(bpp)
        ? QImage::fromData(data, "JPG")
        : QImage(reinterpret_cast<const uchar *>(byteSwap(data,
            IS_RGB(bpp)).constData()), w, h, IS_BINARY(bpp) ? ((w + 31) / 32) : (w * bpp),
            IS_RGB(bpp) ? QImage::Format_RGB16 : (IS_GS(bpp) ? QImage::Format_Grayscale8 : (IS_BINARY(bpp) ? QImage::Format_MonoLSB : QImage::Format_Invalid)))))
    : QPixmap();

    if(pixmap.isNull() && IS_JPG(bpp))
    {
        data = data.mid(1, data.size() - 2);

        int size = data.size();
        QByteArray temp;

        for(int i = 0, j = (size / 4) * 4; i < j; i += 4)
        {
            int x = 0;
            x |= (data.at(i + 0) & 0x3F) << 0;
            x |= (data.at(i + 1) & 0x3F) << 6;
            x |= (data.at(i + 2) & 0x3F) << 12;
            x |= (data.at(i + 3) & 0x3F) << 18;
            temp.append((x >> 0) & 0xFF);
            temp.append((x >> 8) & 0xFF);
            temp.append((x >> 16) & 0xFF);
        }

        if((size % 4) == 3) // 2 bytes -> 16-bits -> 24-bits sent
        {
            int x = 0;
            x |= (data.at(size - 3) & 0x3F) << 0;
            x |= (data.at(size - 2) & 0x3F) << 6;
            x |= (data.at(size - 1) & 0x0F) << 12;
            temp.append((x >> 0) & 0xFF);
            temp.append((x >> 8) & 0xFF);
        }

        if((size % 4) == 2) // 1 byte -> 8-bits -> 16-bits sent
        {
            int x = 0;
            x |= (data.at(size - 2) & 0x3F) << 0;
            x |= (data.at(size - 1) & 0x03) << 6;
            temp.append((x >> 0) & 0xFF);
        }

        pixmap = QPixmap::fromImage(QImage::fromData(temp, "JPG"));
    }

    return pixmap;
}


QByteArray OpenMV::pasrsePrintData(const QByteArray &data)
{
    enum
    {
        ASCII,
        UTF_8,
        EXIT_0,
        EXIT_1
    }
    int_stateMachine = ASCII;
    QByteArray int_shiftReg = QByteArray();
    QByteArray int_frameBufferData = QByteArray();

    QByteArray buffer;

    for(int i = 0, j = data.size(); i < j; i++)
    {
        if((int_stateMachine == UTF_8) && ((data.at(i) & 0xC0) != 0x80))
        {
            int_stateMachine = ASCII;
        }

        if((int_stateMachine == EXIT_0) && ((data.at(i) & 0xFF) != 0x00))
        {
            int_stateMachine = ASCII;
        }

        switch(int_stateMachine)
        {
            case ASCII:
            {
                if(((data.at(i) & 0xE0) == 0xC0)
                || ((data.at(i) & 0xF0) == 0xE0)
                || ((data.at(i) & 0xF8) == 0xF0)
                || ((data.at(i) & 0xFC) == 0xF8)
                || ((data.at(i) & 0xFE) == 0xFC)) // UTF_8
                {
                    int_shiftReg.clear();

                    int_stateMachine = UTF_8;
                }
                else if((data.at(i) & 0xFF) == 0xFF)
                {
                    int_stateMachine = EXIT_0;
                }
                else if((data.at(i) & 0xC0) == 0x80)
                {
                    int_frameBufferData.append(data.at(i));
                }
                else if((data.at(i) & 0xFF) == 0xFE)
                {
                    int size = int_frameBufferData.size();
                    QByteArray temp;

                    for(int k = 0, l = (size / 4) * 4; k < l; k += 4)
                    {
                        int x = 0;
                        x |= (int_frameBufferData.at(k + 0) & 0x3F) << 0;
                        x |= (int_frameBufferData.at(k + 1) & 0x3F) << 6;
                        x |= (int_frameBufferData.at(k + 2) & 0x3F) << 12;
                        x |= (int_frameBufferData.at(k + 3) & 0x3F) << 18;
                        temp.append((x >> 0) & 0xFF);
                        temp.append((x >> 8) & 0xFF);
                        temp.append((x >> 16) & 0xFF);
                    }

                    if((size % 4) == 3) // 2 bytes -> 16-bits -> 24-bits sent
                    {
                        int x = 0;
                        x |= (int_frameBufferData.at(size - 3) & 0x3F) << 0;
                        x |= (int_frameBufferData.at(size - 2) & 0x3F) << 6;
                        x |= (int_frameBufferData.at(size - 1) & 0x0F) << 12;
                        temp.append((x >> 0) & 0xFF);
                        temp.append((x >> 8) & 0xFF);
                    }

                    if((size % 4) == 2) // 1 byte -> 8-bits -> 16-bits sent
                    {
                        int x = 0;
                        x |= (int_frameBufferData.at(size - 2) & 0x3F) << 0;
                        x |= (int_frameBufferData.at(size - 1) & 0x03) << 6;
                        temp.append((x >> 0) & 0xFF);
                    }

                    QPixmap pixmap = QPixmap::fromImage(QImage::fromData(temp, "JPG"));

                    if(!pixmap.isNull())
                    {
                        emit sgnlFrameBufferData(pixmap);
                    }

                    int_frameBufferData.clear();
                }
                else if((data.at(i) & 0x80) == 0x00) // ASCII
                {
                    buffer.append(data.at(i));
                }

                break;
            }

            case UTF_8:
            {
                if((((int_shiftReg.at(0) & 0xE0) == 0xC0) && (int_shiftReg.size() == 1))
                || (((int_shiftReg.at(0) & 0xF0) == 0xE0) && (int_shiftReg.size() == 2))
                || (((int_shiftReg.at(0) & 0xF8) == 0xF0) && (int_shiftReg.size() == 3))
                || (((int_shiftReg.at(0) & 0xFC) == 0xF8) && (int_shiftReg.size() == 4))
                || (((int_shiftReg.at(0) & 0xFE) == 0xFC) && (int_shiftReg.size() == 5)))
                {
                    buffer.append(int_shiftReg + data.at(i));

                    int_stateMachine = ASCII;
                }

                break;
            }

            case EXIT_0:
            {
                int_stateMachine = EXIT_1;

                break;
            }

            case EXIT_1:
            {
                int_stateMachine = ASCII;

                break;
            }
        }

        int_shiftReg = int_shiftReg.append(data.at(i)).right(5);
    }

    return buffer;
}

OpenMV::OpenMV(QObject *parent) : QObject(parent)
{
    //inicializa objetos
    m_serial = new QSerialPort(this);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);   

    m_frameSizeW = int();
    m_frameSizeH = int();
    m_frameSizeBPP = int();
    m_mtu = MTU_DEFAULT_SIZE;

    mapaComandos.insert(USBDBG_CMD                    ,"USBDBG_CMD");
    mapaComandos.insert(USBDBG_FW_VERSION             ,"USBDBG_FW_VERSION");
    mapaComandos.insert(USBDBG_FRAME_SIZE             ,"USBDBG_FRAME_SIZE");
    mapaComandos.insert(USBDBG_FRAME_DUMP             ,"USBDBG_FRAME_DUMP");
    mapaComandos.insert(USBDBG_ARCH_STR               ,"USBDBG_ARCH_STR");
    mapaComandos.insert(USBDBG_LEARN_MTU              ,"USBDBG_LEARN_MTU");
    mapaComandos.insert(USBDBG_SCRIPT_EXEC            ,"USBDBG_SCRIPT_EXEC");
    mapaComandos.insert(USBDBG_SCRIPT_STOP            ,"USBDBG_SCRIPT_STOP");
    mapaComandos.insert(USBDBG_SCRIPT_SAVE            ,"USBDBG_SCRIPT_SAVE");
    mapaComandos.insert(USBDBG_SCRIPT_RUNNING         ,"USBDBG_SCRIPT_RUNNING");
    mapaComandos.insert(USBDBG_TEMPLATE_SAVE          ,"USBDBG_TEMPLATE_SAVE");
    mapaComandos.insert(USBDBG_DESCRIPTOR_SAVE        ,"USBDBG_DESCRIPTOR_SAVE");
    mapaComandos.insert(USBDBG_ATTR_READ              ,"USBDBG_ATTR_READ");
    mapaComandos.insert(USBDBG_ATTR_WRITE             ,"USBDBG_ATTR_WRITE");
    mapaComandos.insert(USBDBG_SYS_RESET              ,"USBDBG_SYS_RESET");
    mapaComandos.insert(USBDBG_FB_ENABLE              ,"USBDBG_FB_ENABLE");
    mapaComandos.insert(USBDBG_JPEG_ENABLE            ,"USBDBG_JPEG_ENABLE");
    mapaComandos.insert(USBDBG_TX_BUF_LEN             ,"USBDBG_TX_BUF_LEN");
    mapaComandos.insert(USBDBG_TX_BUF                 ,"USBDBG_TX_BUF");

}

void OpenMV::setTerminate()
{
    m_mutex.lock();
    m_terminate = true;
    m_mutex.unlock();
}

QString OpenMV::getVersion()
{
    return m_version;
}

bool OpenMV::getScriptIsRunning()
{
    return m_scriptIsRunning;
}

void OpenMV::setConnectCam()
{
    m_mutex.lock();
    m_connectCam = true;
    m_mutex.unlock();
}

void OpenMV::setDisconnectCam()
{
    m_mutex.lock();
    m_disconnectCam = true;
    m_mutex.unlock();
}

bool OpenMV::connectCam()
{

    if (!m_serial->isOpen()) {

    //busca a camera conectada na serial port
    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        if ( port.hasVendorIdentifier() && (port.vendorIdentifier() == OPENMVCAM_VID) &&
             port.hasProductIdentifier() && (port.productIdentifier() == OPENMVCAM_PID)) {
            //encontrou a camera
            emit sgnlInfo( QString("OpenMV Cam found - Port Name: %1 Vendor: %2 Product Identifier: %3").arg(port.portName()).arg(port.hasVendorIdentifier()).arg(port.productIdentifier()) );
            //recria a serial
            //delete m_serial;
            //m_serial = new QSerialPort(port.portName(), this);
            m_serial->setPort(port);           
            m_serial->setReadBufferSize(1000000);
            //m_serial->setReadBufferSize(0); //unlimited buffer size

            //tenta abrir a conexão com velocidade máxima
            if ( (!m_serial->setBaudRate(OPENMVCAM_BAUD_RATE)) ||
                 (!m_serial->open(QIODevice::ReadWrite))       ||
                 (!m_serial->setDataTerminalReady(true)) ) {
                //delete m_serial;
                //m_serial = new QSerialPort(port.portName(), this);
                m_serial->setPort(port);
                m_serial->setReadBufferSize(1000000);
                //m_serial->setReadBufferSize(0);  //unlimited buffer size
                //se não conseguiu abrir em 12000000 bps, tenta abrir em 921600
                if ( (!m_serial->setBaudRate(OPENMVCAM_BAUD_RATE_2)) ||
                     (!m_serial->open(QIODevice::ReadWrite))       ||
                     (!m_serial->setDataTerminalReady(true)) ) {
                    emit sgnlError(QString("Cannot open serial port %1").arg(m_serial->portName()));

                    return false;
                }
            }
            //to.. do.. seta fb enabled para true ..envia o comando

            emit sgnlInfo(QString("Serial port %1 opened at %2 bps").arg(m_serial->portName()).arg(QString::number(m_serial->baudRate())));
            return true;
        }
    }
    //Se não encontrou a camera
    emit sgnlError("Serial port not found!");


    return false;
    } else {
        //porta jah aberta
        emit sgnlInfo("Serial port already open!");

        return true;
    }


}

void OpenMV::disconnectCam()
{
    //to.. do.. seta fb enabled para false
    if (m_serial->isOpen()) {
        m_serial->flush();
        m_serial->clear(QSerialPort::AllDirections);
        m_serial->close();
            emit sgnlInfo(QString("Serial port %1 closed!").arg(m_serial->portName()));
    } else {
            emit sgnlInfo("Serial port already closed!");
    }

}

void OpenMV::scriptExec(const QByteArray &data)
{

    QByteArray buffer;
    QByteArray script = data; //scr.toUtf8();

    if (m_serial->isOpen()) {
        serializeByte(buffer, USBDBG_CMD);
        serializeByte(buffer, USBDBG_SCRIPT_EXEC);
        serializeLong(buffer, script.size());

        m_mutex.lock();
        m_queue.enqueue(OpenMVCommand(buffer, 0));
        m_queue.enqueue(OpenMVCommand(script, 0));
        m_mutex.unlock();
    } else {
        emit sgnlInfo(QString("Cam isn't connected!"));
    }

}

void OpenMV::scriptStop()
{
    QByteArray buffer;

        serializeByte(buffer, USBDBG_CMD);
        serializeByte(buffer, USBDBG_SCRIPT_STOP);
        serializeLong(buffer, int());

        m_mutex.lock();
        m_queue.enqueue(OpenMVCommand(buffer, 0));
        m_mutex.unlock();


}

void OpenMV::enableFB(bool enabled)
{    
    QByteArray buffer;

    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_FB_ENABLE);
    serializeLong(buffer, int());
    serializeWord(buffer, enabled ? true : false);

    m_fbEnabled = enabled;

    m_mutex.lock();
    m_queue.enqueue(OpenMVCommand(buffer, 0));
    m_mutex.unlock();

}

void OpenMV::updateFWVersion()
{
    QByteArray buffer;

    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_FW_VERSION);
    serializeLong(buffer, FRAME_SIZE_RESPONSE_LEN);
    m_mutex.lock();
    m_queue.enqueue(OpenMVCommand(buffer, FRAME_SIZE_RESPONSE_LEN));
    m_mutex.unlock();


}

void OpenMV::sysReset()
{
    QByteArray buffer;
    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_SYS_RESET);
    serializeLong(buffer, 0);
    m_mutex.lock();
    m_queue.enqueue(OpenMVCommand(buffer, 0));
    m_mutex.unlock();
}

bool OpenMV::updateScriptIsRunning()
{
    QByteArray buffer;
    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_SCRIPT_RUNNING);
    serializeLong(buffer, 4);
    m_mutex.lock();
    m_queue.enqueue(OpenMVCommand(buffer, 4));
    m_mutex.unlock();
}

void OpenMV::getFrameBuffer()
{
    getFrameBufferSize();
}

void OpenMV::getFrameBufferSize()
{
    //monta comando
    QByteArray buffer;
    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_FRAME_SIZE);
    serializeLong(buffer, FRAME_SIZE_RESPONSE_LEN);
    //coloca o comando na fila para processar
    m_queue.enqueue(OpenMVCommand(buffer, FRAME_SIZE_RESPONSE_LEN));
}

void OpenMV::getTextBuffer()
{
    getTextBufferSize();
}

void OpenMV::getTextBufferSize()
{
    //monta comando
    QByteArray buffer;
    serializeByte(buffer, USBDBG_CMD);
    serializeByte(buffer, USBDBG_TX_BUF_LEN);
    serializeLong(buffer, 4);
    //coloca o comando na fila para processar
    m_queue.enqueue(OpenMVCommand(buffer, 4));

}

void OpenMV::processCommand(const OpenMVCommand &command)
{
    QByteArray response;
    int responseLen = command.m_responseLen;
    QElapsedTimer elapsedTimer;

    qint64 bytesWritten = 0;

    //envia pacote
    if (m_serial){
        //m_serial->clear(QSerialPort::AllDirections);
        bytesWritten = m_serial->write(command.m_data.constData(), command.m_data.count());
        m_serial->flush();
        //calcula um timeout para a escrita dos bytes conforme a qtde bytes a serem escritos
        m_serial->waitForBytesWritten(static_cast<int>(  m_serial->bytesToWrite()*10) );

        //verifica se eh comando ou script. (obs.: não permite colocar script menor que 100 bytes... to.. do.. incluir cabeçalho com # comentarios.
        if (bytesWritten < 100) {
            if (DEBUG_INFO)
                emit sgnlInfo(QString("command %1 sended successfully. Waiting response len %2").arg(mapaComandos[ static_cast<unsigned char>(command.m_data[1]) ] ).arg(QString::number(responseLen)) );
        }

        response.clear();
        if (responseLen > 0){
            //aguarda resposta
            elapsedTimer.start();
            do {
                //aguarda receber dados conforme a qtde de dados que estah aguardando
                if (m_serial->waitForReadyRead(5)) {
                    //se tem dados inclui na resposta
                    response.append(m_serial->readAll());
                }

                //emit sgnlInfo(QString("size: %1 el: %2 ").arg(QString::number(response.length())).arg(QString::number(elaspedTimer.elapsed())));
            }
            while((response.size() < responseLen) && (!elapsedTimer.hasExpired(m_readTimeOut)));

            if(response.size() >= responseLen)
            {
                if (DEBUG_INFO)
                    emit sgnlInfo(QString("command %1 received successfully. Processing...").arg(mapaComandos[ static_cast<unsigned char>(command.m_data[1]) ] ) );
                processCommandResult(static_cast<unsigned char>(command.m_data[1]), response.left(command.m_responseLen));
            }
            else{
                if (DEBUG_INFO)
                    emit sgnlError(QString("command %1 receiving time out").arg(mapaComandos[ static_cast<unsigned char>(command.m_data[1]) ] ) );
            }
        }

    }
}

void OpenMV::processCommandResult(unsigned char cmd, QByteArray data)
{

    int w;
    int h;
    int bpp;

    switch (cmd) {
        case USBDBG_FRAME_SIZE: {
            w = deserializeLong(data);
            h = deserializeLong(data);
            bpp = deserializeLong(data);

            //to.. do.. busca imagem...

            if ((0 < w) && (w < 32768) && (0 < h) && (h < 32768) && (0 <= bpp) && (bpp <= (1024 * 1024 * 1024)))
            {
                int size = getImageSize(w, h, bpp);
                if (DEBUG_INFO)
                    emit sgnlInfo(QString("Frame Size: %1 - %2 - %3").arg(QString::number(w)).arg(QString::number(h)).arg(QString::number(bpp)));

                if(size)
                {
                    for(int i = 0, j = (size + m_mtu - 1) / m_mtu; i < j; i++)
                    {
                        //calcula o tamanho da resposta
                        int new_size = qMin(size - ((j - 1 - i) * m_mtu), m_mtu);

                        //adiciona o novo comando
                        QByteArray buffer;
                        serializeByte(buffer, USBDBG_CMD);
                        serializeByte(buffer, USBDBG_FRAME_DUMP);
                        serializeLong(buffer, new_size);

                        m_queue.enqueue(OpenMVCommand(buffer, new_size));
                        if (DEBUG_INFO)
                            emit sgnlInfo(QString("new size: %1  - i: %2").arg(QString::number(new_size)).arg(QString::number(i)));
                    }

                    m_frameSizeW = w;
                    m_frameSizeH = h;
                    m_frameSizeBPP = bpp;
                }
            }
            break;
        }
        case USBDBG_FRAME_DUMP:{
            m_pixelBuffer.append(data);

            if(m_pixelBuffer.size() == getImageSize(m_frameSizeW, m_frameSizeH, m_frameSizeBPP))
            {
                QPixmap pixmap = getImageFromData(m_pixelBuffer, m_frameSizeW, m_frameSizeH, m_frameSizeBPP);

                if(!pixmap.isNull())
                {
                    emit sgnlFrameBufferData(pixmap);
                }

                m_frameSizeW = int();
                m_frameSizeH = int();
                m_frameSizeBPP = int();
                m_pixelBuffer.clear();
            }
            break;
        }
        case USBDBG_TX_BUF_LEN:{
            int len = deserializeLong(data);

            if(len)
            {
                for(int i = 0, j = (len + m_mtu - 1) / m_mtu; i < j; i++)
                {
                    int new_len = qMin(len - ((j - 1 - i) * m_mtu), m_mtu);
                    QByteArray buffer;
                    serializeByte(buffer, USBDBG_CMD);
                    serializeByte(buffer, USBDBG_TX_BUF);
                    serializeLong(buffer, new_len);
                    m_queue.enqueue(OpenMVCommand(buffer, new_len));
                    //emit sgnlInfo(QString("new size: %1  - i: %2").arg(QString::number(new_len)).arg(QString::number(i)));

                }
            }
            else if(m_lineBuffer.size())
            {
                emit sgnlPrintData(pasrsePrintData(m_lineBuffer));
                m_lineBuffer.clear();
            }
            break;
        }
        case USBDBG_TX_BUF: {
            m_lineBuffer.append(data);
            QByteArrayList list = m_lineBuffer.split('\n');

            m_lineBuffer = list.takeLast();

            QByteArray out;

            for(int i = 0, j = list.size(); i < j; i++)
            {
                out.append(pasrsePrintData(list.at(i) + '\n'));
            }

            if(!out.isEmpty())
            {
                emit sgnlPrintData(out);
            }

            break;
        }
        case USBDBG_FW_VERSION:
        {
            m_version = QString::fromUtf8(data);
            break;
        }

        case USBDBG_SCRIPT_RUNNING:
        {
            m_scriptIsRunning = data.at(0);
            break;
        }
    }
}

void OpenMV::sltThreadMainLoop()
{
    OpenMVCommand cmd;
    QString cmdResult;
    QElapsedTimer tmScanTextBuffer;

    //monitora infinitamente a chegada de comando
    tmScanTextBuffer.start();
    while (!m_terminate) {
        //verifica demais comandos
        m_mutex.lock();

        if (m_disconnectCam) {
            disconnectCam();
            m_disconnectCam = false;
        }

        if (m_connectCam) {
            connectCam();
            m_connectCam = false;
        }

        if (m_serial->isOpen()) {  //use connectCam() to open serial

            //if FB is enabled then read FB and Text Buffer
            if (m_fbEnabled) {
                getFrameBuffer();
//                getTextBuffer();
                if(tmScanTextBuffer.hasExpired(100)) {
                    getTextBuffer();
                    tmScanTextBuffer.restart();
                }
            }

            while(!m_queue.isEmpty()) {
                cmd = m_queue.dequeue();
                if (DEBUG_INFO)
                    emit sgnlInfo(QString("______________ [%1]> Command [%2] - Queue Size[%3] ______________").arg(QTime::currentTime().toString("HH:mm:ss.zzz")).arg(mapaComandos[static_cast<unsigned char>(cmd.m_data[1])]).arg(QString::number(m_queue.count())));
                processCommand(cmd);
            }         
        }
        m_mutex.unlock();
        qApp->processEvents();
        //QThread::usleep(1);
    }

    qDebug() << "finalizando...";
    emit sgnlFinished();  //advice thread is finished
}
