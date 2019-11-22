#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    openMV = new OpenMV();
    connect(openMV, SIGNAL(sgnlError(QString)), this, SLOT( sltHandleError(QString) ));
    connect(openMV, SIGNAL(sgnlInfo(QString)), this, SLOT(sltHandleInfo(QString)) );
    connect(openMV, SIGNAL(sgnlFrameBufferData(QPixmap)), this, SLOT(sltHandleFrameBuffer(QPixmap)));
    connect(openMV, SIGNAL(sgnlPrintData(QByteArray)), this, SLOT(sltHandleTextBuffer(QByteArray)));

    tmUpdate = new QTimer(this);
    tmUpdate->start(500);

    connect(tmUpdate, SIGNAL(timeout()), this, SLOT(sltTimerUpdate()));

    lbStatusBarCaption = new QLabel(this);
    lbStatusBarCaption->setFrameShape(QFrame::Box);
    lbStatusBarCaption->setAlignment(Qt::AlignCenter);
    statusBar()->addWidget(lbStatusBarCaption);

}

MainWindow::~MainWindow()
{
    openMV->setTerminate();  //terminate the thread before to destroy the application
    delete ui;
}

void MainWindow::sltTimerUpdate()
{
    static int i = 0;
    static QString statusScript;
    if (i >= 10) {
        openMV->updateScriptIsRunning();
        i = 0;
    }
    statusScript = openMV->getScriptIsRunning()?"Running":"Not Running";
    lbStatusBarCaption->setText(QTime::currentTime().toString("hh:mm:ss")+ " :: "+statusScript);
    i ++;
}

void MainWindow::sltHandleFrameBuffer(const QPixmap &data) {
    //verifica se o QGraphicsView tem scene, se não tiver, cria a scene e o pixmap

    if (ui->graphicsView->scene() == nullptr) {
        ui->graphicsView->setScene(new QGraphicsScene(this));
        ui->graphicsView->scene()->addItem(new QGraphicsPixmapItem());
    }
    //pega o primeiro objeto da scena que eh o pixmap criado
    QGraphicsPixmapItem *pixmap = qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->graphicsView->scene()->items().at(0));

    pixmap->setPixmap( data );
    ui->graphicsView->fitInView(pixmap, Qt::KeepAspectRatio);

    qApp->processEvents();

}

void MainWindow::sltHandleTextBuffer(const QByteArray &data)
{
    ui->txtLog->moveCursor(QTextCursor::End);
    ui->txtLog->setTextColor(Qt::yellow);
    ui->txtLog->insertPlainText(QString::fromUtf8(data));
    ui->txtLog->moveCursor(QTextCursor::End);

}


void MainWindow::sltHandleError(QString err)
{
    //qDebug() << err;
    ui->txtLog->moveCursor(QTextCursor::End);
    ui->txtLog->setTextColor(Qt::red);
    ui->txtLog->append(err);
    ui->txtLog->moveCursor(QTextCursor::End);
}

void MainWindow::sltHandleInfo(QString info)
{
   // qDebug() << info;
    ui->txtLog->moveCursor(QTextCursor::End);
    ui->txtLog->setTextColor(Qt::green);
    ui->txtLog->append(info);
    ui->txtLog->moveCursor(QTextCursor::End);
}

void MainWindow::on_pbStop_pressed()
{
    openMV->setDisconnectCam();
}

void MainWindow::on_pbStart_pressed()
{
    openMV->setConnectCam();    

    ui->pbEnableFB->setChecked(true);
    openMV->updateFWVersion();

}

void MainWindow::on_pbEnableFB_toggled(bool checked)
{
    openMV->enableFB(checked);
}

void MainWindow::on_pbStopScript_pressed()
{
     ui->pbEnableFB->setChecked(false);
     openMV->scriptStop();
}

void MainWindow::on_pbExecScript_pressed()
{
    ui->pbEnableFB->setChecked(true);
    openMV->updateFWVersion();

    openMV->scriptExec(ui->txScript->toPlainText().toUtf8());
}










