#ifndef SMTP_H
#define SMTP_H
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")


#include <iostream>
#include <list>

using namespace std;
const int MAX_EMAIL_MESSAGE_LEN = 1024;
const int MAX_FILE_LEN = 6000;
static const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

struct FILEINFO
{
    char filename[128];
    char filePath[256];
};

class Csmtp
{

    //对外提供的接口
public:
    Csmtp(void);
    Csmtp(int port,     //端口号
          string srvDomin,  //smtp服务器
          string userName,  //用户名
          string password,  //密码--授权码
          string targetEmail,   //发送邮件
          string emailTitle,    //主题
          string content);      //内容

public:
    ~Csmtp(void);

public:
    int SendEmail_Ex();      //发送流程
public:
    void SetSrvDomain(string& strDomain) { this->domain = strDomain; }
    void SetUserName(string& strUser) { this->user = strUser; }
    void SetPass(string& strPass) { this->pass = strPass; }
    void SetTargetEmail(string& strTargetAddr) { this->targetAddr = strTargetAddr; }
    void SetEmailTitle(string& strTitle) { this->title = strTitle; }
    void SetContent(string& strContent) { this->content = strContent; }
    void SetPort(int nPort) { this->port = nPort; }

public:
    void AddAttachment(string &filePath );      //添加文件信息
    void DeleteAttachment(string &filePath);    //删除文件信息
    void DeleteAllAttachment();                 //删除所有附件



    //private函数
private:
    int port;
    string domain;
    string user;
    string pass;
    string targetAddr;
    string title;
    string content;

    list<FILEINFO *> listFile;

private:
    char buff[MAX_EMAIL_MESSAGE_LEN+1];
    int buffLen;
    SOCKET socketClient;

private:
    bool CreateConn();                  //建立连接
    bool Send(string& strMessage);      //发送消息
    bool Recv();                        //接收消息

private:
    void FormatEmailHead(string& strEmail);     //格式化邮件头部信息
    int Login();    //邮件登录
    bool SendEmailHead();   //发送邮件头部信息
    bool SendTextBody();    //发送邮件内容
    bool SendEnd();         //发送邮件结束信息



private:
    char* base64Encode(char const* pOrigsigned,unsigned origlen);       //base64Encode


private:
     int SendAttachment_ex();                    //发送文件过程
};
#endif // SMTP_H


