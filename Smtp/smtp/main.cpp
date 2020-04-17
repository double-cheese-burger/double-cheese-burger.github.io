#include "mainwindow.h"
#include <QApplication>
#include "smtp.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Csmtp smtp(
        25,									// smtp端口
        "smtp.126.com",						// smtp服务器地址
        "wonghulin@126.com",				        // 你的邮箱地址
        "JJAETKUJQYWUEHLL",					        // 邮箱密码
        "861617573@qq.com",					    // 目的邮箱地址
        "title",							// 主题
        "要发送的内容"		                // 邮件正文
    );
    string filePath("C:\\Users\\86161\\Desktop\\ttt\\main.cpp");
    smtp.AddAttachment(filePath);
    int err;
    if ((err = smtp.SendEmail_Ex()) != 0)
    {
        if (err == 1)
            cout << "错误1: 由于网络不畅通，发送失败!" << endl;
        if (err == 2)
            cout << "错误2: 用户名错误,请核对!" << endl;
        if (err == 3)
            cout << "错误3: 用户密码错误，请核对!" << endl;
        if (err == 4)
            cout << "错误4: 请检查附件目录是否正确，以及文件是否存在!" << endl;
        return a.exec();
    }
    cout<<"success"<<endl;
    return a.exec();
}
