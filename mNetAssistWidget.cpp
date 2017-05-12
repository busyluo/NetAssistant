#include "mNetAssistWidget.h"
#include "ui_mNetAssistWidget.h"
#include "mdefine.h"
#include <QMessageBox>
#include <stdlib.h>
#include <QFileDialog>
#include <QFile>
#include <QString>
#include <QTimer>
#include <QInputDialog>
#include <QDateTime>

const char toHex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

mNetAssistWidget::mNetAssistWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mNetAssistWidget)
{
    ui->setupUi(this);

    //设置程序的开启默认画面
    setUdpGuiExt();

    //获取本机的IP地址,并初始化相应的控件属性和变量
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
       if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address()) {
           m_ip = ipAddressesList.at(i).toString();
           break;
       }
    }
     // 设置获取的IP到相应文本框
    ui->lEditIpAddr->setText(m_ip);
    ui->lEditUdpIP->setText(m_ip);
    ui->SndProgressBar->setVisible(false);

   //初始化全局变量
    rmtServerIP =new QHostAddress();
     rcvDataCnt  = 0;
     sndDataCnt = 0;
     TcpClientLinkCnt = 0;
     //NetState = false;
     loopSending = false;
     CurIPPort = "";
     CurPath = "";
     curFile = 0;
}

mNetAssistWidget::~mNetAssistWidget()
{
    delete ui;
}

/**********************************************************/
//Function for connection button
void mNetAssistWidget::on_pBtnNetCnnt_clicked(bool checked)
{
    if(checked){      //切换到链接状态
        if(ui->cBoxNetType->currentIndex() == UDP_MODE){
            //建立UDP链接
            udpSocket = new QUdpSocket(this);
             connect(udpSocket,SIGNAL(readyRead()),this,SLOT(udpDataReceived()));
            lhAddr.setAddress(ui->lEditIpAddr->text());
            lhPort = ui->lEditIpPort->text().toInt();
            rmtAddr.setAddress(ui->lEditUdpIP->text());
            rmtPort = ui->lEditUdpPort->text().toInt();
            bool result=udpSocket->bind(lhPort);
            if(!result)
            {
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("UDP绑定端口失败!"));
                return;
            }
            ui->CurState->setText(tr("建立UDP连接成功"));

        }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
             //建立TCP服务器链接
            lhAddr.setAddress(ui->lEditIpAddr->text());
            lhPort = ui->lEditIpPort->text().toInt();

            if(! slotTryCreateTcpServer())
            {
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("尝试建立服务器失败! 请确认网络状态和端口。"));
                return;
            }
            ui->CurState->setText(tr("建立TCP服务器成功"));
        }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
            //建立TCP客户端
            QString ip = ui->lEditIpAddr->text();
            if(!rmtServerIP->setAddress(ip))
            {
                QMessageBox::information(this,tr("错误"),tr("TCP服务器IP设置失败!"));
                return;
            }
            tcpClientSocket = new QTcpSocket(this);
            connect(tcpClientSocket,SIGNAL(readyRead()),this,SLOT(tcpClientDataReceived()));
            tcpClientSocket->connectToHost(*rmtServerIP,ui->lEditIpPort->text().toInt());

            if( !tcpClientSocket->waitForConnected(2000)){
                ui->lEditUdpPort->setText(QString::number(0,10));
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this,tr("错误"),tr("尝试连接服务器失败! 请确认服务器状态。"));

                return;
            }

            ui->lEditUdpPort->setText(QString::number(tcpClientSocket->localPort(),10));
            ui->CurState->setText(tr("连接TCP服务器成功"));
        }
        ui->pBtnNetCnnt->setText(tr(" 断开网络"));
        ui->pBtnSendData->setEnabled(true);
        //NetState = true;
    }else{ //切换到断开状态
        if(ui->cBoxNetType->currentIndex() == UDP_MODE){
            //断开UDP链接
            udpSocket->close();
            delete udpSocket;
        }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
            //断开TCP服务器链接
            slotDeleteTcpServer();
        }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
            //断开TCP客户端链接
           tcpClientSocket->disconnectFromHost();
        }

        ui->pBtnNetCnnt->setText(tr(" 连接网络"));
        ui->pBtnSendData->setEnabled(false);
        ui->CurState->setText(tr(""));
       // NetState = false;
    }
}

