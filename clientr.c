#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#define BUFSIZE		4096


char	*service;
char	*host = "localhost";
pthread_mutex_t lock;
pthread_t readers[50];

int connectsock( char *host, char *service, char *protocol );

char* toStr(char buf[256], int src){
	buf[0] = '\0';
	char tmpbuf[256] = "";
	int i = 0;
	while(src){
		tmpbuf[i] = (char) ((src%10) + 48);
		src/=10;
		i++;
	}
	i = 0;
	for(int j = strlen(tmpbuf) - 1; j >= 0; j--){
		buf[i] = tmpbuf[j];
		i++;
	}
	return buf;
}

int toInt(char * a){
	int number = 0;
	int size = strlen(a);
	for (int i = 0; i < size; i++){
		number *= 10;
		number += ((int) a[i]) - 48;
	}
	return number;
}


int getWord(char src[4096], char dest[256], int wordIndex){
	int i = 0;
	int j = 0;
	int wordNum = 0;
	int cnt = 0;
	int k;
	for(j = 0;j <= strlen(src) && cnt < 1; j++){
		if((src[j] == ' ' ||src[j] == '\0' || src[j] == '\n') 
		&& src[j-1] != ' ' && src[j-1] != '\n'){
			wordNum++;
			if(wordNum == wordIndex){
				for(k = 0;i < j; i++, k++){
					dest[k] = src[i];
				}
			dest[k] = '\0';
			cnt++;
			j++;
			}else {
				i = j + 1;
			}	
		}else if(src[j] == ' ' || src[j] == '\n'){
		    i++;
		}
		    
	}
	return cnt;
}

void * readText(void * args){
	int file;
	pthread_mutex_lock(&lock);
	long cnt = (long) args;
	pthread_mutex_unlock(&lock);
	char cbuf[25];
	char filename[50] = "file of reader number ";
	strcat(filename, toStr(cbuf, (int) cnt));
	if((file = open(filename, O_CREAT | O_RDWR, S_IRWXU)) < 0){
		printf("Error opening the file\n");
		return 0;
	}
	char		buf[4*BUFSIZE];	
	int		cc;
	int		csock;
	buf[0] = '\0';
	strcat(buf, "READ readWrite.txt");
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}
	
	printf( "The server is ready, please start sending to the server.\n" );
	printf( "Type q or Q to quit.\n" );
	fflush( stdout );	
	
	if ( write( csock, buf, strlen(buf) ) < 0 )
	{
		fprintf( stderr, "client write: %s\n", strerror(errno) );
		exit( -1 );
	}
	
	if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
        {
        	printf( "The server has gone.\n" );
                close(csock);
                exit(-1);
        }
        else
        {
        	buf[cc] = '\0';
        	char args[256];
                getWord(buf, args, 1);
                if(strcmp(args, "SIZE") != 0){
                	printf("WRONG PROTOCOL\n");
                	exit(-1);
                }else{
                
                	getWord(buf, args, 2);
                	int size = toInt(args);
                	int i = 0;
                	while(i < size){
                		if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
				{
					printf( "The server has gone.\n" );
					close(csock);
					exit(-1);
				}
                	}
                	
                	dprintf(file, "%s\n", &buf[strlen(args) + 6]);
                }
	}
}
/*
**	Client
*/
int main( int argc, char *argv[] )
{
	
	long count = 0;
	switch( argc ) 
	{
		case    2:
			service = argv[1];
			break;
		default:
			fprintf( stderr, "usage: chat [host] port\n" );
			exit(-1);
	}


	// 	implement poisson distribution
	while (count < 2)
	{
		pthread_mutex_lock(&lock);
		pthread_create(&readers[count], NULL, &readText, (void*)count);
		count++;
		pthread_mutex_unlock(&lock);
	}
	return 0;

}


