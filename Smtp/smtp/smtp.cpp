#include "smtp.h"
#include <fstream>
#include <string.h>

Csmtp::Csmtp(){
    this->port = 25;
    this->domain = "";
    this->user = "";
    this->pass = "";
    this->targetAddr = "";
    this->title = "";
    this->content = "";
    this->socketClient = 0;
};
Csmtp::Csmtp(
        int port,
        string srvDomin,
        string userName,
        string password,
        string targetEmail,
        string emailTitle,
        string content)
{
    this->port = port;
    this->domain = srvDomin;
    this->user = userName;
    this->pass = password;
    this->targetAddr = targetEmail;
    this->title = emailTitle;
    this->content = content;
    this->socketClient = 0;
}

Csmtp::~Csmtp()
{
    closesocket(socketClient);
}

bool Csmtp::CreateConn(){

    WORD wVersionRequested;
    WSADATA wsaData;
    int ret;

    //WinSock初始化
    wVersionRequested = MAKEWORD(2, 2); //希望使用的WinSock DLL的版本
    ret = WSAStartup(wVersionRequested, &wsaData);


    SOCKET skClientTemp = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in saddr;
    hostent* pHostent;
    pHostent = gethostbyname(domain.c_str());

    saddr.sin_port = htons((u_short)port);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.S_un.S_addr = *((unsigned long*)pHostent->h_addr_list[0]);

    int err = connect(skClientTemp,(sockaddr*)&saddr,sizeof(saddr));
    if(err != 0)
        return false;
    this->socketClient = skClientTemp;
    if(false == Recv())
        return false;
    return true;
}

bool Csmtp::Send(string &strMessage)
{
    int err = (int)send(socketClient,strMessage.c_str(),(int)strMessage.length(),0);
    if(err < 0)
        return false;
    cout<<strMessage.c_str()<<endl;
    return true;
}

bool Csmtp::Recv()
{
    memset(buff,0,sizeof(char)*(MAX_EMAIL_MESSAGE_LEN+1));
    int err = recv(socketClient,buff,MAX_EMAIL_MESSAGE_LEN,0);
    if(err < 0)
        return false;
    buff[err] = '\0';
    cout<<buff<<endl;
    return true;
}

void Csmtp::FormatEmailHead(string &strEmail)
{
    //加入发送方
    strEmail = "From: ";
    strEmail += user;
    strEmail += "\r\n";

    //加入接收方
    strEmail += "To: ";
    strEmail += targetAddr;
    strEmail += "\r\n";

    //加入主题
    strEmail += "Subject: ";
    strEmail += title;
    strEmail += "\r\n";

    //加入版本信息
    strEmail += "MIME-Version: 1.0";
    strEmail += "\r\n";

    //文本类型
    strEmail += "Content-Type: multipart/mixed;boundary=qwertyuiop";
    strEmail += "\r\n";
    strEmail += "\r\n";
}

int Csmtp::Login()
{
    string sendBuff;

    //辨明发件人信息
    sendBuff = "HELO ";
    sendBuff += user;
    sendBuff += "\r\n";
    if(false == Send(sendBuff) || false == Recv())      //如果发送失败
        return 1;

    //请求认证
    sendBuff = "AUTH LOGIN \r\n" ;
    if(false == Send(sendBuff)|| false == Recv())
        return 1;

    sendBuff = "";
    int pos = (int)user.find('@',0);
    sendBuff = user.substr(0,pos);

    //密码
    char* ecode;
    ecode = base64Encode(sendBuff.c_str(),(unsigned int)strlen(sendBuff.c_str()));
    sendBuff = ecode;
    sendBuff += "\r\n";
    delete[] ecode;

    if (false == Send(sendBuff) || false == Recv())
        return 1;

    sendBuff.empty();
    ecode = base64Encode(pass.c_str(), (unsigned int)strlen(pass.c_str()));
    sendBuff = ecode;
    sendBuff += "\r\n";
    delete[]ecode;

    if (false == Send(sendBuff) || false == Recv())
        return 1;

    if (NULL != strstr(buff, "550"))
        return 2;

    if (NULL != strstr(buff, "535"))
        return 3;
    return 0;
}

bool Csmtp::SendEmailHead()
{
    //文件头格式

    string sendBuff = "";
    sendBuff = "MAIL FROM: <"+user +">\r\n";
    if(false == Send(sendBuff) || false == Recv())
        return false;

    sendBuff = "";
    sendBuff = "RCPT TO: <" + targetAddr + ">\r\n";
    if(false == Send(sendBuff) || false == Recv())
        return false;

    sendBuff = "";
    sendBuff = "DATA\r\n";
    if(false == Send(sendBuff) || false == Recv())
        return false;

    //sendBuff = "";
    FormatEmailHead(sendBuff);
    if(false == Send(sendBuff))
        return false;

    return true;
}

bool Csmtp::SendTextBody()
{
    string sendBuff;
    sendBuff = "--qwertyuiop\r\n";
    sendBuff += "Content-Type: text/plain;";
    sendBuff += "charset=\"utf-8\"\r\n\r\n";
    sendBuff += content;
    sendBuff += "\r\n\r\n";
    return Send(sendBuff);
}

bool Csmtp::SendEnd()
{

    //发送结束信息
    string sendBuff;
    sendBuff = "--qwertyuiop--";
    sendBuff += "\r\n.\r\n";
    if(false == Send(sendBuff) || false == Recv())
        return false;
    cout<<buff<<endl;

    sendBuff = "QUIT\r\n";
    bool ret = Send(sendBuff)&&Recv();
    return ret;
}

