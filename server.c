#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#define	QLEN			50
#define	BUFSIZE			4096


pthread_t threads[QLEN];
int file;
char *output;
pthread_mutex_t rd;
pthread_mutex_t lock;
int count = 0;
int rcount = 0;
int passivesock( char *service, char *protocol, int qlen, int *rport );

/*
**	This poor server ... only serves one client at a time
*/


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

int toInt(char * a, int size){
	int number = 0;
	
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

int readerHandler(int ssock, char buf[BUFSIZE], char args[256], int cc){
	pthread_mutex_lock(&rd);
	
	rcount++;
	if(rcount == 1){
		pthread_mutex_lock(&lock);
	}
	pthread_mutex_unlock(&rd);
	
	
	char size[256] = "SIZE ";
	char Size[20] = "";
	strcat(size, toStr(Size , strlen(output) + 1));
	
	if((cc = write(ssock, size, strlen(size))) < 0){
		printf("The reader %i timed out\n", ssock);
		
		pthread_mutex_lock(&rd);
		close(ssock);
		rcount--;
		pthread_mutex_lock(&rd);
		pthread_mutex_unlock(&lock);

		return -1;
	}
	int i = 0;
	int chunkSize = 4096;
	while(cc){
		if((cc = write(ssock, &output[i*chunkSize], chunkSize)) < 0){
			printf("The reader %i has finished operation\n", ssock);
		
			pthread_mutex_lock(&rd);
			close(ssock);
			rcount--;
			pthread_mutex_lock(&rd);
			pthread_mutex_unlock(&lock);
		
			return -1;
		}
		i++;
		
	}
	
	printf("Client %i has gone\n", ssock);
	
	pthread_mutex_lock(&rd);
	rcount--;
	close(ssock);
	pthread_mutex_unlock(&rd);
	
	pthread_mutex_unlock(&lock);	
	return 0;
}

int writerHandler(int ssock, char buf[BUFSIZE], char args[256], int cc){
	
	pthread_mutex_lock(&lock);
	if(getWord(buf, args, 2) == 0){
		printf("error while getting the second word\n");
		return -1;
		pthread_mutex_unlock(&lock);
	}	
	char answer[100] = "GO ";
	strcat(answer, args);
	
	if ( write( ssock, answer, cc ) < 0 )
	{
		/* This guy is dead */
		printf("The writer %i has timed out\n", ssock);
		close( ssock );
		return -1;
		pthread_mutex_unlock(&lock);
	}
	
	if ((cc = read( ssock, buf, BUFSIZE)) < 0){
		printf( "The client has gone.\n" );
		close(ssock);
		return -1;
		pthread_mutex_unlock(&lock);
	}
	buf[cc] = '\0';
	getWord(buf, args, 1);
	if(strcmp(args, "SIZE") != 0){
		printf("Wrong protocol");
		close(ssock);
		pthread_mutex_unlock(&lock);
		return -1;
	}else {
		getWord(buf, args, 2);
		int dataSize = toInt(args, strlen(args));
		output[0] = '\0';
		
		strcat(output, &buf[strlen(args) + 6]);
		lseek(file, 0, SEEK_SET);
		dprintf(file, "%s\n", &buf[strlen(args) + 6]);
		
		if(dataSize > (BUFSIZE - 5 - strlen(args))){
			while(cc){
				if( (cc = read(ssock, buf, BUFSIZE) ) < 0){
					printf("Client has gone\n");
					close(ssock);
					return -1;
				}
				buf[cc] = '\0';
				strcat(output, buf);
				dprintf(file, "%s\n", buf);
			}
		}
	}
	close(ssock);
	printf("Client %i has gone\n", ssock);
	pthread_mutex_unlock(&lock);
	
	return 0;
}




void * talk(void * args){
	int cc = 0;
	long ssock = (long) args;
	char buf[BUFSIZE];
	for (;;)
	{
		if ( (cc = read( ssock, buf, BUFSIZE )) <= 0 )
		{
			printf( "The client has gone.\n" );
			close(ssock);
			break;
		}
		buf[cc-1] = '\0';
		char args[256];
		if (getWord(buf,  args, 1) == 0){
			printf("error while getting the first word\n");
			return 0;
		}
		if(strcmp(args, "WRITE") == 0){
			writerHandler(ssock, buf, args, cc);
			break;
		}else if(strcmp(args, "READ") == 0){
			readerHandler(ssock, buf, args, cc);
			break;
		}else{
			write(ssock, "WRONG PROTOCOL\n", cc);
			break;
		}
		
	}
}




int main( int argc, char *argv[] )
{
	output = malloc(BUFSIZE*1000);
	char			buf[BUFSIZE];
	char			*service;
	struct sockaddr_in	fsin;
	int			alen;
	int			msock;
	int			rport = 0;
	int			cc;
	if((file = open("readWrite.txt", O_CREAT | O_RDWR, S_IRWXU)) < 0){
		printf("Error opening the file\n");
		return 0;
	}
	if( (cc = read(file, output, 16*BUFSIZE)) < 0){
		printf("Error reading the file\n");
		return 0;
	}
	
	output[cc] = '\0';
	//int errrno = 0;
	//dprintf(file, "%s %i\n","HI", 2);
	
	switch (argc) 
	{
		case	1:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			break;
		case	2:
			// User provides a port? then use it
			service = argv[1];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
	{
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}

	long ssock[1000];
	for (;;)
	{

		alen = sizeof(fsin);
		ssock[count] = accept( msock, (struct sockaddr *)&fsin, &alen );
		if (ssock < 0)
		{
			fprintf( stderr, "accept: %s\n", strerror(errno) );
			exit(-1);
		}

		pthread_create(&threads[count], NULL, &talk, (void*) ssock[count]);
		count++;
		printf( "A client has arrived for operation.\n" );
		fflush( stdout );
		/* start working for this guy */
		/* ECHO what the client says */
	}
}


