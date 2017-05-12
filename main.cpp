#include "mNetAssistWidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
   // QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));  //QT4.8
    //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));            //QT4.8

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));       //QT5

    mNetAssistWidget w;
    w.show();
    return a.exec();
}