int Csmtp::SendEmail_Ex()
{
    if (false == CreateConn())
    {
        return 1;
    }
    //Recv();
    int err = Login(); //先登录
    if (err != 0)
    {
        return err; //错误代码必须要返回
    }
    if (false == SendEmailHead()) //发送EMAIL头部信息
    {
        return 1; // 错误码1是由于网络的错误
    }
    if(false == SendTextBody())
        return 1;
    err = SendAttachment_ex();
    if(err != 0)
        return err;
    if(false == SendEnd())
        return 1;
    return 0;
}


char* Csmtp::base64Encode(char const* origSigned, unsigned origLength)
{
    unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
    if (orig == NULL) return NULL;

    unsigned const numOrig24BitValues = origLength / 3;
    bool havePadding = origLength > numOrig24BitValues * 3;
    bool havePadding2 = origLength == numOrig24BitValues * 3 + 2;
    unsigned const numResultBytes = 4 * (numOrig24BitValues + havePadding);
    char* result = new char[numResultBytes + 3]; // allow for trailing '/0'

    unsigned i;
    for (i = 0; i < numOrig24BitValues; ++i)
    {
        result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
        result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
        result[4 * i + 2] = base64Char[((orig[3 * i + 1] << 2) | (orig[3 * i + 2] >> 6)) & 0x3F];
        result[4 * i + 3] = base64Char[orig[3 * i + 2] & 0x3F];
    }

    if (havePadding)
    {
        result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
        if (havePadding2)
        {
            result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
            result[4 * i + 2] = base64Char[(orig[3 * i + 1] << 2) & 0x3F];
        }
        else
        {
            result[4 * i + 1] = base64Char[((orig[3 * i] & 0x3) << 4) & 0x3F];
            result[4 * i + 2] = '=';
        }
        result[4 * i + 3] = '=';
    }

    result[numResultBytes] = '\0';
    return result;
}

void Csmtp::AddAttachment(std::string &filePath) //添加附件
{
    FILEINFO *pFile = new FILEINFO;
    strcpy(pFile->filePath, filePath.c_str());
    const char *p = filePath.c_str();
    strcpy(pFile->filename, p + filePath.find_last_of("\\") + 1);
    listFile.push_back(pFile);
}
void Csmtp::DeleteAttachment(std::string &filePath) //删除附件
{
    std::list<FILEINFO *>::iterator pIter;
    for (pIter = listFile.begin(); pIter != listFile.end(); pIter++)
    {
        if (strcmp((*pIter)->filePath, filePath.c_str()) == 0)
        {
            FILEINFO *p = *pIter;
            listFile.remove(*pIter);
            delete p;
            break;
        }
    }
}
void Csmtp::DeleteAllAttachment() /*删除所有的文件*/
{
    for (std::list<FILEINFO *>::iterator pIter = listFile.begin(); pIter != listFile.end();)
    {
        FILEINFO *p = *pIter;
        pIter = listFile.erase(pIter);
        delete p;
    }
}


 int Csmtp::SendAttachment_ex() // 发送附件
{
    for (std::list<FILEINFO *>::iterator pIter = listFile.begin(); pIter != listFile.end(); pIter++)
    {
        //cout << "Attachment is sending ~~~~~" << endl;
        //cout << "Please be patient!" << endl;
        string sendBuff;
        sendBuff = "--qwertyuiop\r\n";
        sendBuff += "Content-Type: application/octet-stream;\r\n";
        sendBuff += " name=\"";
        sendBuff += (*pIter)->filename;
        sendBuff += "\"";
        sendBuff += "\r\n";
        sendBuff += "Content-Transfer-Encoding: base64\r\n";
        sendBuff += "Content-Disposition: attachment;\r\n";

        sendBuff += " filename=\"";
        sendBuff += (*pIter)->filename;
        sendBuff += "\"";

        sendBuff += "\r\n";
        sendBuff += "\r\n";
        Send(sendBuff);
        ifstream ifs((*pIter)->filePath, std::ios::in | std::ios::binary);
        if(false == ifs.is_open())
        {
            return 4;;
        }
        char fileBuff[MAX_FILE_LEN];
        char *chSendBuff;
        memset(fileBuff, 0, sizeof(char)*(MAX_FILE_LEN));


        //文件使用base64加密传送
        while (ifs.read(fileBuff, MAX_FILE_LEN))
        {
            //cout << ifs.gcount() << endl;
            chSendBuff = base64Encode(fileBuff, MAX_FILE_LEN);
            chSendBuff[strlen(chSendBuff)] = '\r';
            chSendBuff[strlen(chSendBuff)] = '\n';
            send(socketClient, chSendBuff, strlen(chSendBuff), 0);
            delete[]chSendBuff;
        }
        //cout << ifs.gcount() << endl;
        chSendBuff = base64Encode(fileBuff, ifs.gcount());
        chSendBuff[strlen(chSendBuff)] = '\r';
        chSendBuff[strlen(chSendBuff)] = '\n';
        int err = send(socketClient, chSendBuff, strlen(chSendBuff), 0);
        if (err != (int)strlen(chSendBuff))
        {
            //cout << "文件传送出错!" << endl;
            return 1;
        }
        delete[]chSendBuff;
    }
    return 0;
 }