/**********************************************************/
//funcions switch
void mNetAssistWidget::on_cBoxNetType_currentIndexChanged(int index)
{
    if(index == UDP_MODE){
        setUdpGuiExt();
        ui->label_Port->setText(tr("本地端口"));
        ui->label_IP->setText(tr("本地IP地址"));
        ui->labelUdp->setText(tr("目标IP地址"));
        ui->labelUdp1->setText(tr("目标端口"));
        ui->lEditIpAddr->setText(m_ip);
    }else if(index == TCP_SERVER_MODE){
        setTcpSvrGuiExt();
       // setTcpClientGuiExt();
        ui->label_Port->setText(tr("本地端口"));
        ui->label_IP->setText(tr("本地IP地址"));
        ui->lEditIpAddr->setText(m_ip);
    }else if(index == TCP_CLIENT_MODE){
        setTcpClientGuiExt();
        ui->label_Port->setText(tr("服务器端口"));
        ui->label_IP->setText(tr("服务器IP地址"));
        ui->labelUdp->setText(tr("本地IP地址"));
        ui->labelUdp1->setText(tr("本地端口"));
        ui->lEditIpAddr->setText(tr("220.165.9.87"));
    }
}

/**********************************************************/
//set UDP mode
void mNetAssistWidget::setUdpGuiExt()
{
    ui->labelSpaceUdp->setVisible(true);
    ui->lEditUdpIP->setVisible(true);
    ui->labelUdp->setVisible(true);
    ui->labelUdp1->setVisible(true);
    ui->lEditUdpPort->setVisible(true);

    ui->labelClients->setVisible(false);
    ui->cBoxClients->setVisible(false);
    ui->labelSpaceClients->setVisible(false);
    ui->cBox_chatMode->setVisible(false);
    ui->cBox_echoMode->setVisible(false);
}

/**********************************************************/
//set TCP server
void mNetAssistWidget::setTcpSvrGuiExt()
{
    ui->labelSpaceUdp->setVisible(false);
    ui->lEditUdpIP->setVisible(false);
    ui->labelUdp->setVisible(false);
    ui->labelUdp1->setVisible(false);
    ui->lEditUdpPort->setVisible(false);

    ui->labelClients->setVisible(true);
    ui->cBoxClients->setVisible(true);
    ui->labelSpaceClients->setVisible(true);
    ui->cBox_chatMode->setVisible(true);
    ui->cBox_echoMode->setVisible(true);
}

/**********************************************************/
void mNetAssistWidget::setTcpClientGuiExt()
{
    ui->labelSpaceUdp->setVisible(true);
    ui->lEditUdpIP->setVisible(true);
    ui->labelUdp->setVisible(true);
    ui->labelUdp1->setVisible(true);
    ui->lEditUdpPort->setVisible(true);

    ui->labelClients->setVisible(false);
    ui->cBoxClients->setVisible(false);
    ui->labelSpaceClients->setVisible(false);
    ui->cBox_chatMode->setVisible(false);
    ui->cBox_echoMode->setVisible(false);
}

/**********************************************************/
//send data
void mNetAssistWidget::on_pBtnSendData_clicked()
{
    if(ui->cBoxStartSndFile->checkState()){
        on_pBtnResetCnt_clicked();
        ui->pBtnSendData->setText(tr("正在发送"));
        ui->pBtnSendData->setEnabled(false);
        insertDateTimeInRcvDisp();
        ui->ReceiveTextEdit->appendPlainText(tr("开始发送..."));
        toSendFile();
        insertDateTimeInRcvDisp();
        ui->ReceiveTextEdit->appendPlainText(tr("发送完成！"));
        float hasSnd = sndDataCnt;
        hasSnd = hasSnd/1024/1024;
        QString hasSndSz = QString("%1").arg(hasSnd);
        ui->ReceiveTextEdit->appendPlainText(tr("共发送数据：")+hasSndSz+"MB");

        ui->pBtnSendData->setText(tr("发送"));
        ui->pBtnSendData->setEnabled(true);
        return;
    }

    if(ui->tEditSendText->toPlainText().size()==0) {
        QMessageBox::information(this,tr("提示"),tr("发送区为空，请输入内容。"));
        return;  //如果发送区为空则直接跳出
    }

    if(ui->cBoxLoopSnd->checkState())
    {
        if( !loopSending){
            ui->pBtnSendData->setText(tr("停止发送"));
            timer->start();
            loopSending=true;
        }else{
            timer->stop();
            ui->pBtnSendData->setText(tr("发送"));
            loopSending=false;
        }
    }else{
        toSendData();
        if(ui->cBox_AntoClearSnd->checkState()){
            ui->tEditSendText->clear();
        }
    }
}

