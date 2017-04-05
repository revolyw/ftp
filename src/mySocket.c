#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <shadow.h>
#include "myCommand.h"
#include "myThread.h"
#include "mySocket.h"

UserMnag userMnag;
/**公共**/
//建立套接字
int getSocket() {
	//socket(domain,type,protocol)
	//domain:所使用的协议族（常用AF_INET表示互联网协议族，即TCP/IP协议族）
	//type:套接字类型（SOCKET_STREAM为流式套接字TCP）
	//protocol:特定协议，若不使用则置为0,表示使用默认的连接模式
	//*return*:套接字描述符（int）
	int socketId = socket(AF_INET, SOCK_STREAM, 0);
	return socketId;
}
//关闭连接
int closeConnect(int sockfd) {
	return close(sockfd);
}
//发送单位数据
int FTP_Send(int sockfd, const void* pBuffer, int nSize) {
	return (int) send(sockfd, pBuffer, nSize, 0); //返回值为所发送数据的总数，否则返回SOCKET_ERROR。
}
//发送数据
int FTP_SendBlockAll(int sockfd, const void *pBuffer, int nSize) {
	int nSent = 0;
	int nSentBytes;
	for (;;) {
		nSentBytes = FTP_Send(sockfd, pBuffer + nSent, nSize - nSent);
		if (nSentBytes <= 0)
			return -1;
		nSent += nSentBytes;
		if (nSent == nSize)
			return 0;
	}
	return 0;
}
//发送消息头
int FTP_SendHeader(int sockfd, const MsgHead* pHead) {
	uint32_t bufHead[2];
	bufHead[0] = htons(pHead->_msgType);
	bufHead[1] = htons(pHead->_bytes);
	if (FTP_SendBlockAll(sockfd, bufHead, sizeof(uint32_t) * 2) == -1) {
		printf("Connection lost at sending packet header:FTP_SendMessge\n");
		return -1;
	}
	return 0;
}
//发送消息体
int FTP_SendMessage(int sockfd, const MsgData* pMsg) {
	uint32_t bufEndMarker = htons(BUF_END_MARKER);
	if (!pMsg || sockfd == -1 || (pMsg->_msgHead._bytes != 0 && !pMsg->_data)) {
		return -1;
	}

	if (FTP_SendHeader(sockfd, &pMsg->_msgHead) == -1)
		return -1;
	if (pMsg->_msgHead._bytes != 0) {
		if (FTP_SendBlockAll(sockfd, pMsg->_data, pMsg->_msgHead._bytes)
				== -1) {
			printf(
					"Connection Lost at sending packet message:FTP_SendMessage(%d)\n",
					sockfd);
			return -1;
		}
	}
	if (FTP_SendBlockAll(sockfd, &bufEndMarker, sizeof(uint32_t)) == -1) {
		printf("Connection Lost at sending endmarker:FTP_SendMessage(%d)\n",
				sockfd);
		return -1;
	}
	return 0;
}
//接收元数据
int FTP_Receive(int sockfd, void *pBuffer, int nSize) {
	return (int) recv(sockfd, pBuffer, nSize, 0);
}
//阻塞式接收数据
int FTP_RecvBlockAll(int sockfd, void* pBuffer, int nSize) {
	int nReceived = 0;
	int nGotBytes;
	for (;;) {
		nGotBytes = FTP_Receive(sockfd, pBuffer + nReceived, nSize - nReceived);
		if (nGotBytes <= 0)
			return -1;
		nReceived += nGotBytes;
		if (nReceived == nSize)
			return 0;
	}
	return 0;
}
//接收消息体
int FTP_RecvMessage(int sockfd, MsgData* pMsg) {
	uint32_t bufHead[2];
	uint32_t bufEndMarker;
	if (!pMsg || sockfd == -1) {
		return -1;
	}
	if (pMsg->_data != NULL) {
		free(pMsg->_data);
	}
	pMsg->_data = NULL; //清空
	if (FTP_RecvBlockAll(sockfd, bufHead, 2 * sizeof(uint32_t)) == -1) { //接受消息头
		printf(
				"Connection Lost at receive packet header: FTP_RecvMessage(%d)\n",
				sockfd);
		return -1;
	}

	pMsg->_msgHead._msgType = ntohs(bufHead[0]);
	pMsg->_msgHead._bytes = ntohs(bufHead[1]);

	if (pMsg->_msgHead._bytes == 0) {
		pMsg->_data = NULL;
	} else {
		pMsg->_data = malloc(pMsg->_msgHead._bytes); //500 and client-->release

		if (!pMsg->_data) {
			printf(
					"Cannot malloc %dbytes for getting data at FTP_RecvMessage(%d)\n",
					pMsg->_msgHead._bytes, sockfd);
			return -1;
		}
		if (FTP_RecvBlockAll(sockfd, pMsg->_data, pMsg->_msgHead._bytes)
				== -1) {
			printf("Connection Lost at receiving bytes:FTP_RecvMessage(%d)\n",
					sockfd);
			free(pMsg->_data);
			return -1;
		}
		//printf("malloc %d truely %d \n",pMsg->_msgHead._bytes,strlen(pMsg->_data));
	}

	if (FTP_RecvBlockAll(sockfd, &bufEndMarker, sizeof(uint32_t)) == -1) {
		printf("Connection Lost at receiving end marker:FTP_RecvMessage(%d)\n",
				sockfd);
		free(pMsg->_data);
		return -1;
	}
	bufEndMarker = ntohs(bufEndMarker);
	if (bufEndMarker != BUF_END_MARKER) {
		printf("Received non-EndMarker %x:FTP_RecvMessage(%d)\n", bufEndMarker,
				sockfd);
		free(pMsg->_data);
		return -1;
	}
	return 0;
}
//读写文件
int rwDataTxt(char* fileName, char* model, char* writeString, char* outString) {

	char* result = (char*) malloc(FILE_RW_BUFFER); //179 release
	FILE* fp = fopen(fileName, model); //180 release
	if (fp == NULL)
		return -1;

	if (strcmp(model, "r") == 0) {
		while (!feof(fp)) {
			result = fgets(result, 100, fp);
			if (result != NULL) {
				strcpy(outString, result);
			}
		}
	} else if (strcmp(model, "w") == 0) {
		fputs(writeString, fp);
	}

	free(result);
	fclose(fp);
	return 0;
}
//增加用户列表中的用户
int l_addUser(UserNode* ul_head, ClientInfo* user) {

	ClientInfo* tmp = (ClientInfo*) malloc(sizeof(ClientInfo));
	tmp->m_nSocket = user->m_nSocket;
//	tmp->m_tClientAddress;
	if (ul_head == NULL) {
//		ul_head->next = ;
		return 0;
	}
	return 0;
}
//显示用户列表中的用户
int l_showUser(UserNode* ul_head) {
	return 0;
}
//释放用户列表资源
int l_release(UserNode* ul_head) {
	return 0;
}

