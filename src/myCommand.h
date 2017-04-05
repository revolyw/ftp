#define FILE_RECV_BUF 1024
#define BUF_END_MARKER 4

int _pwd(char *path);

int _cd(char *dir);

int _ls(char* pBuf);

int _mkdir(char *dir);

int _download(char* dest, char* filename, int skd);

int _slipPutParm(char* dest, char* filename, const char* parm);

int _put(char* dest, char* filename, int skd);

void _rmdir();

int _rmDefDir(char* filename);