/**********************************************************/
//send data
void mNetAssistWidget::toSendData()
{
    QByteArray datagram;

    if(ui->cBox_SndHexDisp->checkState()){
        QStringList hexStr = ui->tEditSendText->toPlainText().split(" ",QString::SkipEmptyParts);
        int hexSize = hexStr.size();
        for(int i=0;i<hexSize;i++){
            QString hexSubStr = hexStr.at(i);
            datagram.append(ConvertHexStr(hexSubStr));
        }
        datagram.resize(hexSize);
    }else{
        datagram = ui->tEditSendText->toPlainText().toLocal8Bit();
    }

    if(datagram.size()==0)   return;

    if(ui->cBoxNetType->currentIndex() == UDP_MODE){
        udpSocket->writeDatagram(datagram.data(), datagram.size(),rmtAddr, rmtPort);
    }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
        int idx = ui->cBoxClients->currentIndex() ;
        if(idx == 0){
             emit sendDataToClient((char *)datagram.data(), datagram.size(),0,0);
        }else{
             emit sendDataToClient((char *)datagram.data(), datagram.size(),tcpClientSocketDescriptorList.at(idx),0);
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
        tcpClientSocket->write(datagram.data(), datagram.size());
    }

    sndDataCnt+=datagram.size();
    ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
}

/**********************************************************/
void mNetAssistWidget::toSendFile()
{
    if(curFile==0)   return;
    char buf[1024];
    int rdLen=0;
   ui->SndProgressBar->setMaximum(curFile->bytesAvailable());
   ui->SndProgressBar->setVisible(true);
   ui->SndProgressBar->setValue(0);
    if(ui->cBoxNetType->currentIndex() == UDP_MODE){    //UDP 模式

        while(!curFile->atEnd())
        {              
              rdLen = curFile->read(buf,1024);
              udpSocket->writeDatagram(buf, rdLen,rmtAddr, rmtPort);
              sndDataCnt+=rdLen;
              ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
              ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
              msDelay(1);
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){ //TCP服务器模式
        int idx = ui->cBoxClients->currentIndex() ;
        if(idx == 0){
            while(!curFile->atEnd())
            {
                  //msDelay(2);
                  rdLen = curFile->read(buf,1024);
                  emit sendDataToClient(buf, rdLen,0,0);

                  sndDataCnt+=rdLen;
                  ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
                  ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
                  QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
        }else{
            while(!curFile->atEnd())
            {
                 // msDelay(2);
                  rdLen = curFile->read(buf,1024);
                  emit sendDataToClient(buf, rdLen,tcpClientSocketDescriptorList.at(idx),0);
                  sndDataCnt+=rdLen;
                  ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
                  ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
                  QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
        }
    }else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE){
        while(!curFile->atEnd())
        {          
              rdLen = curFile->read(buf,1024);
              tcpClientSocket->write(buf, rdLen);
              //更新发送数据计数器
              sndDataCnt+=rdLen;
              ui->lEdit_SndCnt->setText(QString::number(sndDataCnt,10));
              ui->SndProgressBar->setValue(ui->SndProgressBar->value()+rdLen);
              QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
    }

    ui->SndProgressBar->setVisible(false);
}
/**********************************************************/
//udp data received
void mNetAssistWidget::udpDataReceived()
{
    QHostAddress address;
    quint16 port;
    QString tmpIPPort="";

    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),datagram.size(),&address,&port);

        if(ui->StartRcvFile->checkState()){
            char *buf;
            buf = datagram.data();
             if( curFile!=0) {
                  curFile->write(buf,datagram.size());
             }
        }else{
             if(!ui->cBox_PauseShowRcv->checkState()){
                 tmpIPPort = address.toString()+":"+QString::number(port,10);
                 QString rcvMsg  = QString::fromUtf8(datagram,datagram.size());
                  if(CurIPPort!=tmpIPPort){
                      CurIPPort=tmpIPPort;
                      if(ui->ReceiveTextEdit->toPlainText().size()!=0){
                          ui->ReceiveTextEdit->insertPlainText("\n");
                      }
                       ui->ReceiveTextEdit->insertPlainText(tr("【数据来自")+CurIPPort+tr("】\n"));
                 }
                  //显示数据接收时间
                  if(ui->cBox_ShowRcvTime->checkState())    insertDateTimeInRcvDisp();

                  //输出数据到接收显示区
                  if(!ui->cBox_RcvHexDisp->checkState()){              //显示字符串
                      ui->ReceiveTextEdit->insertPlainText(rcvMsg);
                  }else{                                               //显示十六进制字符串
                      for(int i=0;i<datagram.size();i++){
                          char ch = datagram.at(i);
                          QString tmpStr="";
                          tmpStr.append(toHex[(ch&0xf0)/16]);
                          tmpStr.append(toHex[ch&0x0f]);
                          tmpStr.append(" ");
                          ui->ReceiveTextEdit->insertPlainText(tmpStr);
                      }
                  }
            }
        }
        rcvDataCnt+=datagram.size();
        ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
    }
}

