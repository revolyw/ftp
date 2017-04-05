#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "mySocket.h"

int main(int argNum, char** args) {
	/**
	 *get username and password
	 */
	char userName[256];
	char passWord[256];
	printf("\n");
	printf("userName:");
	scanf("%s", userName);
	printf("passWord:");
	scanf("%s", passWord);
	printf("\n");
	/**
	 *client socket initializing
	 */
	struct sockaddr_in serv_addr;
	int sockfd = -1;
	char serverIp[20];
	int port = 95271;

	strcpy(serverIp, "127.0.0.1");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(serverIp);

	printf("client initializing......\n");

	sockfd = getSocket();
	if (sockfd == -1) {
		printf("client getting socket error!!\n");
		return -1;
	}
	/**
	 * connecting to servcer
	 */
	if (connectToServer(sockfd, &serv_addr, sizeof(struct sockaddr_in)) == -1) {
		printf("connect to server error!!\n");
		return -1;
	}

	char * ip_string = inet_ntoa(serv_addr.sin_addr); //将一个IP转换成一个互联网标准点分格式的字符串。
	printf("Connect to server success!! serverIp: %s,port:%d\n", ip_string,
			port);

	/**
	 * login
	 */
	MsgData sendMsg, recvMsg;

	UserInfo userInfo;
	strcpy(userInfo.userName, userName);
	strcpy(userInfo.passWord, passWord);
	int needSend = sizeof(UserInfo);
	char* buf = (char*) malloc(needSend); //83 release
	bcopy(&userInfo, buf, needSend);
	sendMsg._msgHead._msgType = TYPE_userInfo;
	sendMsg._msgHead._bytes = needSend;
	sendMsg._data = buf;
	FTP_SendMessage(sockfd, &sendMsg);

	if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
		printf("recv login info error!!\n");

	if (recvMsg._msgHead._msgType == TYPE_userNotExists
			|| recvMsg._msgHead._msgType == TYPE_pwError) {
		printf("%splease try again!\n", (char*) recvMsg._data);

		if (closeConnect(sockfd) == -1) {
			printf("socket资源回收失败!!\n");
		}
		return -1;
	}

	/**
	 *Send Message....
	 */

	char pBuffer[RECV_BUFFER];
	char cmd[RECV_BUFFER];

	while (fgetc(stdin) != '\n')
		; //清理标准输入缓冲
	while (1) {
		printf("\n");
		printf("Myftp>");

		fgets(cmd, RECV_BUFFER, stdin);
		cmd[strlen(cmd) - 1] = '\0';

		if (!strcmp(cmd, "")) {

			continue;

		} else if (!strcmp(cmd, "ls")) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_ls;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);
			memset(recvMsg._data, 0, strlen(recvMsg._data));

			continue;

		} else if (!strcmp(cmd, "pwd")) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_pwd;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);
			memset(recvMsg._data, 0, strlen(recvMsg._data));

			continue;

		} else if (!strncmp(cmd, "cd", 2)) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_cd;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);
			memset(recvMsg._data, 0, strlen(recvMsg._data));

			continue;

		} else if (!strncmp(cmd, "mkdir", 5)) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_mkdir;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);
			memset(recvMsg._data, 0, strlen(recvMsg._data));

			continue;

		} else if (!strncmp(cmd, "rmdir", 5)) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_rmdir;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);
			memset(recvMsg._data, 0, strlen(recvMsg._data));

			continue;

		} else if (!strncmp(cmd, "put", 3)) {
			char dest[512], filename[256];
			if (_slipPutParm(dest, filename, (char*) (cmd + 4)) == -1)
				printf("parm error!!\n");
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_put;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (_put(dest, filename, sockfd) == -1)
				printf("upload error!!\n");
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("recv error!!\n");
			printf("%s\n", (char*) recvMsg._data);

			continue;

		} else if (!strncmp(cmd, "get", 3)) {
			char dest[512], filename[256];
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_get;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (_slipPutParm(dest, filename, (char*) (cmd + 4)) == -1)
				printf("parm error!!\n");
			printf("%s %s\n",dest,filename);
			if (_download(dest, filename, sockfd) == -1)
				printf("download error!!\n");
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("recv error!!\n");
			printf("%s\n\n", (char*) recvMsg._data);

			continue;

		} else if (!strncmp(cmd, "quit", 4)) {
			strcpy(pBuffer, cmd);
			sendMsg._msgHead._msgType = TYPE_quit;
			sendMsg._msgHead._bytes = strlen(cmd) + 1;
			sendMsg._data = pBuffer;
			FTP_SendMessage(sockfd, &sendMsg);
			if (FTP_RecvMessage(sockfd, &recvMsg) == -1)
				printf("Myftp>recv error!!\n");
			if (closeConnect(sockfd) == -1) {
				printf("socket resycle failed !!\n");
				continue;
			}
			printf("%s\n", (char*) recvMsg._data);
			break;
		} else {
			printf("The command %s is not found !! \n", cmd);
		}
	}

	free(buf);
	printf("\n");
	return 0;
}

