#include "my_lineedit.h"
#include <QDebug>

My_lineEdit::My_lineEdit(QWidget *parent) :
    QLineEdit(parent)
{

}

/*
mousePressEvent 是 Qt 框架中用于处理鼠标点击事件的虚拟函数。
它属于 QWidget 类及其派生类，例如 QLineEdit、QPushButton 等。
通过重写这个函数，可以自定义鼠标点击事件的处理逻辑。
这个重写函数是实现点击用户名或密码输入框时，将原本的文本传入到虚拟键盘上的lineEdit_window框内
*/
void My_lineEdit::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);//标记 event 参数在函数中未被使用。这可以避免编译器警告，同时表明该参数是故意未使用的。
    emit send_show(this->text());//会在打开的键盘的lineEdit_window框中展示之前已经编辑好的内容，如已经输入过的用户名和密码

}