/**服务器端**/
//从参数获得ip及端口号和最大连接数
int getServerArgs(char** args, char** _ip, int* _port, int* _client_max) {
	char* argString = args[1];
	char c = 'a';
	int argLength = strlen(argString), i = 0, j = 0, k = 0, y = 0;
	char* buf[3];
	argString[argLength] = '\0';
	for (i = 0; i < 3; i++) {
		while ((c = argString[j++]) != '\0') {
			if (c != ':' && c != '\0')
				y++;
			else {
				break;
			}
		}
		buf[i] = (char*) malloc(y + 1);
	}
	i = j = k = y = 0;
	while ((c = argString[i]) != '\0') {
		if (c != ':' && c != '\0') {
			buf[j][k++] = c;
			buf[j][k] = '\0';
		} else {
			if (j == 3)
				return -1; //参数有误
			buf[j++][k] = '\0';
			k = 0;
		}
		i++;
	}
	*_ip = buf[0]; //在主函数中释放
	*_port = atoi(buf[1]); //atoi()字符串转整形数
	*_client_max = atoi(buf[2]);
	free(buf[1]);
	free(buf[2]);

//	printf("%s,%d,%d\n", *_ip, *_port, *_client_max);
//	printf("\nThe main arg : %s\n", argString);
	return 0;
}
//绑定地址和端口
int bindIpAndPort(int sockfd, const struct sockaddr* my_addr, int addrlen) {
	return bind(sockfd, my_addr, addrlen);
}
//建立套接字队列
int ListenAndCreatQueue(int sockfd, int backlog) {
	return listen(sockfd, backlog);
}
//等待客户请求到达
int acceptForCLient(int sockfd, void *addr, socklen_t *addrlen) {
	return accept(sockfd, (struct sockaddr*) addr, addrlen);
}
//启动服务器
int FTP_RunServer(char* ip, int port, int client_max) {
	struct sockaddr_in serv_addr; //服务端ip地址及端口信息
	int sockfd = -1; //socket描述符

	serv_addr.sin_family = AF_INET;
	inet_aton(ip, &serv_addr.sin_addr); //将一个字符串IP地址转换为一个32位的网络序列IP地址并放入一个结构中。
	serv_addr.sin_port = htons(port);

	printf("\nserver initializing......\n");

	sockfd = getSocket();
	if (sockfd == -1) {
		printf("server getting socket error!!\n");
		return -1;
	}

	if (bindIpAndPort(sockfd, (const struct sockaddr*) &serv_addr,
			sizeof(struct sockaddr_in)) == -1) {
		printf("server binding socket error!!\n");
		return -1;
	}

	if (ListenAndCreatQueue(sockfd, client_max) == -1) {
		printf("server listening error!!\n");
		return -1;
	}

	char * ip_string = inet_ntoa(serv_addr.sin_addr); //将一个IP转换成一个互联网标准点分格式的字符串。
	printf("server is running, ip: %s, port: %d\n", ip_string, port);

	//读取站点统计数据
	char * read = (char*) malloc(FILE_RW_BUFFER); //280 release
	int startPos = 0;

	if (rwDataTxt("data/data.txt", "r", NULL, read) == -1)
		printf("open file error!!\n");

	while (*(read + startPos) != '=') {
		startPos++;
	}
	startPos++;
	userMnag.count_all = atoi(read + startPos);
	free(read);

	return sockfd;
}
//接入命令线程
int FTP_CommandThread(int sockfd) {
	tThread serv_CommandThread;
	StartThread(&serv_CommandThread, (void*) (&FTP_ServerCommand), &sockfd);
	return 0;
}
//服务端执行命令函数
int FTP_ServerCommand(void * pParam) {
	int sockfd = *((int *) pParam);
	char cmd[RECV_BUFFER];

	while (1) {
		printf("\n");
		printf("server ftp->");

		fgets(cmd, RECV_BUFFER, stdin);
		cmd[strlen(cmd) - 1] = '\0';

		if (!strcmp(cmd, "")) {
			printf("please input a command!!\n");
			continue;
		} else if (!strcmp(cmd, "count current")) {
			printf("当前在线用户数：%d \n", userMnag.count_current);
			continue;
		} else if (!strcmp(cmd, "count all")) {
			printf("访问用户总数：%d \n", userMnag.count_all);
			continue;
		} else if (!strcmp(cmd, "list")) {
			printf("在线用户列表：\n ...");
			continue;
		} else if (!strncmp(cmd, "kill", 4)) {
			printf("will kill %s!! \n", cmd + 5);
			continue;
		} else if (!strcmp(cmd, "quit")) {
			if (closeConnect(sockfd) == -1) {
				printf("socket Recovery failed ! please try again !\n");
				continue;
			}
			char* part1 = (char*) malloc(FILE_RW_BUFFER);
			char part2[256];

			strcpy(part1, "count all=");
			sprintf(part2, "%d", userMnag.count_all); //整转串
			strcat(part1, part2);

			if (rwDataTxt("data/data.txt", "w", part1, NULL) == -1)
				printf("open file error!!\n");

			free(part1);

			printf("quit success !\n\n");
			exit(0);
		} else {
			printf("The command %s is not found !! \n", cmd);
			continue;
		}
	}
	return 0;
}

