#include "mainwigt.h"
#include "ui_mainwigt.h"
#include<stdio.h>
mainwigt::mainwigt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainwigt)
{
    ui->setupUi(this);
    this->setWindowTitle(QString("校准软件")+QString(__DATE__));

    ui->le_ID->setText("0"); //当前设备地址为0，初始化


    init_RTU();
    init_Tbl1();
    init_Tbl2();
    connect(&my_serialport,SIGNAL(readyRead()),this,SLOT(slotGetDate()));//有数据就读

    connect(&timer_loop, SIGNAL(timeout()), this, SLOT(on_pB_readvalue_clicked()));

    connect(ui->pB_J_1_1,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_1_2,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_1_3,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_2_1,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_2_2,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_2_3,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_I_1,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
    connect(ui->pB_J_I_2,SIGNAL(pressed()),this,SLOT(slotCmdBtnClick()));
}

mainwigt::~mainwigt()
{
    delete ui;
    this->my_serialport.close();
}

bool mainwigt::init_RTU()
{
    //枚举所有串口
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        //这里相当于自动识别串口号之后添加到了界面里的索引cb_port中去
        QSerialPort serial;
        serial.setPort(info);
        if (serial.open(QIODevice::ReadWrite))
        {
           ui->cB_RTU->addItem(info.portName());
            serial.close();
        }
    }
    return OK;
}

bool mainwigt::init_Tbl1()
{
    Model1= new QStandardItemModel();
    ui->tbV_value->horizontalHeader()->setVisible(0);
    ui->tbV_value->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QStringList titles = QStringList()<<"系统时间"<<"软件版本"<<"电压1"<<"电压2"<<"阻值1"<<"阻值2"<<"电流"<<"电压1采样值"<<"电压2采样值"<<"电流采样值";
    Model1->setVerticalHeaderLabels(titles);
    QStandardItem *item ;
    for(int i =0;i<titles.count();i++)
    {
       item= new QStandardItem(" ");
       Model1->setItem(i,0,item);
    }
    ui->tbV_value->setModel(Model1);
   // ui->tbV_value->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑项
    ui->tbV_value->setSelectionMode(QAbstractItemView::SingleSelection);//单选
    ui->tbV_value->setSelectionBehavior(QAbstractItemView::SelectRows);//选择行

    return OK;
}

bool mainwigt::init_Tbl2()
{
    Model2= new QStandardItemModel();
    ui->tbV_param->horizontalHeader()->setVisible(0);
    ui->tbV_param->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QStringList titles = QStringList()<<"模块地址"<<"设备编号"<<"电压1曲线斜率1"<<"电压1曲线截距1"<<"电压1曲线斜率2"<<"电压1曲线截距2"
                                     <<"电压2曲线斜率1"<<"电压2曲线截距1"<<"电压2曲线斜率2"<<"电压2曲线截距2"
                                    <<"电流曲线斜率"<<"电流曲线截距";
    Model2->setVerticalHeaderLabels(titles);
    ui->tbV_param->setModel(Model2);
    QStandardItem *item ;
    for(int i =0;i<titles.count();i++)
    {
       item= new QStandardItem(" ");
       Model2->setItem(i,0,item);
    }
    // ui->tbV_param->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑项
    ui->tbV_param->setSelectionMode(QAbstractItemView::SingleSelection);//单选
    ui->tbV_param->setSelectionBehavior(QAbstractItemView::SelectRows);//选择行


    return OK;
}