/**********************************************************/
//clear the receive editor
void mNetAssistWidget::on_pBtnClearRcvDisp_clicked()
{
    ui->ReceiveTextEdit->clear();
}

/**********************************************************/
//reset the conters
void mNetAssistWidget::on_pBtnResetCnt_clicked()
{
    sndDataCnt = 0;
    rcvDataCnt = 0;

    ui->lEdit_RcvCnt->setText(QString::number(0,10));
    ui->lEdit_SndCnt->setText(QString::number(0,10));
}

/**********************************************************/
//UDP port changed PRC
void mNetAssistWidget::on_lEditUdpPort_textChanged(QString text)
 {
    rmtPort = text.toInt();
 }

/**********************************************************/
//UDP IP  Adderess changed PRC
void mNetAssistWidget::on_lEditUdpIP_textChanged(QString text)
 {
     rmtAddr.setAddress(text);
 }

/**********************************************************/
// TCP Client data received
void mNetAssistWidget::tcpClientDataReceived()
{
    while(tcpClientSocket->bytesAvailable()>0)
    {
        QByteArray datagram;
        datagram.resize(tcpClientSocket->bytesAvailable());
        tcpClientSocket->read(datagram.data(),datagram.size());
        if(ui->StartRcvFile->checkState()){
            char *buf;
            buf = datagram.data();
             if( curFile!=0) {
                  curFile->write(buf,datagram.size());
             }
        }else{
        if(!ui->cBox_PauseShowRcv->checkState()){
            QString rcvMsg  = QString::fromUtf8(datagram,datagram.size());
            QString tmpIPPort = ui->lEditIpAddr->text()+":"+ui->lEditIpPort->text();
            if(CurIPPort!=tmpIPPort){
               CurIPPort=tmpIPPort;
               if(ui->ReceiveTextEdit->toPlainText().size()!=0){
                  ui->ReceiveTextEdit->insertPlainText("\n");
               }
               ui->ReceiveTextEdit->insertPlainText(tr("【数据来自")+CurIPPort+tr("】\n"));
            }          
            //显示数据接收时间
            if(ui->cBox_ShowRcvTime->checkState())   insertDateTimeInRcvDisp();
            //输出数据到接收显示区
            if(!ui->cBox_RcvHexDisp->checkState()){            //显示字符串
                ui->ReceiveTextEdit->insertPlainText(rcvMsg);
            }else{                                             //显示十六进制字符串
                for(int i=0;i<datagram.size();i++){
                    char ch = datagram.at(i);
                    QString tmpStr="";
                    tmpStr.append(toHex[(ch&0xf0)/16]);
                    tmpStr.append(toHex[ch&0x0f]);
                    tmpStr.append(" ");
                    ui->ReceiveTextEdit->insertPlainText(tmpStr);
                }
            }
        }
        }

         rcvDataCnt+=datagram.size();
         ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
    }
}

