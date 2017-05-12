#include "mTcpServer.h"
#include "mNetAssistWidget.h"

/*==============================================================*/
mTcpServer::mTcpServer(QObject *parent):QTcpServer(parent)
{
      connect(this, SIGNAL(newConnection()), this, SLOT(acceptNewClient()));
}

/*==============================================================*/
//accept a new client and set the SIGNAL and SLOT
void mTcpServer::acceptNewClient()
{
    int socketDescriptor;
   QTcpSocket *tcpClientSocket=nextPendingConnection();
   socketDescriptor  = tcpClientSocket->socketDescriptor();

   connect(tcpClientSocket,SIGNAL(disconnected()),this,SLOT(clientDisconnected()));
   connect(tcpClientSocket, SIGNAL(disconnected()), tcpClientSocket, SLOT(deleteLater()));
   connect(tcpClientSocket,SIGNAL(readyRead()),this,SLOT(clientDataReceived()));

   tcpClientSocketList.append(tcpClientSocket);   //add client

   //send the client address and descriptor to mNetAssistWidget
   QString peerPortStr = QString::number(tcpClientSocket->peerPort());
   QString rdClientAddress_Port =  tcpClientSocket->peerAddress().toString()+":"+peerPortStr;
   emit addClientLink(rdClientAddress_Port,socketDescriptor);
}

/*==============================================================*/
//send data to a client or all client
void mTcpServer::sendDataToClient(char *msg,int length,int socketDescriptor,int socketDescriptorEx)
{
    for(int i=0;i<tcpClientSocketList.count();i++)
    {
        QTcpSocket *item = tcpClientSocketList.at(i);

        if(socketDescriptor == 0){
            if(item->socketDescriptor() != socketDescriptorEx){
               if(item->write(msg,length)!=length)
               {
                     continue;
                }
            }
        }else{
            if(item->socketDescriptor() == socketDescriptor){
                if(item->write(msg,length)!=length)
                {
                    break;
                }
            }
        }
    }

}

/*==============================================================*/
//send disconnect a valid client tcpsocket
void mTcpServer::clientDisconnected()
{
    for(int i=0;i<tcpClientSocketList.count();i++)
    {
        QTcpSocket *item = tcpClientSocketList.at(i);
        if(item->state() == 0)
        {
            QString peerPortStr = QString::number(item->peerPort());
            QString rdClientAddress_Port = item->peerAddress().toString()+":"+peerPortStr;
             //send the client address and descriptor to mNetAssistWidget
            emit removeClientLink(rdClientAddress_Port,item->socketDescriptor());

            tcpClientSocketList.removeAt(i); //remove Client

            break;
        }
    }
}

/*==============================================================*/
//processe a client data
void mTcpServer::clientDataReceived()
{
    for(int i=0;i<tcpClientSocketList.count();i++)
    {
        QTcpSocket *item = tcpClientSocketList.at(i);
        while(item->bytesAvailable()>0)
        {
            QByteArray datagram;
            datagram.resize(item->bytesAvailable());
            item->read(datagram.data(),datagram.size());
            emit updateTcpServer((char *)datagram.data(),datagram.size(),item->socketDescriptor());
        }
    }
}
