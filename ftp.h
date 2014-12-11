#ifndef FTP_H
#define FTP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define OFFSET_VECTOR_SIZE 60
#define SERVER_PORT 21

typedef struct FTP{
	int socket_fd; // socket file descriptor
	int data_socket_fd; // data socket file descriptor
} FTP;

typedef struct URL{
    char user[256]; 
    char password[256]; 
    char host[256]; 
    char * host_ip;
    char path[256];  
    char * file_name;

} URL;

const char* REGEX_WITH_USER = "ftp://%[^:]:%[^@]@%[^/]/%s\n";
const char* REGEX_WITHOUT_USER = "ftp://%[^/]/%s\n";




int ftp_connect(FTP* ftp, char* ip, int port);

int ftp_login(FTP* ftp, char* user, char* password);

int ftp_passive_mode(FTP * ftp);

int ftp_change_dir(FTP* ftp, char* directory_path);

int ftp_send_cmd(FTP* ftp,  char* cmd);

int ftp_send_cmd_read_after(FTP *ftp, char *cmd);

int ftp_read(FTP * ftp, char* cmd, int size);

int ftp_retrieve_file(FTP * ftp, URL * url);

int ftp_download(FTP* ftp, URL * url) ;

void reverse_string(char str[]);

int url_parser(URL * url, char * url_string);

#endif