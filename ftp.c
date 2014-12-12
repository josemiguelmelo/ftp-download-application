#include "ftp.h"

int ftp_socket_connect(const char* ip, int port){
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    /** connect to socket. if error, exit with code -1. */
    if ((sockfd = socket(server_addr.sin_family, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    /** connect to server. if error, exit with code -2. */
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-2);
    }
    return sockfd;
}


/** Connect to ftp */
int ftp_connect(FTP* ftp, char* ip, int port) {

	int sockfd = ftp_socket_connect(ip, port);

    if (sockfd < 0) {
        perror("connect_socket");
        return sockfd;
    }

    ftp->socket_fd = sockfd;
    
    usleep(500000); // sleep half a second

    char str[1024] = "";
    
    int read_return = ftp_read(ftp, str, sizeof(str));

    if (read_return != 0){
        perror("ftp_read");	// error reading from ftp
        return read_return;
    }

    return 0;
}

int ftp_disconnect(FTP* ftp) {
    char cmd[1024] = "QUIT\r\n";
    int send_cmd_response = ftp_send_cmd_read_after(ftp, cmd);
    if (send_cmd_response!=0){
        perror("disconnect ftp");
        return send_cmd_response;
    }
    close(ftp->socket_fd);   
    close(ftp->data_socket_fd);
    return 0;
}


/**
	user command ---> USER username
	password command ---> PASS password
*/
int ftp_login(FTP* ftp, char* user, char* password){
	char cmd[1024] = "";

    sprintf(cmd, "USER %s\r\n", user); // cmd => USER $user
	int sent_user_successfully = ftp_send_cmd_read_after(ftp, cmd);

    if (sent_user_successfully != 0){
        perror("ftp_login (USER)");
        return sent_user_successfully;
    }

    sprintf(cmd, "PASS %s\r\n", password); // cmd => PASS $password
	int sent_password_successfully = ftp_send_cmd_read_after(ftp, cmd);

    if (sent_password_successfully != 0){
        perror("ftp_login (PASS)");
        return sent_password_successfully;
    }

    return 0;
}

/**
Set passive mode. Command to use: PASV

The server will respond with the address of the port it is listening on, with a message like:
		227 Entering Passive Mode (a1,a2,a3,a4,p1,p2)
		where a1.a2.a3.a4 is the IP address and p1*256+p2 is the port number.
*/
int ftp_passive_mode(FTP * ftp){
	char cmd[1024] = "PASV\r\n";
	char answer_received[1024];
    int error = ftp_send_cmd(ftp, cmd);
    if (error!=0) {
        perror("passive move send cmd (CWD)");
        return error;
    }
    sleep(1);
    // read answer
    int answer = ftp_read(ftp, answer_received, sizeof(answer_received));
    if (answer) {
        perror("passive move read");
        return answer;
    }

    //printf("%s\n", answer_received);

    int a1, a2, a3, a4, p1, p2; 	// a1..a4 -> ip's   ;  p1,p2 -> ports
    error = sscanf(answer_received, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &a1, &a2, &a3, &a4, &p1, &p2);
    if (error < 0) {
        perror("sscanf");
        return error;
    }

    // port = p1*256 + p2
    int final_port;
    char final_ip[32] = "";
    
    final_port = p1 * 256 + p2;
    sprintf(final_ip, "%d.%d.%d.%d", a1, a2, a3, a4);

    ftp->data_socket_fd = ftp_socket_connect(final_ip, final_port);

    if (ftp->data_socket_fd < 0) {
        perror("connect_socket");
        return 0;
    }

    sleep(1);

    return 0;
}

/** change ftp current current directory. Command used: CWD directory */
int ftp_change_dir(FTP* ftp, char* directory_path){
	char cmd[1024] = "";
    sprintf(cmd, "cwd %s\r\n", directory_path);	// cmd => CWD $directory_path
    int sent_cmd_successfully = ftp_send_cmd(ftp, cmd);
    sleep(1);
    char rec [256];
    ftp_read(ftp, rec, 256);
    printf("%s\n", rec );

    if (sent_cmd_successfully!=0){
        perror("ftp_cwd");
        return sent_cmd_successfully;
    }
    return 0;
}
/** send command to ftp. */
int ftp_send_cmd(FTP* ftp,  char* cmd){
	ssize_t wrote = write(ftp->socket_fd, cmd, strlen(cmd));
    if (wrote == 0) {
        perror("write command");
        return -1;
    }
    return 0;
}

/** send command to ftp and read answer after sending */
int ftp_send_cmd_read_after(FTP *ftp, char *cmd) {
   
    ssize_t written = write(ftp->socket_fd, cmd, strlen(cmd));
    if (written == 0){
        perror("write");	// error writing to ftp
        return -1;
    }

    sleep(1);

    char str[1024] = "";
    ssize_t read_return = ftp_read(ftp, str, sizeof(str));

    if (read_return != 0) {
        perror("ftp_read");	// error reading from ftp
        return read_return;
    }

    /*
    	Server return codes
		  *     4xx ----> Transient Negative Completion reply
		  *     5xx ----> Permanent Negative Completion reply
    */
    if (str[0] == '4' || str[0] == '5') {
        return str[0];	// return server error code
    }

    return 0;
}

/** read from ftp */
int ftp_read(FTP * ftp, char* cmd, int size){
	int read_result = read(ftp->socket_fd, cmd, size);

    if (read_result==0){
        perror("read");	// error reading from ftp
        return -1;
    }
    //printf("%.*s\n", (int)read_result, cmd);
    return 0;
}

/*
Syntax: RETR remote-filename
Begins transmission of a file from the remote host. Must be preceded by either a PORT command or a PASV command to indicate where the server should send data.
*/
int ftp_retrieve_file(FTP * ftp, URL * url){
    char cmd[1024] = "";
    sprintf(cmd, "RETR %s\r\n", url->file_name);
    int send_cmd_response = ftp_send_cmd_read_after(ftp, cmd);
    sleep(1);
    if (send_cmd_response != 0) {
        perror("retrieve file");
        return send_cmd_response;
    }
    return 0;
}


int ftp_download(FTP* ftp, URL * url) {
    // retrieve file from ftp server
    int retrieve_file_result = ftp_retrieve_file(ftp, url);
    // error retrieving file
    if(retrieve_file_result != 0){
        fprintf(stderr, "Error retrieving file\n");
        return -1;
    }

    FILE* file = fopen(url->file_name, "wb");
    if (file == NULL) {
        perror("open file");
        return -1;
    }
    char receive_buffer[256];
    int len;
    while ((len = read(ftp->data_socket_fd, receive_buffer, 255))>0) {
        receive_buffer[len]='\0';
        printf("%d %s\n", len, receive_buffer);
        int error = fwrite(receive_buffer, 1, len, file);
        if (error < 0) {
            perror("fwrite");
            return error;
        }
    }

    close(ftp->data_socket_fd);
    ftp->data_socket_fd = 0;

    fclose(file);

    return 0;
}

void reverse_string(char str[]){
    int i,j;
    char temp[100];
    for(i=strlen(str)-1,j=0; i+1!=0; --i,++j)
    {
        temp[j]=str[i];
    }
    temp[j]='\0';
    strcpy(str,temp);
}

void removeSubstring(char *s,const char *toremove)
{
    while( strstr(s,toremove) ){
        memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
    }
}

/** get url from string */
int url_parser(URL * url, char * url_string){

    struct hostent *h;

    // if user inserted
    if(sscanf(url_string, REGEX_WITH_USER, url->user, url->password, url->host, url->path) == 4){
        printf("User: %s\n", url->user);
        printf("Pass: %s\n", url->password);
        printf("Host: %s\n", url->host);
       // printf("Path: %s\n", url->path);
    }
    // if not inserted user
    else if(sscanf(url_string, REGEX_WITHOUT_USER, url->host, url->path) == 2){
        printf("Host: %s\n", url->host);
        //printf("Path: %s\n", url->path);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "pass");
    }
    // if usage error
    else{
        fprintf(stderr,"Invalid URL!\n Usage: download ftp://[<user>:<password>@]<host>/<url-path>");
        exit(1);
    }
    printf("\n");

    if ((h=gethostbyname(url->host)) == NULL) {
        herror("gethostbyname");
        exit(2);
    }

    // set host ip
    url->host_ip = inet_ntoa(*((struct in_addr *)h->h_addr));

    // temporary string with path
    char temp_path[256]; 
    // copy path to temp_path
    memcpy(temp_path, url->path, sizeof(url->path));
    // reverse temporary path string to get file at begining
    reverse_string(temp_path);

    // string with file name
    char * file;
    const char delim[2] = "/";
    // get first string until delim is found (file name)
    file = strtok(temp_path, delim);

    // reverse file name
    reverse_string(file);
    // set file name
    strcpy(url->file_name,file);
    // remove file from path
    removeSubstring(url->path, url->file_name);

    return 0;
}
void url_initialize(URL * url){
    url->file_name=  (char*) malloc (256);
    url->host_ip=  (char*) malloc (256);
}

int main(int argc, char* argv[]){
    // check number of arguments
	if(argc!=2 || !strcmp(argv[0], "download")){
		printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>");
		return -1;
	}
    
    /** structs with url and ftp */
    URL url;
    FTP ftp;

    // initialize url
    url_initialize(&url);

    // get url from argument and insert into struct URL
    url_parser(&url, argv[1]);
    
    printf("Connecting to ftp...\n");
    // connect via ftp to host_ip through SERVER_PORT
    int connection_result = ftp_connect(&ftp, url.host_ip, SERVER_PORT);
    // error connecting ftp
    if(connection_result != 0){
        fprintf(stderr, "Error connecting to ftp\n");
        return -1;
    }
    printf("Connected successfully.\n\n");


    printf("Logging in...\n");
    // login with user credentials
    int login_result = ftp_login(&ftp, url.user, url.password);
    // error logging in
    if(login_result!=0){
        fprintf(stderr, "Login error. Invalid username or password.\n");
        return -1;
    }
    printf("Successfully logged in.\n\n");


    printf("Changing to passive mode...\n");
    // change connection to passive move
    int change_passive_mode_result = ftp_passive_mode(&ftp);
    // error changing to passive mode
    if(change_passive_mode_result != 0){
        fprintf(stderr, "Error changing to passive mode\n");
        return -1;
    }
    printf("Changed successfully to passive mode.\n\n");

    if(strcmp(url.path, "")){
        printf("Changing to given path...\n");
        // change to path given in ftp
        int change_path_result = ftp_change_dir(&ftp, url.path);
        // error changing to path given
        if(change_path_result!=0){
            fprintf(stderr, "Error changing to path %s\n", url.path);
            return -1;
        }
        printf("Changed to to ftp...\n\n");
    }

    printf("Downloading file...\n");
    // download file from ftp server
    int download_file_result = ftp_download(&ftp, &url);
    // error changing to passive mode
    if(download_file_result != 0){
        fprintf(stderr, "Error downloading file\n");
        return -1;
    }
    printf("File downloaded successfully.\n\n");


    printf("Disconnecting from ftp...\n");
    // disconnect from ftp server
    int disconnect_result = ftp_disconnect(&ftp);
    // error disconnecting from ftp server
    if(disconnect_result!=0){
        fprintf(stderr, "Error disconnecting from ftp server\n");
        return -1;
    }
    printf("Successfully disconnected.\n\n");

    return 0;
}
