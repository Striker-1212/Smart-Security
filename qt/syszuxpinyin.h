/***************************************************************************
**
**  <SYSZUXpinyin 1.0 , a chinese input method based on Qt for Embedded linux>
**  Copyright (C) <2010> <Gemfield> <gemfield@civilnet.cn>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License version 3 as published
**  by the Free Software Foundation.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
**  If you have questions regarding the use of this file, please contact
**  Gemfield at gemfield@civilnet.cn or post your questions at
**  http://civilnet.cn/syszux/bbs
**
****************************************************************************/

#ifndef SYSZUXPINYIN_H
#define SYSZUXPINYIN_H
#include <QFile>
#include "ui_syszuxpinyin.h"

//Ui::Dialog是通过 Qt Designer 生成的 UI 类，用于定义对话框的界面布局。
class SyszuxPinyin : public QDialog ,public Ui::Dialog
{
    Q_OBJECT
public:
    SyszuxPinyin();               //
    ~SyszuxPinyin();
    void changePage(int index);   //切换页面
    void matching(QString gemfield);//匹配输入的拼音。
    void initGb();              //初始化 GB 编码。
    void selectHanziPre();      //选择上一个汉字。
    void selectHanziNext();     //选择下一个汉字。
    void changeInputMethod();   //切换输入法模式。
    void changeLowerUpper();    //切换大小写。
    void clearString();         //清空输入内容。
    void affirmString();        //确认输入内容。
    void deleteString();        //删除输入内容。
public slots:
    void buttonClickResponse(int gemfield);//按钮点击响应函数，处理按钮点击事件。
signals:
    void sendPinyin(QString gemfield);//发送拼音信号，携带输入的拼音字符串。
private:
    QFile pinyin_file;                              //文件类
    QKeyEvent *event;                               //键盘事件对象
    QRegExp regExp;                                 //正则表达式
    QButtonGroup *button_group;                     //按钮组,在构造函数的初始化变量中初始化好了,将所有按键加入进按钮组，只需要连接一次按钮组的信号槽.
    QMultiMap<QString,QString> pinyin_map;          //mmap，插入键值对, 多值映射表
    QList<QString> pinyin_list;                     //拼音列表。(QString向量数组),从字库中取出的汉字部分
    QVector<QPushButton*> button_vector;            //按钮向量(QPushButton*数组),存储虚拟键盘中所有按键的指针。它提供了一种方便的方式来动态管理按键的集合
    int input_method,lower_upper,page_count;        //输入法模式、大小写状态、页码计数,在构造函数的初始化变量中初始化好了
};

#endif

