#ifndef MAINWIGT_H
#define MAINWIGT_H

#include <QWidget>
#include<QtSerialPort/QSerialPort>
#include<QtSerialPort/QSerialPortInfo>
#include<QDebug>
#include<QTimer>
#include<QStandardItemModel>
#include<QStandardItem>
#include<QMessageBox>
#include<QByteArray>
#include <windows.h>
#include<QTimer>
#include<QTime>

#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR -1
#endif


typedef struct
{
    uint8_t frm_head = 68;   //报文头指针
    uint8_t frm_len;    //报文长度
    uint8_t frm_addr;   //设备地址
    uint8_t frm_afn;    //功能码
    uint8_t frm_fn;     //校准命令
    uint8_t frm_dlen;   //数据长度
    uint8_t frm_crc;    //校验和
    uint8_t frm_end = 16;    //报尾指针
    uint8_t frm_data[50];  //数据域
} s_rcvdata;

namespace Ui {
class mainwigt;
}

class mainwigt : public QWidget
{
    Q_OBJECT

public:
    explicit mainwigt(QWidget *parent = 0);
    ~mainwigt();

private slots:
    void on_pB_findRTU_clicked();
    void on_pB_linkRTU_clicked(bool checked);
    void on_pB_readvalue_clicked();
    void on_pB_readparam_clicked();
    void on_pB_setparam_clicked();

    void slotGetDate();
    void slotCmdBtnClick();


    void on_pB_clearlog_clicked();


    void on_pB_findID_clicked();

    void on_chB_isloop_clicked(bool checked);

private:
    Ui::mainwigt *ui;
    QSerialPort my_serialport;//新建一个串口
    QStandardItemModel* Model1;//表格1
    QStandardItemModel* Model2;//表格2
    s_rcvdata t_rcvdata;
    QTimer timer_loop;

    uint8_t * rcvbuf;  //接收到的报文

    QByteArray rcvba_old;//缓存
    int getlen;             //接收到的报文长度
    bool init_RTU();
    bool init_Tbl1();
    bool init_Tbl2();
    int loop_unpack();      //解包函数
    void funcAnalytic();     //解析函数
    void func_get_ff_value();//广播获取数据
    QByteArray RTU_readAll();
    int RTU_read(char * rcvbuf);
    int RTU_write(char *date, int len);
    int RTU_write(QByteArray &date);
    uint8_t sumCRC(uint8_t *buf, int len);


    //复制来的
    QString byteArrayToHexStr(const QByteArray &data);
    QByteArray hexStrToByteArray(const QString &str);
    char convertHexChar(char ch);
    QStringList byteArrayToHexStrList(const QByteArray &data);
    QString hexTofloat(QString strHex);
    QString hexToDec(QString strHex);
    QString floatTohex(QString str_float);
    QByteArray getCSCode(const QByteArray &data);

};

#endif // MAINWIGT_H
