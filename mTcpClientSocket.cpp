#include "mTcpClientSocket.h"

TcpClientSocket::TcpClientSocket(QObject *parent)
{
    parent = parent;
    connect(this,SIGNAL(readyRead()),this,SLOT(dataReceived()));
    connect(this,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
}

void TcpClientSocket::dataReceived()
{
    qDebug() << "TcpClientSocket::dataReceived()";
    while(bytesAvailable()>0)
    {
        int length = bytesAvailable();
        char buf[1024];
        read(buf,length);

        QString msg=buf;
        emit updateClients(msg,length);
    }
}

void TcpClientSocket::slotDisconnected()
{
    emit disconnected(this->socketDescriptor());
}
