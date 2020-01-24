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
    static QString statusScript;

    statusScript = openMV->getScriptIsRunning()?"Running":"Not Running";
    lbStatusBarCaption->setText(QTime::currentTime().toString("hh:mm:ss")+ " :: "+statusScript);

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

void MainWindow::runFree()
{
    int countFrames = 0;
    int FPS = 0;
    QElapsedTimer elTimer;


    elTimer.start();
    while (true) {

        on_pbSnapshot_clicked();

        countFrames++;
        if (elTimer.hasExpired(1000)){
            FPS = countFrames;

            qDebug() << FPS;
            countFrames = 0;
            elTimer.restart();
        }
        qApp->processEvents();
    }

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
    openMV->closeCam();
}

void MainWindow::on_pbStart_pressed()
{
    openMV->openCam();
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

void MainWindow::on_pbSnapshot_clicked()
{
    QPixmap data;
    data = openMV->snapshot();

    if (ui->snapshotView->scene() == nullptr) {
        ui->snapshotView->setScene(new QGraphicsScene(this));
        ui->snapshotView->scene()->addItem(new QGraphicsPixmapItem());
    }

    //pega o primeiro objeto da scena que eh o pixmap criado
    QGraphicsPixmapItem *pixmap = qgraphicsitem_cast<QGraphicsPixmapItem *>(ui->snapshotView->scene()->items().at(0));

    pixmap->setPixmap( data );
    ui->snapshotView->fitInView(pixmap, Qt::KeepAspectRatio);
}

void MainWindow::on_pbSnapshot_2_clicked()
{
    runFree();
}
