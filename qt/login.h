#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QKeyEvent>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QDebug>
#include "syszuxpinyin.h"
#include "my_lineedit.h"
//#include "HumanDetect.h"
namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();
    /* 两个键盘对象 */
    SyszuxPinyin *syszuxpinyin_usrname;
    SyszuxPinyin *syszuxpinyin_passwd;

private slots:
    void  pushbutton_loginSlot();

    void  keyboardshow_usrnameSlot(QString data);
    void  keyboardshow_passwdSlot(QString data);
    void confirmString_usrnameSlot(QString gemfield);   //接收键盘发过来的数据
    void confirmString_passwdSlot(QString gemfield);   //接收键盘发过来的数据
    void pushbutton_exitSlot();
private:
    /* 两个输入框对象 */
    My_lineEdit *password_edit;
    My_lineEdit *usr_edit;
    bool openDb();
    void createTable();
    void insertInfo();
    Ui::Login *ui;
signals:
    void loginsuccess();
    void loginfailed();
    void loginclose();
};

#endif // LOGIN_H