void mNetAssistWidget::insertDateTimeInRcvDisp()
{
      int year,month,day;
      QDateTime::currentDateTime().date().getDate(&year,&month,&day);
      QString date = QString::number(year,10)+"-"+QString::number(month,10)+"-"+QString::number(day,10);
      ui->ReceiveTextEdit->appendPlainText(tr("【")+date+tr(" ")+QDateTime::currentDateTime().time().toString()+tr("】"));
}

/**********************************************************/
bool mNetAssistWidget::slotTryCreateTcpServer()
{
    mtcpServer = new mTcpServer(this);

    if(! mtcpServer->listen(lhAddr,lhPort))
    {
        return false;
        QMessageBox::information(this,tr("错误"),tr("尝试建立服务器失败! 请确认网络状态和端口。"));
    }

    connect(mtcpServer,SIGNAL(updateTcpServer(char*,int,int)),this,SLOT(tcpServerDataReceived(char*,int,int)));
    connect(this,SIGNAL(sendDataToClient(char*,int,int,int)),mtcpServer,SLOT(sendDataToClient(char*,int,int,int)));
    connect(mtcpServer,SIGNAL(addClientLink(QString,int)),this,SLOT(addClientLink(QString,int)));
    connect(mtcpServer,SIGNAL(removeClientLink(QString,int)),this,SLOT(removeClientLink(QString,int)));

    return true;
}

/**********************************************************/
void mNetAssistWidget::slotDeleteTcpServer()
{
    //disconnect(mtcpServer,SIGNAL(updateTcpServer(char*,int,int)),this,SLOT(tcpServerDataReceived(char*,int,int)));
    mtcpServer->disconnect();
    mtcpServer->close();
    delete  mtcpServer;
}

/**********************************************************/
void mNetAssistWidget::tcpServerDataReceived(char *msg,int length,int socketDescriptorEx)
{
    if(ui->StartRcvFile->checkState()){     //保存接收数据到文件
         if(curFile!=0) {
                curFile->write(msg,length);
          }
    }else{                                                  //显示接收数据
        if(!ui->cBox_PauseShowRcv->checkState()){
            int idx = tcpClientSocketDescriptorList.indexOf(socketDescriptorEx);
            QString tmpIPPort = ui->cBoxClients->itemText(idx);
            if(CurIPPort!=tmpIPPort){
                 CurIPPort=tmpIPPort;
                 if(ui->ReceiveTextEdit->toPlainText().size()!=0){
                     ui->ReceiveTextEdit->insertPlainText("\n");
                 }
                ui->ReceiveTextEdit->insertPlainText(tr("【数据来自")+CurIPPort+tr("】\n"));
            }
            //显示数据接收时间
            if(ui->cBox_ShowRcvTime->checkState())  insertDateTimeInRcvDisp();

             //输出数据到接收显示区
            if(!ui->cBox_RcvHexDisp->checkState()){  //显示字符串
                ui->ReceiveTextEdit->insertPlainText(msg);
             }else{                                                            //显示十六进制字符串
                 for(int i=0;i<length;i++){
                     char ch =*(msg+i);
                     QString tmpStr="";
                     tmpStr.append(toHex[(ch&0xf0)/16]);
                     tmpStr.append(toHex[ch&0x0f]);
                     tmpStr.append(" ");
                     ui->ReceiveTextEdit->insertPlainText(tmpStr);
                 }
             }
         }
    }
    //群聊功能控制，转发数据
    if(ui->cBox_chatMode->checkState()){
        if(ui->cBox_echoMode->checkState()){
            emit sendDataToClient(msg,length,0,0);
        }else{
            emit sendDataToClient(msg,length,0,socketDescriptorEx);
        }
    }

    //数据接收计数器更新
    rcvDataCnt+=length;
    ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt,10));
}

