ftp-download-application
========================

###Description

Implementation of an ftp application, which allows the user to download a specific file from an ftp server.


###Implementation

The application was implemented in C and uses sockets to connect to the server and retrieve files.

###Usage

#####Compilation

To compile the application run this command on the terminal:
'make'

#####Run

To run the application (after compiling) just run this command:
'''
./download ftp://[<user>:<password>@]<host>/<url-path>

<user> - login username
<password> - login password
<host> - ftp server to connect
<url-path> - file location in server
'''
