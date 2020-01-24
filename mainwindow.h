#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "openmv.h"
#include <QLabel>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    OpenMV *openMV;
    QTimer *tmUpdate;

    QLabel *lbStatusBarCaption;
    QStatusBar *stBar;

private slots:
    void sltTimerUpdate();
    //slots of data from openMV thread
    void sltHandleError(QString err);  //thread error
    void sltHandleInfo(QString info); //thread info
    void sltHandleFrameBuffer(const QPixmap &data);
    void sltHandleTextBuffer(const QByteArray &data);

    void runFree();

    void on_pbStop_pressed();
    void on_pbStart_pressed();
    void on_pbEnableFB_toggled(bool checked);

    void on_pbStopScript_pressed();
    void on_pbExecScript_pressed();

    void on_pbSnapshot_clicked();

    void on_pbSnapshot_2_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