/**********************************************************/
void mNetAssistWidget::addClientLink(QString clientAddrPort,int socketDescriptor)
{  
    if(TcpClientLinkCnt == 0){
        tcpClientSocketDescriptorList.clear();
        tcpClientSocketDescriptorList.append(0);
        ui->cBoxClients->addItem(tr("全部连接"));
    }
    TcpClientLinkCnt++;
    tcpClientSocketDescriptorList.append(socketDescriptor);
    ui->cBoxClients->addItem(clientAddrPort);
}

/**********************************************************/
void mNetAssistWidget::removeClientLink(QString clientAddrPort,int socketDescriptor)
{
    if(socketDescriptor!=-1) return;
    if(TcpClientLinkCnt <= 1){
        tcpClientSocketDescriptorList.clear();
        ui->cBoxClients->clear();
    }else{
        TcpClientLinkCnt--;
        int idx = ui->cBoxClients->findText(clientAddrPort);
        ui->cBoxClients->removeItem(idx);
        tcpClientSocketDescriptorList.removeAt(idx);
    }
}

/***********************************************/
//保存接收区的文本文件的内容到文件
void mNetAssistWidget::on_pBtnSaveRcvData_clicked()
{
    QString path = QFileDialog::getSaveFileName(this,tr("保存接收区内容到文本文件"),tr(""),tr("文本文件(*.txt)"));

    QFile saveFile(path);
    if (saveFile.open(QFile::WriteOnly | QIODevice::Truncate)) {
         QTextStream out(&saveFile);
         QString  str = ui->ReceiveTextEdit->toPlainText();
         out << str;
    }
    saveFile.close();
}

/***********************************************/
//开启定时自动发送功能
void mNetAssistWidget::on_cBoxLoopSnd_toggled(bool checked)
{
    if(checked){
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(toSendData()));
        int msInterval = ui->lEdit_Interval_ms->text().toInt();
        if(msInterval>0){
           timer->setInterval(ui->lEdit_Interval_ms->text().toInt());
        }else{
            ui->cBoxLoopSnd->setChecked(false);
            delete timer;
       }
    }else{
        timer->stop();
        delete timer;
        ui->pBtnSendData->setEnabled(true);
    }
}

/***********************************************/
//修改发送间隔时间
void mNetAssistWidget::on_lEdit_Interval_ms_editingFinished()
{
    int msInterval = ui->lEdit_Interval_ms->text().toInt();
    if(msInterval>0){
       timer->setInterval(ui->lEdit_Interval_ms->text().toInt());
    }
}

/***********************************************/
//清空发送区
void mNetAssistWidget::on_pBtnClearSndDisp_clicked()
{
    ui->tEditSendText->clear();
}

/***********************************************/
//载入文本文件到发送区
void mNetAssistWidget::on_pBtnLoadSndData_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,tr("载入文本文件到发送区"));
    QFile file(path);
    QByteArray str;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         QMessageBox::information(this,tr("错误"),tr("打开文件失败。"));
         return ;
     } else {
         while (!file.atEnd()) {
              str = file.readLine();
              ui->tEditSendText->insertPlainText(str);
         }
    }
    file.close();
}

/***********************************************/
//接收数据转存到文件
void mNetAssistWidget::on_StartRcvFile_clicked(bool checked)
{
      if(checked){
           QFileDialog *qfd=new QFileDialog(this);
           qfd->setViewMode(QFileDialog::List);
           qfd->setFileMode(QFileDialog::AnyFile);
           qfd->setWindowTitle(tr("建立接收文件"));
          // qfd->setFilter(tr("所有文件(*.*)"));         //qt4.8
           qfd->setNameFilter(tr("所有文件(*.*)"));  //qt5

           if (qfd->exec() == QDialog::Accepted )   //如果成功的执行
           {
                  QStringList slist = qfd->selectedFiles();
                  CurPath = slist[0];
                  curFile = new QFile(CurPath);

                  if (!curFile->open(QFile::WriteOnly | QIODevice::Truncate)) {
                      //打开文件失败
                      ui->StartRcvFile->setChecked(false);
                      return;
                  }
                  ui->ReceiveTextEdit->setPlainText(tr("接收数据保存到文件：\n")+CurPath+tr("\n"));
                  on_pBtnResetCnt_clicked();
             }else{
                  ui->StartRcvFile->setChecked(false);
                  return;
             }
      }else{
           ui->ReceiveTextEdit->clear();
           if(curFile) curFile->close();
           if(curFile) delete curFile;
      }
}

