all: download

download: ftp.o
	gcc ftp.o -o download
	rm -rf *.o

ftp: ftp.c
	gcc -Wall ftp.c

clean:
	rm -rf *o download