///循环解包
int mainwigt::loop_unpack()
{
    qDebug()<<"进解包";
    uint16_t pos = 0;
    uint16_t dlen = 0;

    while(pos < getlen)
    {
      if ((getlen - pos) < 7)//必要部分：头+长度+地址+afn+fn+crc+end
      {
          QString txt ;
          for(int i = 0;i<getlen;i++)
          {
              txt.append(QString("%1 ").arg(rcvbuf[i], 2, 16, QLatin1Char('0')));
          }
           qDebug()<<tr("存入缓存:")+txt;

          return -1;
      }

      if (0x68 == rcvbuf[pos])
      {
          //qDebug()<<"头部对上";
          dlen = rcvbuf[pos + 1] ;

          if (0x16 == rcvbuf[pos + dlen - 1])
          {
              // qDebug()<<"尾部对上";
              if (rcvbuf[pos + dlen - 2] == sumCRC(rcvbuf + pos, dlen - 2))
              {
                  //qDebug()<<"校验对上";
                  memset(&t_rcvdata, 0x00, sizeof(s_rcvdata));
                  //qDebug()<<"清空结构体";

                  QString txt_new ="【receive】获得报文:";
                  txt_new.append(QString("%1 ").arg(68));

                  t_rcvdata.frm_len = dlen;
                  txt_new.append(QString("%1 ").arg(t_rcvdata.frm_len, 2, 16, QLatin1Char('0')));
                  //qDebug()<<"报文长度"<<t_rcvdata.frm_len;

                  t_rcvdata.frm_addr = rcvbuf[pos + 2];
                  txt_new.append(QString("%1 ").arg(t_rcvdata.frm_addr, 2, 16, QLatin1Char('0')));
                  //qDebug()<<"设备地址"<<t_rcvdata.frm_addr;

                  t_rcvdata.frm_afn = rcvbuf[pos + 3];
                  txt_new.append(QString("%1 ").arg(t_rcvdata.frm_afn, 2, 16, QLatin1Char('0')));
                  //qDebug()<<"功能码"<<t_rcvdata.frm_afn;

                  t_rcvdata.frm_fn = rcvbuf[pos + 4];
                  txt_new.append(QString("%1 ").arg(t_rcvdata.frm_fn, 2, 16, QLatin1Char('0')));
                  //qDebug()<<"类型"<<t_rcvdata.frm_fn;

                  t_rcvdata.frm_dlen = dlen - 5 - 2;
                  //qDebug()<<"数据长度"<<t_rcvdata.frm_dlen;
                  memcpy(&t_rcvdata.frm_data, rcvbuf + pos + 5, dlen -7);

                  for(int i =0;i<t_rcvdata.frm_dlen;i++)
                  {
                      //txt_new.append(QString("%1 ").arg(&t_rcvdata.frm_data));
                      txt_new.append(QString("%1 ").arg(t_rcvdata.frm_data[i], 2, 16, QLatin1Char('0')));
                  }

                  t_rcvdata.frm_crc = rcvbuf[pos + dlen - 2];
                  txt_new.append(QString("%1 ").arg(rcvbuf[pos + dlen - 2],2,16,QLatin1Char('0')));

                  txt_new.append("16\n");
                  QString txt_old = ui->tB_log->toPlainText();
                  ui->tB_log->setText(txt_old+txt_new);
                  funcAnalytic();

                  rcvba_old.clear();
                  return pos + dlen;
              }
          }
      }

      pos++;
    }
    qDebug()<<tr("报文过短");
    return ERROR;
}