/***********************************************/
//发送数据源为文件
void mNetAssistWidget::on_cBoxStartSndFile_clicked(bool checked)
{
    if(checked){      
        QFileDialog *qfd=new QFileDialog(this);
        qfd->setViewMode(QFileDialog::List);
        qfd->setFileMode(QFileDialog::AnyFile);
        qfd->setWindowTitle(tr("选择发送文件"));
       // qfd->setFilter(tr("所有文件(*.*)"));            //qt4.8
        qfd->setNameFilter(tr("所有文件(*.*)"));   //qt5

         if (qfd->exec() == QDialog::Accepted )   //如果成功的执行
         {
                QStringList slist = qfd->selectedFiles();
                CurPath = slist[0];
                curFile = new QFile(CurPath);

                if (!curFile->open(QFile::ReadOnly | QIODevice::Truncate)) {
                         //打开文件失败
                    ui->cBoxStartSndFile->setChecked(false);
                    return;
                 }
                ui->ReceiveTextEdit->setPlainText(tr("从文件发送数据：\n")+CurPath+tr("\n"));
          }else{
               ui->cBoxStartSndFile->setChecked(false);
               return;
         }
    }else{
        ui->ReceiveTextEdit->clear();
        if(curFile) curFile->close();
        if(curFile) delete curFile;
    }
}

/***********************************************/
//发送区以十六进制与字符串间切换
void mNetAssistWidget::on_cBox_SndHexDisp_clicked(bool checked)
{
    QByteArray datagram;
    if(checked){
        if(ui->tEditSendText->toPlainText().length()!=0){
             datagram = ui->tEditSendText->toPlainText().toLocal8Bit();
             ui->tEditSendText->clear();
             for(int i=0;i<datagram.size();i++){
                 char ch = datagram.at(i);
                 QString tmpStr = QString::number(ch,16);
                 ui->tEditSendText->insertPlainText(tmpStr+" ");
             }
        }
    }else{
         if(ui->tEditSendText->toPlainText().length()!=0){
             QStringList hexStr = ui->tEditSendText->toPlainText().split(" ",QString::SkipEmptyParts);
             int hexSize = hexStr.size();
             qDebug()<<QString::number(hexSize,10);
             for(int i=0;i<hexSize;i++){
                 QString hexSubStr = hexStr.at(i);
                 datagram.append(ConvertHexStr(hexSubStr));
             }
             ui->tEditSendText->clear();
             QString msg = datagram.data();
             ui->tEditSendText->setPlainText(msg);
         }
    }
}

/***********************************************/
//转化十六进制中的字符到ASCII
char mNetAssistWidget::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

/***********************************************/
//转化十六进制字符到ASCII字母
char mNetAssistWidget::ConvertHexStr(QString hexSubStr)
{
    char ch = 0;
    if(hexSubStr.length()==2){
      // ch =  ConvertHexChar(hexSubStr.at(0).toAscii())*16+ ConvertHexChar(hexSubStr.at(1).toAscii()); //qt4.8
        ch =  ConvertHexChar(hexSubStr.at(0).toLatin1())*16+ ConvertHexChar(hexSubStr.at(1).toLatin1());
    }else if(hexSubStr.length()==1){
       //ch =  ConvertHexChar(hexSubStr.at(0).toAscii());  //qt4.8
       ch =  ConvertHexChar(hexSubStr.at(0).toLatin1());
    }
    return ch;
}

/***********************************************/
//毫秒级延时
void mNetAssistWidget::msDelay(unsigned int msec)
{
     QTime dieTime = QTime::currentTime().addMSecs(msec);
     while( QTime::currentTime() < dieTime )
      QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
