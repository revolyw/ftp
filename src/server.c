#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "myThread.h"
#include "mySocket.h"

int main(int argNum, char** args) {
	/**
	 *server socket initialization
	 */
	char* ip = "127.0.0.1";
	int port = 95271;
	int client_max = 5; //用户请求队列峰值
	
	/**e
	 * get ip , port,and client_max from args
	 */
	if (argNum != 1)
		if (getServerArgs(args, &ip, &port, &client_max) == -1) {
			printf("bed args !! please run the server througth new args !\n");
			return -1;
		}
	/**
	 * server socket initializing
	 */
	int sockfd = FTP_RunServer(ip, port, client_max); //socket描述符
	if (sockfd == -1) {
		printf("cannot start server . WAIT_TIME . \n");
		return -1;
	}
	/**
	 *starting command thread
	 */
	if (FTP_CommandThread(sockfd) == -1) {
		printf("Starting server command thread failed !");
		return -1;
	}
	/**
	 * wait for client .....
	 **/
	if (FTP_ServerThread(sockfd) == -1) {
		printf("Starting server main thread failed !");
		return -1;
	}

	printf("\n");
	free(ip);
	return 0;
}