///解析报文
void mainwigt::funcAnalytic()
{
    qDebug()<<"进解析";
    switch (t_rcvdata.frm_afn)
    {
    case 0://确认返回，（设置和命令返回）
    {

        uint8_t fhex[4];
        memcpy(&fhex,&t_rcvdata.frm_data,4);
        qDebug()<<"解析确认"<<fhex[0];
        if(fhex[0]==0)
            QMessageBox::information(this,tr("提示"),tr("确认！"));
        else
            QMessageBox::information(this,tr("提示"),tr("否认！"));
        break;
    }
    case 1://实时
    {
        qDebug()<<"解析实时";
        uint8_t fhex[4];
        QString tmpstr, fv;
        int tmpi ;
        bool ok;
        QStandardItem *item ;
        ui->le_ID->setText(QString::number(t_rcvdata.frm_addr));

        memcpy(&fhex,&t_rcvdata.frm_data,4);
        tmpstr = QString("%1%2%3%4").arg(fhex[3], 2, 16, QLatin1Char('0')).arg(fhex[2], 2, 16, QLatin1Char('0'))
                .arg(fhex[1], 2, 16, QLatin1Char('0')).arg(fhex[0], 2, 16, QLatin1Char('0'));
        tmpi = tmpstr.toInt(&ok,16);
        QString tmptimestr = QDateTime::fromTime_t(tmpi).toString("yyyy-MM-dd HH:mm:ss");
        item= new QStandardItem(tmptimestr);
        Model1->setItem(0,0,item);
        //qDebug()<<"系统时间"<<tmpi;

        for(int i = 1;i<=9;i++)
        {
            memcpy(&fhex,&t_rcvdata.frm_data[4*i],4);
            tmpstr = QString("%1%2%3%4").arg(fhex[3], 2, 16, QLatin1Char('0')).arg(fhex[2], 2, 16, QLatin1Char('0'))
                    .arg(fhex[1], 2, 16, QLatin1Char('0')).arg(fhex[0], 2, 16, QLatin1Char('0'));
            //qDebug()<<"hex[i]:"<<tmpstr;
            fv = hexTofloat(tmpstr);
            item= new QStandardItem(QString("%1").arg(fv));
            Model1->setItem(i,0,item);
            //qDebug()<<"float[i]:"<<fv;
        }


        break;
    }
    case 2://参数
    {
        qDebug()<<"解析参数";
        uint8_t fhex[4];
        QString tmpstr, fv;
        int tmpi ;
        bool ok;
        QStandardItem *item ;


        memcpy(&fhex,&t_rcvdata.frm_data[0],1);
        tmpstr = QString("%1").arg(fhex[0], 2, 16, QLatin1Char('0'));
        tmpi = tmpstr.toInt(&ok,16);
        item= new QStandardItem(QString::number(tmpi));
        Model2->setItem(0,0,item);
        //qDebug()<<"模块地址"<<tmpi;


        memcpy(&fhex,&t_rcvdata.frm_data[1],2);
        tmpstr = QString("%1%2").arg(fhex[1], 2, 16, QLatin1Char('0')).arg(fhex[0], 2, 16, QLatin1Char('0'));
        tmpi = tmpstr.toInt(&ok,16);
        item= new QStandardItem(QString::number(tmpi));
        Model2->setItem(1,0,item);
        //qDebug()<<"设备编号"<<tmpi;


        for(int i = 0;i<=9;i++)
        {
            memcpy(&fhex,&t_rcvdata.frm_data[4*i+3],4);
            tmpstr = QString("%1%2%3%4").arg(fhex[3], 2, 16, QLatin1Char('0')).arg(fhex[2], 2, 16, QLatin1Char('0'))
                    .arg(fhex[1], 2, 16, QLatin1Char('0')).arg(fhex[0], 2, 16, QLatin1Char('0'));
            //qDebug()<<"hex[i]:"<<tmpstr;
            fv = hexTofloat(tmpstr);
            item= new QStandardItem(QString("%1").arg(fv));
            Model2->setItem(i+2,0,item);
            //qDebug()<<"float[i]:"<<fv;
        }
        break;
    }
    default:
        break;
    }
}

int mainwigt::RTU_read(char * rcvbuf)
{
    return this->my_serialport.read(rcvbuf,this->my_serialport.bytesAvailable());
}

QByteArray mainwigt::RTU_readAll()
{
    QByteArray  buf;
    while(my_serialport.bytesAvailable()>0)
    {
        buf += this->my_serialport.readAll() ;
        //Sleep(200);
    }
    return buf;
}

int mainwigt::RTU_write(char *date,int len)
{
    return this->my_serialport.write(date,len);
}

int mainwigt::RTU_write(QByteArray &date)
{
    return this->my_serialport.write(date);
}

uint8_t mainwigt::sumCRC(uint8_t *buf, int len)
{
    int sum = 0;
    for (int i = 0; i < len; i++)
    {
        sum += buf[i];
    }
    return sum & 0xFF;
}

void mainwigt::on_pB_findRTU_clicked()
{
    ui->cB_RTU->clear();
    init_RTU();
}

