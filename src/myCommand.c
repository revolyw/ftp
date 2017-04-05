#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "myCommand.h"
#include "mySocket.h"

int _pwd(char * const path) {

	memset(path, 0, strlen(path));

	if (getcwd(path, 1024) == NULL) {
		return -1;
	}

//	printf("%s\n", path);

	return 0;
}

int _cd(char* const dir) {
	if (chdir(dir) == -1)
		return -1;
	return 0;
}

int _ls(char * const pBuf) {
	char buf[1024];
	int i = 1;

	memset(pBuf, 0, strlen(pBuf)); //必须
	memset(buf, 0, strlen(buf));

	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	if ((dp = opendir(".")) == NULL) {
		printf("ls open error!\n");
		return -1;
	}
	while ((entry = readdir(dp)) != NULL) {
		i++;
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(".", entry->d_name) == 0
					|| strcmp("..", entry->d_name) == 0)
				continue;
			strcat(buf, entry->d_name);
			if (i == 4) {
				strcat(buf, "\n");
				i = 0;
			} else
				strcat(buf, "\t");
		} else {
			strcat(buf, entry->d_name);
			if (i == 4) {
				strcat(buf, "\n");
				i = 0;
			} else
				strcat(buf, "\t");
		}
	}
	closedir(dp);

	strcpy(pBuf, buf);
//	printf("%s\n", pBuf);

	return 0;
}

int _mkdir(char *dir) {
	mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
	chmod(dir, 0777);
//	chown(dir,1,-1);
	return 0;
}

int _download(char* dest, char* filename, int skd) {
	FILE* fp;
	MsgData msg;
	int flag = 0;
	char buffer[FILE_RECV_BUF];
	char crtPath[256];
	getcwd(crtPath,256);
	chdir(dest);

	//打开文件
	if ((fp = fopen(filename, "w")) == NULL) {
		printf("file open error\n");
		exit(1);
	}
	bzero(buffer, FILE_RECV_BUF);
	//接受到来自客户端的文件
	while (1) {
		if(FTP_RecvMessage(skd, &msg)==-1)
		{
			printf("recv file data error!!\n");
			return -1;
		}
		flag++;
		if (msg._msgHead._bytes < 0) {
			printf("接收错误\n");
			exit(1);
		}
		int write_len = fwrite(msg._data, sizeof(char), msg._msgHead._bytes,
				fp);
		if (write_len > msg._msgHead._bytes) {
			printf("file write failed\n");
			exit(1);
		}
		if (msg._msgHead._bytes < FILE_RECV_BUF)
			break;
	}
	if (flag == 0)
		return -1;
	chmod(filename,0777);
	//关闭文件流
	fclose(fp);
	chdir(crtPath);
	return 0;
}

int _slipPutParm(char* dest, char* filename, const char* parm) {
	int index = 0, i = 0;
	char c = '\0';
	if (parm == NULL || !strcmp(parm, ""))
		return -1;
	while ((c = *(parm + index)) != '\040') {
		*(dest + i++) = c;
		index++;
	}
	*(dest + i) = '\0';
	i = 0;
	index++;
	while ((c = *(parm + index)) != '\0') {
		*(filename + i++) = c;
		index++;
	}
	*(filename + i) = '\0';
	return 0;
}

int _put(char* dest, char* filename, int skd) {
	FILE* fp;
	MsgData msg;
	char buffer[FILE_RECV_BUF];
	//打开指定文件
	if ((fp = fopen(filename, "r")) == NULL) {
		printf("the file was not opened\n");
		return -1;
	}
	bzero(buffer, FILE_RECV_BUF);
	printf("正在传输...\n");
	int len = 0;
	//将文件读入缓冲
	while (!feof(fp)) {
		len = fread(buffer, 1, FILE_RECV_BUF, fp);
		msg._data = buffer;
		msg._msgHead._bytes = len;
		if (FTP_SendMessage(skd, &msg) == -1) {
			printf("send file error\n");
			break;
		}
		bzero(buffer, FILE_RECV_BUF);
	}
	//关闭流
	if (fclose(fp)) {
		printf("file close error\n");
		exit(1);
	}
//	close(skd);吓尿了！！
	return 0;
}

void _rmdir() {
	DIR* dp = opendir(".");
	struct dirent* entry = NULL;
	struct stat st;
	if (!dp) {
		perror("opendir");
		exit(1);
	}
	while ((entry = readdir(dp)) != NULL) {
		stat(entry->d_name, &st);

		if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			chdir(entry->d_name);
			_rmdir();
			//这个chdir()是为了在删除了子目录后回来还是他的当前目录。用以保证下次循环时正确
			chdir("..");
			rmdir(entry->d_name);

		} else {
			remove(entry->d_name);
		}
	}

	closedir(dp);
}

int _rmDefDir(char* filename) {
	char path[512];
	getcwd(path, 512);
	strcat(path, "/");
	strcat(path, filename);

	struct stat info;
	stat(filename, &info);
	if (S_ISDIR(info.st_mode)) {
		char former_path[100];
		getcwd(former_path, 100);

		if (chdir(path) == -1) {
			fprintf(stderr, "change dir error");
			return -1;
		}
		printf("inner function %s\n", path);
		_rmdir();
		chdir(former_path);

		//删除该目录自身
		if (rmdir(path) == 0) {
			return 0;
		} else {
			return -1;
		}
	} else {
		if (remove(filename) == 0) {
			return 0;
		} else {
			return -1;
		}
	}
}
