/*
* File : Receiver.cpp
* Julio Savigny, 2015
* Credits to
    Hartono Sulaiman Wijaya
	Alfian Ramadhan
	Septu Jamasoka
*/
#include "dcomm.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>

using namespace std;
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500
/* Define receive buffer size */
#define RXQSIZE 8
/* Define minimum upperlimit */
#define MIN_UPPERLIMIT 4
/* Define maximum lowerlimit */
#define MAX_LOWERLIMIT 1
Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false, send_xoff = false;
/* Socket */
int sockfd; // listen on sock_fd
/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);
struct sockaddr_in serv_addr, cli_addr;
void error(char *msg)
{
  perror(msg);
  exit(1);
}
struct sockaddr_storage clntAddr;
socklen_t clntAddrLen = sizeof(clntAddr);

// global variable
int co = 0;		//count total byte which want to be read
int con = 0;	//count total byte which want to be consumed
Byte current_byte;			//value of byte read
int main(int argc, char *argv[])
{
    Byte c;
    /*
    Insert code here to bind socket to the port number given in argv[1].
    */
    int iSetOption = 1;
    sockfd=socket(AF_UNSPEC,SOCK_DGRAM,IPPROTO_UDP);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    //bzero((char *) &serv_addr, sizeof(serv_addr));
    char * portno=argv[1];
    /*serv_addr.sin_family = AF_UNIX;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;*/
    struct addrinfo addrCriteria;						// Criteria for address
	memset(&addrCriteria, 0, sizeof(addrCriteria));		// Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;					// Any address family
	addrCriteria.ai_flags = AI_PASSIVE;					// Accept on any address/port
	addrCriteria.ai_socktype = SOCK_DGRAM;				// Only datagram socket
	addrCriteria.ai_protocol = IPPROTO_UDP;
    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(NULL, portno, &addrCriteria, &servAddr);
	if (rtnVal != 0) {
		printf("getaddrinfo() failed");
		return 0;
	}
    printf("Binding pada :%s \n",portno);
    if ((sockfd, servAddr->ai_addr, servAddr->ai_addrlen) < 0)
        error("ERROR on binding");

    /* Initialize XON/XOFF flags */

    /* Create child process */
    /*** IF PARENT PROCESS ***/
    while (true) {
        c = *(rcvchar(sockfd, rxq));
        /* Quit on end of file */
        if (c == Endfile) {
            exit(0);
        }
    }
    /*** ELSE IF CHILD PROCESS ***/
    while (true) {
    /* Call q_get */
    /* Can introduce some delay here. */
        Byte * test = q_get(rxq,&c);
        if (test != NULL) {
            //front not in 0
            if (rxq->front > 0) {
                if (rxq->data[rxq->front-1] != Endfile && rxq->data[rxq->front-1] != CR && rxq->data[rxq->front-1] != LF) {
                    printf("Consuming byte %i: '%c'\n",++con,rxq->data[rxq->front-1]);
                } else if (rxq->data[rxq->front-1] == Endfile) {
                    //if endfile
                    printf("End of File accepted.\n");
                    exit(0);
                }
            } else {
                if (rxq->data[7] != Endfile && rxq->data[7] != CR && rxq->data[7] != LF) {
                    printf("Consuming byte %i: '%c'\n",++con,rxq->data[7]);
                } else if (rxq->data[7] == Endfile) {
                    //if endfile
                    printf("End of File accepted.\n");
                    exit(0);
                }
            }
        }
        //delay
        usleep(DELAY*1000);

    }
    close(sockfd);
}
Byte tes[2];

static Byte *rcvchar(int sockfd, QTYPE *queue)
{
    /*
    Insert code here.
    Read a character from socket and put it to the receive buffer.
    If the number of characters in the receive buffer is above certain
    level, then send XOFF and set a flag (why?).
    Return a pointer to the buffer where data is put.
    */
    Byte *buffer;
	if (!send_xoff) {
		//read from socket & push it to queue
		ssize_t numBytesRcvd = recvfrom(sockfd, tes, sizeof(tes), 0,
			(struct sockaddr *) &clntAddr, &clntAddrLen);

		if (numBytesRcvd < 0) {
			//if error
			printf("recvfrom() failed\n");
		} else {
			//fill the circular
			queue->data[queue->rear] = tes[0];
			queue->count++;
			if (queue->rear < 7) {
				queue->rear++;
			} else {
				queue->rear = 0;
			}
			co++;
		}

		if (tes[0] != Endfile && tes[0] != CR && tes[0] != LF && co>0 ) {
			printf("Menerima byte ke-%i.\n",co);
		}

		//if buffer size excess minimum upperlimit
		if (queue->count > MIN_UPPERLIMIT && sent_xonxoff == XON) {
			sent_xonxoff = XOFF;
			send_xoff = true;
			send_xon = false;
			printf("Buffer > minimum upperlimit. Send XOFF.\n");
			char test[2];
			test[0] = XOFF;
			//send XOFF to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
				(struct sockaddr *) &clntAddr, sizeof(clntAddr));
			if (numBytesSent < 0)
				printf("sendto() failed)");
		}

		return &tes[0];

	} else {
		*buffer = 0;
		return buffer;
	}

    }
    /* q_get returns a pointer to the buffer where data is read or NULL if
    * buffer is empty.
    */
static Byte *q_get(QTYPE *queue, Byte *data)
{

    /*
    Insert code here.
    Retrieve data from buffer, save it to "current" and "data"
    If the number of characters in the receive buffer is below certain
    level, then send XON.
    Increment front index and check for wraparound.
    */
    Byte *current;
    /* Nothing in the queue */
    if (!queue->count) return (NULL);
    else {
		do {
			//obtain buffer
			if (queue->count > 0) {
				(*data) = queue->data[queue->front];
				queue->count--;
				if (queue->front < 7) {
					queue->front++;
				} else {
					queue->front = 0;
				}
			}
			//check if data valid
		} while ((*data < 32) && (*data != LF) && (queue->count > 0));

		//if count reach the maksimum lowerlimit
		if (queue->count < MAX_LOWERLIMIT && sent_xonxoff == XOFF) {
			sent_xonxoff = XON;
			send_xon = true;
			send_xoff = false;
			printf("Buffer < maksimum lowerlimit. Send XON\n");
			char test[2];
			test[0] = XON;
			//send XON to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
			(struct sockaddr *) &clntAddr, sizeof(clntAddr));
			if (numBytesSent < 0)
				printf("sendto() failed\n");
		}

		return data;
	}
}

