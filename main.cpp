#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QFile f(":qdarkstyle/Eclippy.qss");
    QFile f(":qdarkstyle/style.qss");
    //QFile f(":qdarkstyle/coffee.qss");
    //QFile f(":qdarkstyle/pagefold.qss");
   // QFile f(":/Resources/blender.qss");
  //  QFile f(":/Resources/familiar-dark.qss");
   // QFile f(":/Resources/pgilfernandez-dark.qss");


    if (!f.exists())
    {
        qDebug() << "Unable to set stylesheet, file not found";
    }
    else
    {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);

        qApp->setStyleSheet(ts.readAll());
    }

    MainWindow w;
    w.show();

    return a.exec();
}


