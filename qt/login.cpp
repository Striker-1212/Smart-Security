#include "login.h"
#include "ui_login.h"
//QLineEdit
extern int login_flag;
Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    password_edit = new My_lineEdit(ui->mydlg);
    password_edit->setObjectName("password");
    password_edit->setGeometry(QRect(670, 340, 181, 31));
    password_edit->setAutoFillBackground(false);
    password_edit->setStyleSheet(QString::fromUtf8(""));
    password_edit->setMaxLength(6);
    password_edit->setEchoMode(QLineEdit::Password);
    password_edit->setClearButtonEnabled(false);

    usr_edit = new My_lineEdit(ui->mydlg);
    usr_edit->setObjectName("username");
    usr_edit->setGeometry(QRect(670, 240, 181, 31));
    usr_edit->setStyleSheet(QString::fromUtf8(""));
    usr_edit->setText(QString::fromUtf8(""));
    usr_edit->setMaxLength(4);
    usr_edit->setClearButtonEnabled(true);
    syszuxpinyin_passwd =new SyszuxPinyin();
    syszuxpinyin_usrname=new SyszuxPinyin();

    connect(usr_edit,SIGNAL(send_show(QString)),this,SLOT(keyboardshow_usrnameSlot(QString)));
    connect(syszuxpinyin_usrname,SIGNAL(sendPinyin(QString)),this,SLOT(confirmString_usrnameSlot(QString)));
    connect(password_edit,SIGNAL(send_show(QString)),this,SLOT(keyboardshow_passwdSlot(QString)));
    connect(syszuxpinyin_passwd,SIGNAL(sendPinyin(QString)),this,SLOT(confirmString_passwdSlot(QString)));
    connect(ui->pushButton_login,SIGNAL(clicked()),this,SLOT(pushbutton_loginSlot()));
    connect(ui->pushButton_exit,SIGNAL(clicked()),this,SLOT(pushbutton_exitSlot()));

}

Login::~Login()
{
    delete ui;
}

/*将拼音输入框的内容设置为 data。
 调整拼音输入框的大小为 800x310。
 将拼音输入框移动到 (120, 300) 位置。
 显示拼音输入框。
 */
void Login::keyboardshow_passwdSlot(QString data)
{

    syszuxpinyin_passwd->lineEdit_window->setText(data);
    syszuxpinyin_passwd->resize(800,310);
    syszuxpinyin_passwd->move(120,300);
    syszuxpinyin_passwd->show();

}

//当拼音输入完成时，将最终的字符串设置到密码输入框中。
void  Login::confirmString_passwdSlot(QString gemfield)
{
   password_edit->setText(gemfield);
}

/*将拼音输入框的内容设置为 data。
 调整拼音输入框的大小为 800x310。
 将拼音输入框移动到 (120, 300) 位置。
 显示拼音输入框。
 */
void Login::keyboardshow_usrnameSlot(QString data)
{

    syszuxpinyin_usrname->lineEdit_window->setText(data);
    syszuxpinyin_usrname->resize(800,310);
    syszuxpinyin_usrname->move(120,300);
    syszuxpinyin_usrname->show();

}
//当拼音输入完成时，将最终的字符串设置到用户名输入框中。
void Login::confirmString_usrnameSlot(QString gemfield)
{
   usr_edit->setText(gemfield);
}

//当用户点击登录按钮时，调用此槽函数。
void Login::pushbutton_loginSlot()
{
    bool ret =openDb();//尝试打开数据库并验证用户输入。
    if(ret == true)
        emit loginsuccess();
    else
        emit loginfailed();
}

void Login::createTable()
{
    //创建一个数据库操作对象
    QSqlQuery sq;
    QString sql = "CREATE TABLE IF NOT EXISTS usr(usrname TEXT PRIMARY KEY,password TEXT NOT NULL);";
    if(sq.exec(sql))
        qDebug() << "CREATE TABLE success";
    //else//如果表已存在也会走这个分支
        //qDebug() << sq.lastError().text();
}
void Login::insertInfo()
{
    QSqlQuery sq;
    QString sql = "INSERT INTO usr VALUES('usr','123');";
    if(sq.exec(sql))
        qDebug() << "INSERT INTO success";
    else
        qDebug() << sq.lastError().text();
}
bool Login::openDb()
{
    /* 下面表的数据都会创建在程序的当前目录/root */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("usr.db");
    if(!db.open())
    {
        QMessageBox::warning(0, tr("Warning"), db.lastError().text());
        return false;
    }
//    建表
    createTable();//创建用户表（如果表不存在）。
//    插入
    insertInfo();//插入默认用户数据（如果需要）。

    /* 查询usr表，验证用户输入的用户名和密码是否匹配。 */
    QSqlQuery query(db);
    if(!query.exec("select usrname,password from usr"))
    {
        db.close();
        return false;
    }
    while(query.next())//如果表中有sql数据则尝试逐条匹配数据
    {
        QString UserName =   query.value(0).toString();
        QString Password =   query.value(1).toString();
        qDebug()<< UserName;
        qDebug()<< Password;
        //若输入的英文账号密码存在于数据库返回true
        if(UserName == usr_edit->text() &&Password ==password_edit->text())
            return true;
    }
    db.close();
    return false;
}
void Login::pushbutton_exitSlot()
{
    emit loginclose();//发送loginclose信号给mainwindow
    close();//关闭login widget
    //system("echo 1 > /sys/class/graphics/fb0/blank");//关闭屏幕
}
