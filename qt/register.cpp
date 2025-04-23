#include "register.h"
#include "ui_register.h"

Register::Register(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Register)
{
    ui->setupUi(this);
    password_edit = new My_lineEdit(ui->mydlg);             //mydlg是QWidget类型，传入作为编辑框的父对象
    password_edit->setObjectName("password");
    password_edit->setGeometry(QRect(670, 340, 181, 31));
    password_edit->setAutoFillBackground(false);            //如果true Qt 就会采用默认的背景填充行为，通常是使用系统的背景色。
    password_edit->setStyleSheet(QString::fromUtf8(""));    //qss样式表为空
    password_edit->setMaxLength(6);                         //限定输入的最大字符数
    password_edit->setEchoMode(QLineEdit::Password);
    password_edit->setClearButtonEnabled(true);             //设置是否可点击一键清除编辑框中内容button

    usr_edit = new My_lineEdit(ui->mydlg);
    usr_edit->setObjectName("username");
    usr_edit->setGeometry(QRect(670, 240, 181, 31));
    usr_edit->setStyleSheet(QString::fromUtf8(""));
    usr_edit->setText(QString::fromUtf8(""));
    usr_edit->setMaxLength(4);
    usr_edit->setClearButtonEnabled(true);

    /* 实例化两个输入键盘对话框对象 */
    syszuxpinyin_passwd =new SyszuxPinyin();
    syszuxpinyin_usrname=new SyszuxPinyin();

    //这个send_show(QString)信号在my_lineedit.cpp中重写的mousePressEvent方法中发送，只要点击相应编辑框就会发送该信号,槽函数显示出键盘
    connect(usr_edit,SIGNAL(send_show(QString)),this,SLOT(keyboardshow_usrnameSlot(QString)));

    /* 点击ok发送此信号 */
    connect(syszuxpinyin_usrname,SIGNAL(sendPinyin(QString)),this,SLOT(confirmString_usrnameSlot(QString)));

    connect(password_edit,SIGNAL(send_show(QString)),this,SLOT(keyboardshow_passwdSlot(QString)));
    connect(syszuxpinyin_passwd,SIGNAL(sendPinyin(QString)),this,SLOT(confirmString_passwdSlot(QString)));

    connect(ui->pushButton_register,SIGNAL(clicked()),this,SLOT(pushbutton_registerSlot()));
    connect(ui->pushButton_exit_2,SIGNAL(clicked()),this,SLOT(pushbutton_exitSlot()));
}

Register::~Register()
{
    delete ui;
}
void Register::keyboardshow_passwdSlot(QString data)
{

    syszuxpinyin_passwd->lineEdit_window->setText(data);
    syszuxpinyin_passwd->resize(800,310);
    syszuxpinyin_passwd->move(120,300);
    syszuxpinyin_passwd->show();

}
void  Register::confirmString_passwdSlot(QString gemfield)
{
   password_edit->setText(gemfield);
}
void Register::keyboardshow_usrnameSlot(QString data)
{

    syszuxpinyin_usrname->lineEdit_window->setText(data);
    syszuxpinyin_usrname->resize(800,310);
    syszuxpinyin_usrname->move(120,300);
    syszuxpinyin_usrname->show();

}
void Register::confirmString_usrnameSlot(QString gemfield)
{
   usr_edit->setText(gemfield);
}
void Register::pushbutton_registerSlot()
{
    insertusr();
}
void Register::pushbutton_exitSlot()
{
    close();
}
bool Register::insertusr()
{
    QString name = usr_edit->text();
    QString password = password_edit->text();
    if (name == ""||password == "")
    {
        QMessageBox::warning(this,"警告","用户名密码不能为空");
        return false;
    }
    //创建一个空的 QSqlQuery 对象,将与数据库交互，执行 SQL 查询或命令。
    QSqlQuery sq;
    //防止sql注入问题
    QString sql = "INSERT INTO usr VALUES(?,?)";//? 是占位符，表示后续会通过绑定值的方式填充这些参数。

    /* 预处理,prepare() 会将 SQL 语句提交给数据库进行预编译。
     * 这意味着 SQL 语句中的占位符（?）将在后续执行时被实际的值所替代，
     * 而不是直接将用户输入插入到查询中。
     */
    sq.prepare(sql);
    //占位符数据替换,将实际的参数值绑定到 SQL 语句中的占位符（?）上。
    sq.addBindValue(name);
    sq.addBindValue(password);
    //执行sql语句
    if(sq.exec())
    {
        QMessageBox::information(this,"通知","注册成功");
    }
    else
    {
        //获取最近一次执行的 SQL 查询的错误信息。
        QString errorMsg = sq.lastError().text();
        // Qt 中用于显示错误信息框的静态方法。它会弹出一个对话框，通常用于提示用户某些操作失败或出现错误。
        QMessageBox::critical(this,"错误",errorMsg);
        return false;
    }
    return true;
}
