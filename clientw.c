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
#include <math.h>
#define BUFSIZE		4096


char	*service;
char	*host = "localhost";
char wordset[5] = "ABCD";


pthread_t writers[50];
pthread_mutex_t lock;
int connectsock( char *host, char *service, char *protocol );


double poissonRandomInterarrivalDelay( double L )
{
    return (log((double) 1.0 - ((double) rand())/((double) RAND_MAX)))/-L;
}

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

void * writeText(void * args){
	pthread_mutex_lock(&lock);
	long cnt = (long) args;
	pthread_mutex_unlock(&lock);
	char cbuf[BUFSIZE];
	
	char		buf[BUFSIZE];	
	int		cc;
	int		csock;
	//connect
	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}
	int word = csock%4;
	//fill out the buffer with certain letters
	for(int i = 0; i < BUFSIZE; i++){
		buf[i] = wordset[word];
	}
	//request writing access
	if((cc = write(csock, "WRITE readWrite.txt", 20)) < 0){
		close(csock);
		pthread_exit(0);
	}
	//read the answer
	if((cc = read(csock, cbuf, BUFSIZE)) < 0){
		close(csock);
		pthread_exit(0);
	}
	char answer[BUFSIZE];
	int ret = getWord(cbuf, answer, 1);
	//check the answer
	if(strcmp(answer, "GO") != 0){
		printf("WRONG PROTOCOL\n");
		close(csock);
		pthread_exit(0);
	}
	//tell the size
	if((cc = write(csock, "SIZE 2097152", 13)) < 0){
		close(csock);
		pthread_exit(0);
	}
	//start writing to the socket
	int i = 0;
	while(i < 5012){
		if( (cc = write(csock, buf, strlen(buf))) < 0){
			close(csock);
			pthread_exit(0);
		}
		i++;
	}
	
	printf( "The server is ready, please start sending to the server.\n" );
	fflush( stdout );	
	
	if ( write( csock, buf, strlen(buf) ) < 0 )
	{
		fprintf( stderr, "client write: %s\n", strerror(errno) );
		exit( -1 );
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
	while (count < 50)
	{
		pthread_mutex_lock(&lock);		
		pthread_create(&writers[count], NULL, &writeText, (void*)count);
		count++;
		pthread_mutex_unlock(&lock);
		poissonRandomInterarrivalDelay( 0.2);
	}
	return 0;

}