void mainwigt::on_pB_linkRTU_clicked(bool checked)
{
    if(checked)
     {
         my_serialport.setPortName(ui->cB_RTU->currentText());//选择串口
         if(!my_serialport.open(QIODevice::ReadWrite))
         {
             QMessageBox::warning(this,tr("警告"),tr("串口打开失败！"));
             ui->pB_linkRTU->setChecked(0);
             return;
         }


         my_serialport.setBaudRate(QSerialPort::Baud115200);//设置串口参数
         my_serialport.setDataBits(QSerialPort::Data8);
         my_serialport.setParity(QSerialPort::NoParity);
         my_serialport.setStopBits(QSerialPort::OneStop);
         my_serialport.setFlowControl(QSerialPort::NoFlowControl);

         ui->cB_RTU->setEnabled(0);//设置控件是否启用
         ui->cB_baud->setEnabled(0);
         ui->pB_findRTU->setEnabled(0);



         ui->lb_stateRTU->setText(tr("已连接"));//设置文本信息
         ui->pB_linkRTU->setText(tr("断开"));

         func_get_ff_value();

     }
    else{
            my_serialport.close();


            ui->cB_RTU->setEnabled(1);//设置控件是否启用
            ui->cB_baud->setEnabled(1);
            ui->pB_findRTU->setEnabled(1);


            ui->lb_stateRTU->setText(tr("已断开"));//设置文本信息
            ui->pB_linkRTU->setText(tr("连接"));

            ui->chB_isloop->setChecked(0);//周期读取自动关闭
            on_chB_isloop_clicked(0);
    }
}


///糙函数
void mainwigt::slotGetDate()
{
    rcvba_old += RTU_readAll();
    rcvbuf =(unsigned char*) rcvba_old.data();
    qDebug()<<"收到报文";
    getlen = rcvba_old.count();


    loop_unpack();//挑出一条报文


}


void mainwigt::func_get_ff_value()
{

    //报文68 07 00 01 00 70 16
    unsigned char sendbuf[] = {0x68, 0x07 ,0xFF, 0x01 ,0x00 ,0x6F ,0x16};
    char *sdbf = (char *)sendbuf;

    QString txt_new = "【send】广播读取:";
    for(int i = 0;i<7;i++)
    {
        txt_new.append(QString("%1 ").arg(sendbuf[i], 2, 16, QLatin1Char('0')));

    }
    txt_new .append("\n");
    QString txt_old = ui->tB_log->toPlainText();

    if(0< RTU_write(sdbf,7))
    {
        ui->tB_log->setText(txt_old+txt_new);
    }
}

///读取数据
void mainwigt::on_pB_readvalue_clicked()
{

    bool ok;
    if(!my_serialport.isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("串口未打开！"));
        return ;
    }
    QByteArray buff;
    buff[0] = 0x68;
    buff[1] = 0x07;
    buff[2] = ui->le_ID->text().toInt();
    buff[3] = 0x01;
    buff[4] = 0x00;
    QByteArray CSnum = getCSCode(buff);
    buff.append(CSnum );
    buff.append(0x16);

    QString txt_new = "【send】读取数据:";
    for(int i = 0;i<7;i++)
    {
        QString onebuff =QString("%1").arg(buff.at(i), 2, 16, QLatin1Char('0')) .right(2);
        //qDebug()<<QString("第%1位字节字符:%2 ").arg(i).arg(onebuff);
        txt_new.append(QString("%1 ").arg(onebuff));
    }
    txt_new .append("\n");
    QString txt_old = ui->tB_log->toPlainText();

    if(0< RTU_write(buff))
    {
        ui->tB_log->setText(txt_old+txt_new);
    }
}

///读取参数
void mainwigt::on_pB_readparam_clicked()
{
    if(!my_serialport.isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("串口未打开！"));
        return ;
    }
    QByteArray buff;
    buff[0] = 0x68;
    buff[1] = 0x07;
    buff[2] = ui->le_ID->text().toInt();
    buff[3] = 0x02;
    buff[4] = 0x00;
    QByteArray CSnum = getCSCode(buff);
    buff.append(CSnum);
    buff.append(0x16);

    QString txt_new = "【send】读取参数:";
    for(int i = 0;i<7;i++)
    {
        QString onebuff =QString("%1").arg(buff.at(i), 2, 16, QLatin1Char('0')) .right(2);
        txt_new.append(QString("%1 ").arg(onebuff));
    }
    txt_new .append("\n");
    QString txt_old = ui->tB_log->toPlainText();

    if(0< RTU_write(buff))
    {
        ui->tB_log->setText(txt_old+txt_new);
    }
}