void get_salt(char *salt, char *passwd) { //取得加密密钥
	int i, j;

	//取出salt,i记录密码字符下标,j记录$出现次数
	for (i = 0, j = 0; passwd[i] && j != 3; ++i) {
		if (passwd[i] == '$')
			++j;
	}

	strncpy(salt, passwd, i - 1);
}
//接入控制线程
int FTP_ServerThread(int sockfd) {
//	static int Counter = 0;
//	tMutex g_tCounterMutex;
//	InitializeMutex(&g_tCounterMutex);
//	LockMutex(&g_tCounterMutex);
//	Counter++;
//	UnlockMutex(&g_tCounterMutex);
	printf("server launched. waiting for users...\n");
	for (;;) {
		ClientInfo tClient;
		struct sockaddr_in tClientAddress;
		socklen_t cli_addr_size = sizeof(tClientAddress);

		int nClientSocket = acceptForCLient(sockfd, &tClientAddress,
				&cli_addr_size);

		if (nClientSocket == -1) {
			printf("cannot accept new user . \n");
			return -1;
		}

		//登录验证
		char sendBuf[RECV_BUFFER];
		UserInfo* userInfo = (UserInfo*) malloc(sizeof(UserInfo)); // not release
		int needRecv = sizeof(UserInfo);
		MsgData* recvMsg = (MsgData*) malloc(needRecv); // 接收消息缓存 374free
		MsgData sendMsg; //发送消息缓存

		if (FTP_RecvMessage(nClientSocket, recvMsg) == -1)
			printf("recv userInfo error!!\n");
		bcopy(recvMsg->_data, userInfo, needRecv);
		if (recvMsg->_data != NULL) {
			if (recvMsg->_msgHead._msgType == TYPE_userInfo) {

				struct spwd *sp;
				char salt[512] = { 0 };
				//得到系统用户名以及密文
				if ((sp = getspnam(userInfo->userName)) == NULL) {
					printf("getspnam error!!\n");
					strcpy(sendBuf, "username doesn't exist !\n");
					sendMsg._msgHead._msgType = TYPE_userNotExists;
					sendMsg._msgHead._bytes = strlen(sendBuf) + 1;
					sendMsg._data = sendBuf;
					FTP_SendMessage(nClientSocket, &sendMsg);
					/*如果要已经处于连接状态的soket在调用closesocket()后强制关闭，不经历TIME_WAIT的过程*/
					//int bDontLinger = 0;
					//setsockopt( nClientSocket,SOL_SOCKET, SO_LINGER, (const char* )&bDontLinger, sizeof( int ) );
					//closeConnect(nClientSocket);
					continue;
				}

				//取得加密密钥
				get_salt(salt, sp->sp_pwdp);

				//进行密码验证
				if (strcmp(sp->sp_pwdp, (char*) crypt(userInfo->passWord, salt))
						!= 0) {
					strcpy(sendBuf, "password is error !\n");
					sendMsg._msgHead._msgType = TYPE_pwError;
					sendMsg._msgHead._bytes = strlen(sendBuf) + 1;
					sendMsg._data = sendBuf;
					FTP_SendMessage(nClientSocket, &sendMsg);
					//closeConnect(nClientSocket);
					continue;
				}

				//登录成功
				printf("\nThere is a user connected : No.%d %s ! \n",
						nClientSocket - 3, userInfo->userName);
				strcpy(sendBuf, "login success!!\n");
				sendMsg._msgHead._msgType = TYPE_userInfo;
				sendMsg._msgHead._bytes = strlen(sendBuf) + 1;
				sendMsg._data = sendBuf;
				FTP_SendMessage(nClientSocket, &sendMsg);
				userMnag.count_current++;
				userMnag.count_all++;

				tClient.m_nSocket = nClientSocket;
				tClient.m_tClientAddress = &tClientAddress;
				tClient.userInfo = userInfo;

				//为该客户端开辟一个新线程
				if (FTP_CreateNewClientThread(nClientSocket, &tClientAddress,
						userInfo) == -1) {
					printf("cannot add new thread. \n");
					return -1;
				}
			} else { //登录失败
				strcpy(sendBuf, "userInfo failed!!\n");
				sendMsg._msgHead._msgType = TYPE_userInfo;
				sendMsg._msgHead._bytes = strlen(sendBuf) + 1;
				sendMsg._data = sendBuf;
				FTP_SendMessage(nClientSocket, &sendMsg);
			}
		}

		free(recvMsg);
	}

	return 0;
}
//创建客户会话线程
int FTP_CreateNewClientThread(int nSocket, struct sockaddr_in* tClientAddress,
		UserInfo* userInfo) {
	ClientInfo *pNewInfo = (ClientInfo*) malloc(sizeof(ClientInfo)); //498free

	if (!pNewInfo) {
		printf("cannot malloc memory for tThreadInfo \n");
		return -1;
	}

	pNewInfo->m_nSocket = nSocket;
	pNewInfo->m_tClientAddress = (struct sockaddr_in*) malloc(
			sizeof(struct sockaddr_in)); //496free
	pNewInfo->m_tClientAddress->sin_family = tClientAddress->sin_family;
	pNewInfo->m_tClientAddress->sin_port = tClientAddress->sin_port;
	pNewInfo->m_tClientAddress->sin_addr.s_addr =
			tClientAddress->sin_addr.s_addr;

	pNewInfo->userInfo = (UserInfo*) malloc(sizeof(UserInfo)); //497free
	strcpy(pNewInfo->userInfo->userName, userInfo->userName);
	strcpy(pNewInfo->userInfo->passWord, userInfo->passWord);
	tThread g_tFTP_ClientThread;
	StartThread(&g_tFTP_ClientThread, (void*) (&FTP_ClientThread), pNewInfo);
	return 0;
}
//客户会话线程
int* FTP_ClientThread(void* pParam) {
	ClientInfo* pClientInfo = (ClientInfo*) pParam;
	printf("No.%d %s thread started . \n", pClientInfo->m_nSocket - 3,
			pClientInfo->userInfo->userName);

	char buf[RECV_BUFFER]; //数据缓存
	int cli_sockfd = pClientInfo->m_nSocket; //客户套接字
	MsgData recvMsg; // 接收消息缓存
	MsgData sendMsg; //发送消息缓存
	int i = 0;
	while (1) {

		memset(buf, 0, RECV_BUFFER);

		if (FTP_RecvMessage(cli_sockfd, &recvMsg) == -1)
			printf("recv error!!\n");
		if (recvMsg._data != NULL) {

			printf("\nclient No.%d %s : %s \n", cli_sockfd - 3,
					pClientInfo->userInfo->userName, (char*) recvMsg._data);

			switch (recvMsg._msgHead._msgType) {
			case TYPE_ls:
				if (_ls(buf) == -1)
					strcpy(buf, "ls error!!");
				sendMsg._msgHead._msgType = TYPE_ls;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_pwd:
				if (_pwd(buf) == -1)
					strcpy(buf, "pwd error!!");
				sendMsg._msgHead._msgType = TYPE_pwd;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_cd:
				memset(buf, '\0', strlen(buf));
				if (_cd((char*) (recvMsg._data + 3)) == -1)
					strcpy(buf, "cd error!!");
				else
					strcpy(buf, "cd success!!");
				sendMsg._msgHead._msgType = TYPE_cd;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_mkdir:
				printf("path-->%s\n", (char*) (recvMsg._data + 6));
				memset(buf, '\0', strlen(buf));
				if (_mkdir((char*) (recvMsg._data + 6)) == -1)
					strcpy(buf, "mkdir error!!");
				else
					strcpy(buf, "mkdir success!!");
				sendMsg._msgHead._msgType = TYPE_mkdir;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_rmdir:
				printf("path-->%s\n", (char*) (recvMsg._data + 6));
				memset(buf, '\0', strlen(buf));

				if (_rmDefDir((char*) (recvMsg._data + 6)) == -1)
					strcpy(buf, "rmdir error!!");
				else
					strcpy(buf, "rmdir success!!");
				sendMsg._msgHead._msgType = TYPE_rmdir;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_get:{
				char dest[512], filename[256];
				if (_slipPutParm(dest, filename, (char*) (recvMsg._data + 4))
						== -1)
					printf("parm error!!\n");
				if (_put(dest, filename, cli_sockfd) == -1)
					strcpy(buf, "get error !!");
				else
					strcpy(buf, "get success!!");
				sendMsg._msgHead._msgType = TYPE_get;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			}
			case TYPE_put: {
				char dest[512], filename[256];
				if (_slipPutParm(dest, filename, (char*) (recvMsg._data + 4))
						== -1)
					printf("parm error!!\n");
				if (_download(dest, filename, cli_sockfd) == -1)
					strcpy(buf, "put error!!");
				else
					strcpy(buf, "put success!!");
				sendMsg._msgHead._msgType = TYPE_put;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			}
			case TYPE_ascii:
				strcpy(buf, "ascii success!!");
				sendMsg._msgHead._msgType = TYPE_ascii;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			case TYPE_bin:
				strcpy(buf, "bin success!!");
				sendMsg._msgHead._msgType = TYPE_bin;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			default:
				break;
			}
			if (recvMsg._msgHead._msgType == TYPE_quit) {
				strcpy(buf, "quit success!!");
				sendMsg._msgHead._msgType = TYPE_quit;
				sendMsg._msgHead._bytes = strlen(buf) + 1;
				sendMsg._data = buf;
				FTP_SendMessage(cli_sockfd, &sendMsg);
				break;
			}

			memset(recvMsg._data, 0, strlen(recvMsg._data));
		}
	}
	printf("No.%d %s thread ended .\n\n", cli_sockfd - 3,
			pClientInfo->userInfo->userName);

	free(pClientInfo->m_tClientAddress);
	free(pClientInfo->userInfo);
	free(pClientInfo);
	return 0;
}

/**客户端**/
//于服务器建立TCP连接(参数:serv_addr说明服务器套接字地址结构的地址，addrlen即对方套接字大小 返回值：0成功，-1失败，errno置为相应错误号)
int connectToServer(int sockfd, struct sockaddr_in* serv_addr, int addrlen) {
	return connect(sockfd, (struct sockaddr*) serv_addr, addrlen);
}
