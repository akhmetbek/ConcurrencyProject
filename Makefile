INCLUDES        = -I. -I/usr/include

LIBS		= libsocklib.a -L/usr/ucblib \
			-ldl -lnsl -lpthread -lm

COMPILE_FLAGS   = ${INCLUDES} -c
COMPILE         = gcc ${COMPILE_FLAGS}
LINK            = gcc -o

C_SRCS		= \
		passivesock.c \
		connectsock.c \
		clientr.c \
		clientw.c \
		server.c 
		 


SOURCE          = ${C_SRCS}

OBJS            = ${SOURCE:.c=.o}


EXEC		= clientr clientw server

.SUFFIXES       :       .o .c .h

all		:	library clientr clientw server


.c.o            :	${SOURCE}
			@echo "    Compiling $< . . .  "
			@${COMPILE} $<

library		:	passivesock.o connectsock.o
			ar rv libsocklib.a passivesock.o connectsock.o


server		:	server.o
			${LINK} $@ server.o ${LIBS}

clientr		:	clientr.o
			${LINK} $@ clientr.o ${LIBS}
clientw         : 	clientw.o
			${LINK} $@ clientw.o ${LIBS}

clean           :
			@echo "    Cleaning ..."
			rm -f tags core *.out *.txt *.o *.lis *.a ${EXEC} libsocklib.a