///设置参数
void mainwigt::on_pB_setparam_clicked()
{
    if(!my_serialport.isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("串口未打开！"));
        return ;
    }
    //报文68 0A 00 03 00 01 00 01 77 16
    QByteArray buff;
    int tmpi;


    buff[0]= 0x68;
    buff[1]= 0x0A;
    buff[2]= ui->le_ID->text().toInt();
    buff[3]= 0x03;
    buff[4]= 0x00;
    tmpi = ui->tbV_param->model()->index(0,0).data().toInt();
    buff[5]= tmpi;          //新的设备地址

    tmpi = ui->tbV_param->model()->index(1,0).data().toInt();
    buff[6] = tmpi % 256;
    buff[7] = tmpi / 256;
    QByteArray CSnum = getCSCode(buff);
    buff.append(CSnum);
    buff.append(0x16);


    QString txt_new = "【send】设置参数:";
    for(int i = 0;i<10;i++)
    {
        QString onebuff =QString("%1").arg(buff.at(i), 2, 16, QLatin1Char('0')) .right(2);
        txt_new.append(QString("%1 ").arg(onebuff));
    }
    txt_new .append("\n");
    QString txt_old = ui->tB_log->toPlainText();

    if(RTU_write(buff)>0)
    {
        ui->tB_log->setText(txt_old+txt_new);
    }

}

///校准命令
void mainwigt::slotCmdBtnClick()
{
    if(!my_serialport.isOpen())
    {
        QMessageBox::warning(NULL,tr("提示"),tr("串口未打开"));
        return;
    }


    QPushButton* button = qobject_cast<QPushButton*>(sender());

    int btnwho = button->statusTip().toInt();
    QString tmpstr,tmpv;

    switch (btnwho)
    {
        case 0:
            tmpstr = ui->le_J_1_1->text();
            break;
        case 1:
            tmpstr = ui->le_J_1_2->text();
            break;
        case 2:
            tmpstr = ui->le_J_1_3->text();
            break;
        case 3:
            tmpstr = ui->le_J_2_1->text();
            break;
        case 4:
            tmpstr = ui->le_J_2_2->text();
            break;
        case 5:
            tmpstr = ui->le_J_2_3->text();
            break;
        case 6:
            tmpstr = ui->le_J_I_1->text();
            break;
        case 7:
            tmpstr = ui->le_J_I_2->text();
            break;
    }


    if( tmpstr.isEmpty())
    {
        QMessageBox::warning(NULL,"提示","校准值为空");
        return;
    }
    else
    {
        tmpv = floatTohex(tmpstr);
        //qDebug()<<"校准值"<<tmpv;
    }

    QByteArray buff;
    buff[0]= 0x68;          //头
    buff[1]= 0x0B;          //长度
    buff[2]= ui->le_ID->text().toInt();      //地址
    buff[3]= 0x04;          //afn
    buff[4]= btnwho+1;      //命令类型
    buff.append(hexStrToByteArray(tmpv));//数据：校准值
    QByteArray CSNum = getCSCode(buff);
    buff.append(CSNum);     //校验码
    buff.append(0x16);      //尾

    QString txt_new = "【send】设置参数:";
    for(int i = 0;i<11;i++)
    {
        QString onebuff =QString("%1").arg(buff.at(i), 2, 16, QLatin1Char('0')) .right(2);
        txt_new.append(QString("%1 ").arg(onebuff));
    }
    txt_new .append("\n");
    QString txt_old = ui->tB_log->toPlainText();

    if(RTU_write(buff)>0)
    {
        ui->tB_log->setText(txt_old+txt_new);
    }
}

///清空文本框
void mainwigt::on_pB_clearlog_clicked()
{
    ui->tB_log->clear();
}

///查找ID
void mainwigt::on_pB_findID_clicked()
{
    if(!my_serialport.isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("串口未打开！"));
        return ;
    }
    func_get_ff_value();
}

///周期读取
void mainwigt::on_chB_isloop_clicked(bool checked)
{


    if(checked)
    {
       int tm = ui->le_looptime->text().toInt();
       if(tm<50)
       {
           QMessageBox::warning(NULL,"提示","时间间隔太小");
           return;
       }
       timer_loop.start(tm);
    }
    else
    {
        timer_loop.stop();
    }
}


