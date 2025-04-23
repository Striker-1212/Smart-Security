#ifndef REGISTER_H
#define REGISTER_H

#include <QDialog>
#include <QKeyEvent>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QDebug>
#include "syszuxpinyin.h"
#include "my_lineedit.h"
namespace Ui {
class Register;
}

class Register : public QDialog
{
    Q_OBJECT

public:
    //explicit禁止隐式类型转换。
    explicit Register(QWidget *parent = nullptr);
    ~Register();
    /* 键盘输入对话框 */
    SyszuxPinyin *syszuxpinyin_usrname;
    SyszuxPinyin *syszuxpinyin_passwd;
private slots:
    void  pushbutton_registerSlot();
    void  pushbutton_exitSlot();
    void  keyboardshow_usrnameSlot(QString data);
    void  keyboardshow_passwdSlot(QString data);
    void confirmString_usrnameSlot(QString gemfield);   //接收键盘发过来的数据
    void confirmString_passwdSlot(QString gemfield);   //接收键盘发过来的数据

private:
    Ui::Register *ui;
    /* 注册时需要操作的两个输入框 */
    My_lineEdit *password_edit;
    My_lineEdit *usr_edit;
    bool insertusr();
};

#endif // REGISTER_H
