#ifndef MYSOCKET_COMPILE
#define MYSOCKET_COMPILE

#define BUF_END_MARKER 4
#define FILE_RW_BUFFER 1024
#define RECV_BUFFER 2048
/*
 * 套接字信息结构
 */
//struct sockaddr{
//	unsigned short sa_family;	// 地址族，AF_xxx
//	char sa_data[14];			// 14B得协议地址
//}
/*
 * 另一种结构
 */
//struct sockaddr_in{
//	short int sin_family;			//	地址族
//	unsigned short int sin_port;	//	端口号
//	struct in_addr sin_addr;		//	IP地址
//	unsigned char sin_zero[8];		//	填充0以保持与struct sockaddr同样得大小
//};

/**定义数据结构**/
typedef enum MsgType { //枚举消息类型
	TYPE_userInfo,
	TYPE_userNotExists,
	TYPE_pwError,
	TYPE_get,
	TYPE_put,
	TYPE_ls,
	TYPE_pwd,
	TYPE_cd,
	TYPE_mkdir,
	TYPE_rmdir,
	TYPE_ascii,
	TYPE_bin,
	TYPE_quit,
	SERVER_checkOnlineUser
} MsgType;

typedef struct { //消息头
	MsgType _msgType;
	uint32_t _bytes;
} MsgHead;

typedef struct { //消息体
	MsgHead _msgHead;
	void* _data;
} MsgData;

typedef struct {
	char userName[256];
	char passWord[256];
} UserInfo;

typedef struct {
	int m_nSocket;
	struct sockaddr_in* m_tClientAddress;
	UserInfo* userInfo;
} ClientInfo;

typedef struct{
	ClientInfo client;
	struct UserNode* next; //加struct用作定义前的声明
} UserNode;

typedef struct{
	int count_current;
	int count_all;
	UserNode* ul_head;
} UserMnag;


/**公共**/
//建立套接字
int getSocket();
//关闭连接
int closeConnect(int sockfd);
//发送单位数据
int FTP_Send(int sockfd, const void* pBuffer, int nSize);
//发送数据
int FTP_SendBlockAll(int sockfd, const void *pBuffer, int Size);
//发送消息头
int FTP_SentHeader(int sockfd, const MsgHead* pHead);
//发送消息体
int FTP_SendMessage(int sockfd, const MsgData* pMsg);
//接收元数据
int FTP_Receive(int sockfd, void *pBuffer, int nSize);
//阻塞式接收数据
int FTP_RecvBlockAll(int sockfd, void* pBuffer, int nSize);
//接收消息体
int FTP_RecvMessage(int sockfd, MsgData* pMsg);
//读写文件
int rwDataTxt(char* fileName,char* model,char* writeString,char* outString);
//增加用户列表中的用户
int l_addUser(UserNode* ul_head,ClientInfo* user);
//显示用户列表中的用户
int l_showUser(UserNode* ul_head);
//释放用户列表资源
int l_release(UserNode* ul_head);

/**服务器端**/
//从参数获得ip及端口号和最大连接数
int getServerArgs(char** args, char** _ip, int* _port, int* _client_max);
//绑定地址和端口(参数：my_addr本机地址及端口结构体,addrlen结构体大小 返回值：0成功，-1失败，errno置为相应错误号)
int bindIpAndPort(int sockfd, const struct sockaddr* my_addr, int addrlen);
//建立套接字队列(参数：backlog请求队列最大请求数 返回值：0成功，-1失败，errno置为相应错误号)
int ListenAndCreatQueue(int sockfd, int backlog);
//等待客户请求到达(参数:addr指向存放客户机信息得sockaddr_in变量得指针，addrlen即sockaddr_in大小 返回值：0成功，-1失败，errno置为相应错误号)
int acceptForCLient(int sockfd, void *addr, socklen_t *addrlen); //socklen_t 为 unsigned int
//启动服务器
int FTP_RunServer(char* ip, int port, int client_max);
//接入命令线程
int FTP_CommandThread(int sockfd);
//服务端执行命令函数
int FTP_ServerCommand(void* pParam);
//接入控制线程
int FTP_ServerThread(int sockfd);
//创建客户会话线程
int FTP_CreateNewClientThread(int nSocket, struct sockaddr_in* tClientAddress,UserInfo* userInfo);
//客户会话线程
int* FTP_ClientThread(void* pParam);
//////////站点统计与用户管理函数


/**客户端**/
//于服务器建立TCP连接(参数:serv_addr说明服务器套接字地址结构的地址，addrlen即对方套接字大小 返回值：0成功，-1失败，errno置为相应错误号)
int connectToServer(int sockfd, struct sockaddr_in* serv_addr, int addrlen);

#endif