/** 1.数据类型转换 **/
//array-hexstr
QString mainwigt::byteArrayToHexStr(const QByteArray &data)
{
    QString temp = "";  //新建字符串
    //qDebug()<<"642data="<<data;
    QString hex = data.toHex();//传进来的数据转为hex类型
    //qDebug()<<"644hex="<<hex;
    for (int i = 0; i < hex.length(); i = i + 2)
    {
        temp += hex.mid(i, 2) + " ";
    }

    return temp.trimmed().toUpper();//全部变大写
}
//hexstr-array
QByteArray mainwigt::hexStrToByteArray(const QString &str)
{
    QByteArray senddata;
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len / 2);
    char lstr, hstr;

    for (int i = 0; i < len;)
    {
        hstr = str.at(i).toLatin1();
        if (hstr == ' ') {
            i++;
            continue;
        }

        i++;
        if (i >= len) {
            break;
        }

        lstr = str.at(i).toLatin1();
        hexdata = convertHexChar(hstr);
        lowhexdata = convertHexChar(lstr);

        if ((hexdata == 16) || (lowhexdata == 16)) {
            break;
        } else {
            hexdata = hexdata * 16 + lowhexdata;
        }

        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }

    senddata.resize(hexdatalen);
    return senddata;
}
//数字转换//hex-10进制
char mainwigt::convertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9')) {
        return ch - 0x30;
    } else if ((ch >= 'A') && (ch <= 'F')) {
        return ch - 'A' + 10;
    } else if ((ch >= 'a') && (ch <= 'f')) {
        return ch - 'a' + 10;
    } else {
        return (-1);
    }
}
//根据第一条改//array-hexlist
QStringList mainwigt::byteArrayToHexStrList(const QByteArray &data)
{

    QString temp = "";  //新建字符串
    QString hex = data.toHex();//传进来的数据转为hex类型
    QStringList outlist;

    for (int i = 0; i < hex.length(); i = i + 2)
    {
        temp += hex.mid(i, 2) + " ";
    }


    QString tmp2 = temp.trimmed().toUpper();
    outlist = tmp2.split(" ");
    return outlist;
}
//网上看到的 //hexstr-floatstr
//****全局函数****-16进制转换工具//十六进制转换工具
int hex2(unsigned char ch){
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    return 0;
}
//转换为浮点数（字符串）
QString mainwigt::hexTofloat(QString strHex)
{
    QString tmp_str = strHex;//进来的保存一下

    //int c = hexToDec(strHex).toInt();
    int v = 0;
    for(int i=0;i<tmp_str.length();i++)
    {
        v*=16;
        v+=hex2(tmp_str[i].toLatin1());
    }
    int c = QString::number(v).toInt();
    float d = *(float*)&c;

    //QString radiation = QString("%1").arg(d);
    QString radiation = QString::number(d,'f',3);
    return radiation;

}
//十六进制转十进制
QString mainwigt::hexToDec(QString strHex)
{
    int v = 0;
    for(int i=0;i<strHex.length();i++)
    {
        v*=16;
        v+=hex2(strHex[i].toLatin1());
    }
    return QString::number(v);
}
//浮点数转换为16进制数（字符串）
QString mainwigt::floatTohex(QString str_float)
{
    QString strFloat = str_float;
    float f = strFloat.toFloat();
    //qDebug()<<"字符串转浮点数："<<f;
    int i = *((int *)&f);
    //qDebug()<<"浮点数强转整数"<<i;
    QString float2 = QString("%1").arg(i,8,16,QLatin1Char('0')).right(8);
    //qDebug()<<"16进制的字符串："<<float2;
    QString tmp_float =float2;
    float2=tmp_float.mid(6,2)+tmp_float.mid(4,2)+tmp_float.mid(2,2)+tmp_float.mid(0,2);//倒序
    return float2;
}
//求和校验
QByteArray mainwigt::getCSCode(const QByteArray &data)
{
    //qDebug()<<"要求和的数据："<<data;
    int sum =0x00;
    int num =data.length();
    for (int i=0;i<num;i++)//
    {
        sum=sum+int(data[i]) ;

    }
    //qDebug()<<"求和结果："<<sum;
    QByteArray cs;
    cs.append(sum & 0xff);
    //qDebug()<<"求和输出："<<cs.data();
    return cs;
}







