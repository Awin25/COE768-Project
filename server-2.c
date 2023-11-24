#include "./ds.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

/*
Storage
Registration -> Peername (Internal Ref) needs to be saved with address
Download -> Get Peername based on content (least used)
List -> Retrieve list of ALL distinct content

De-Registration -> Content pulled based on Peername

From This -> Get This
-----------------------------------
Peername -> Address (IP + Port)

Content -> [Peernames]

Peername -> [Content] DONT NEED 

[Content] (distinctContentName)

*/


#define lengthBuffer		101			/* buffer length */
#define maxDataSize 100
#define maxContentNameSize 10
#define maxPeerNameSize 10
#define maxSize 10


#define contentRegister 'R'
#define contentDownload 'D'
#define contentSearch 'S'
#define contentDeregister 'T'
#define contenData 'C'
#define contentList 'O'
#define Ack 'A'
#define Error 'E'

struct transmissionPDU {
  char type;
  char data[maxDataSize];
};

// Index System Data Structures
typedef struct  {
  char contentName[maxContentNameSize]; 
  char peerName[maxPeerNameSize];
  int host;
  int port;
} contentInfo;

int main(int argc, char *argv[]) 
{
  struct  pduR packetRegister, *pktR;  //part of ds.h
  struct  pduS packetSearch; 
  struct  pduO packetListO;
  struct  pduT packetDeregisterT;
  struct  pduE packetError;
  struct  pduA packetAck;

  // Index System
  contentInfo contentInfos[maxSize]; 
  contentInfo contentTemporaryInfo;



  char registeredFiles[maxSize][maxSize];
  
  int headRegister = 0;
  int tailRegister = 0;
  
  bool commonMatch = false;
  bool fileIsRegistered = false;
bool  distinctContentName = true;

  // Transmission Variables
  struct  transmissionPDU recPacket;
  struct  transmissionPDU sendToPacket;
  int     r_host, r_port;
  char tcp_port[6];
	char tcp_ip_addr[5];

  // Socket Primitives
  struct  sockaddr_in fsin;	        /* the from address of a client	*/
	char    *pts;
	int		  sock;			                /* server socket */
	time_t	now;			                /* current time */
	int		  alen;			                /* from-address length */
	struct  sockaddr_in sin;          /* an Internet endpoint address */
  int     s, type;               /* socket descriptor and socket type */
	int 	  port = 3000;              /* default port */
	int 	   n;
  int i;
  int     FlagE = 0;


	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}
    /* Complete the socket structure */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;         //it's an IPv4 address
    sin.sin_addr.s_addr = INADDR_ANY; //wildcard IP address
    sin.sin_port = htons(port);       //bind to this port number
                                                                                                 
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
		fprintf(stderr, "can't create socket\n");
                                                                                
    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
    listen(s, 5);	
	  alen = sizeof(fsin); 
    


while (true) {
    if (recvfrom(s, &recPacket, lengthBuffer, 0, (struct sockaddr *)&fsin, &alen) < 0)
        fprintf(stderr, "[ERROR] recvfrom error\n");
    fprintf(stderr, "Received a Message\n");

    if (recPacket.type == contentRegister) {
        // Peer Server Content Registration
        memset(&packetRegister, '\0', sizeof(packetRegister));
 i = 0;
while (i < 10) {
    packetRegister.peerName[i] = recPacket.data[i]; // 0:9
    packetRegister.contentName[i] = recPacket.data[i + 10]; // 10:19
    if (i < 5) {
        packetRegister.host[i] = recPacket.data[i + 20]; // 20:25
    }
    if (i < 6) {
        packetRegister.port[i] = recPacket.data[i + 25]; // 26:31
    }
    i++;
}
        r_host = atoi(packetRegister.host);
        r_port = atoi(packetRegister.port);

        FlagE = 0;
i = 0;
while (i < tailRegister) { //print change
    fprintf(stderr, "Content Info %s %s %d %d\n", contentInfos[i].contentName, contentInfos[i].peerName, contentInfos[i].port, contentInfos[i].host);
    // Content has already been uploaded by Peer
    if (strcmp(contentInfos[i].contentName, packetRegister.contentName) == 0 && strcmp(contentInfos[i].peerName, packetRegister.peerName) == 0) {
        FlagE = 1;
        break;
    }
    // PeerName and Address MiscommonMatch
    else if (strcmp(contentInfos[i].peerName, packetRegister.peerName) == 0 && !(contentInfos[i].host == r_host)) {
        FlagE = 2;
        break;
    }
    i++;
}

        /*  Send Reply  */
        if (FlagE == 0) {
            // New Content?
            fileIsRegistered = false;
i = 0;
while (i < headRegister) {
    if (strcmp(registeredFiles[i], packetRegister.contentName) == 0) {
        fileIsRegistered = true;
        break;
    }
    i++;
}
            // If So, Commit Content ls List
            if (!fileIsRegistered) {
                strcpy(registeredFiles[headRegister], packetRegister.contentName);
                headRegister = headRegister + 1;
            }

            // Commit Content to contentInfos
            memset(&contentInfos[tailRegister], '\0', sizeof(contentInfos[tailRegister])); // Clean Struct
            strcpy(contentInfos[tailRegister].contentName, packetRegister.contentName);
            strcpy(contentInfos[tailRegister].peerName, packetRegister.peerName);
            contentInfos[tailRegister].host = r_host;
            contentInfos[tailRegister].port = r_port;

           fprintf(stderr, "Sending Registration PDU:\n");
            fprintf(stderr, "Peer Name: %s\n", contentInfos[tailRegister].peerName);
            fprintf(stderr, "Content Name: %s\n", contentInfos[tailRegister].contentName);
            fprintf(stderr, "Port %d, Host %d\n", contentInfos[tailRegister].port, contentInfos[tailRegister].host);
            tailRegister = tailRegister += 1; // Increment pointer to NEXT block

            // Send Ack Packet
            sendToPacket.type = Ack;
            memset(sendToPacket.data, '\0', 100);
            strcpy(sendToPacket.data, packetRegister.peerName);
            fprintf(stderr, "ACK\n");
        } else {
            // Send Err Packet
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', 100);
            if (FlagE == 2) {
                strcpy(sendToPacket.data, "PeerName exists in registration");
            } else if (FlagE == 1) {
                strcpy(sendToPacket.data, "file already exists in registration");
            }
            fprintf(stderr, "Error\n");
        }
        sendto(s, &sendToPacket, lengthBuffer, 0, (struct sockaddr *)&fsin, sizeof(fsin));
i = 0;
while (i < headRegister) {
    fprintf(stderr, "All registered files: %s\n", registeredFiles[i]);
    i++;
}
    }
        /* Peer Server Content Location Request */
       else if (recPacket.type == contentSearch) {
          //Localize Content
          memset(&packetSearch,'\0', sizeof(packetSearch));
 i = 0;
while (i < 10) {
    packetSearch.peerName[i] = recPacket.data[i]; // 0:9
    packetSearch.contentNameOrAddress[i] = recPacket.data[i + 10]; // 10:19
    i++;
}
          commonMatch = false;
i = 0;
while (i < tailRegister) {
    if (strcmp(packetSearch.contentNameOrAddress, contentInfos[i].contentName) == 0) {
        fprintf(stderr, "packet S Searched File found: %s %s %d %d\n", contentInfos[i].contentName, contentInfos[i].peerName, contentInfos[i].port, contentInfos[i].host);
        memset(&contentTemporaryInfo, '\0', sizeof(contentTemporaryInfo));
        strcpy(contentTemporaryInfo.contentName, contentInfos[i].contentName);
        strcpy(contentTemporaryInfo.peerName, contentInfos[i].peerName);
        contentTemporaryInfo.host = contentInfos[i].host;
        contentTemporaryInfo.port = contentInfos[i].port;
        // Have to send host and port here according to the commonMatched content addr.
        commonMatch = true;
    }
    // If content is found, shift all content down queue
    if (commonMatch && i < tailRegister - 1) {
        strcpy(contentInfos[i].contentName, contentInfos[i + 1].contentName);
        strcpy(contentInfos[i].peerName, contentInfos[i + 1].peerName);
        contentInfos[i].host = contentInfos[i + 1].host;
        contentInfos[i].port = contentInfos[i + 1].port;
    }
    i++;
}

          if (commonMatch) {
            tailRegister = tailRegister-1;
            // Commit Content at EOQ
            memset(&contentInfos[tailRegister],'\0', sizeof(contentInfos[tailRegister])); // Clean Struct
            strcpy(contentInfos[tailRegister].contentName,contentTemporaryInfo.contentName);
            strcpy(contentInfos[tailRegister].peerName,contentTemporaryInfo.peerName);
            contentInfos[tailRegister].host = contentTemporaryInfo.host;
            contentInfos[tailRegister].port = contentTemporaryInfo.port;
            tailRegister = tailRegister+1;

            // TODO: Send S Packet with host and port
            sendToPacket.type = contentSearch;
            memset(sendToPacket.data, '\0', 100);
            memset(tcp_ip_addr, '\0',sizeof(tcp_ip_addr));
            memset(tcp_port, '\0', sizeof(tcp_port));
            strcpy(sendToPacket.data,contentTemporaryInfo.peerName);
					  snprintf (tcp_ip_addr, sizeof(tcp_ip_addr), "%d", contentTemporaryInfo.host);
		        snprintf (tcp_port, sizeof(tcp_port), "%d",contentTemporaryInfo.port);
		        memcpy(sendToPacket.data+10, tcp_ip_addr, 5);
		        memcpy(sendToPacket.data+15, tcp_port, 6);
          }
          else {
            // Send Error Message
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', 100);
					  strcpy(sendToPacket.data,"Content Does Not Exist");
          }
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
       }
        /* Peer Server Content List Request */
        else if (recPacket.type == contentList) {
          fprintf(stderr, "packet O received \n");
          // We don't care about the data in this one
          memset(sendToPacket.data, '\0', 100);
          
i = 0;
while (i < headRegister) {
    memcpy(sendToPacket.data + i * 10, registeredFiles[i], 10);
    fprintf(stderr, "Content on Index Server: %s\n", sendToPacket.data + i * 10);
    i++;
}
          sendToPacket.type = contentList;
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
        }
        /* Peer Server Content De-registration */
         else if (recPacket.type == contentDeregister) {
          // Localize Content
          memset(&packetDeregisterT,'\0', sizeof(packetDeregisterT));
i = 0;
while (i < 10) {
    packetDeregisterT.peerName[i] = recPacket.data[i]; // 0:9
    packetDeregisterT.contentName[i] = recPacket.data[i + 10]; // 10:19
    i++;
}
          // Remove the Content
          commonMatch = false;
i = 0;
while (i < tailRegister) {
    if ((strcmp(packetDeregisterT.peerName, contentInfos[i].peerName) == 0) && strcmp(packetDeregisterT.contentName, contentInfos[i].contentName) == 0) {
        commonMatch = true;
        fprintf(stderr, "Removing file:  %s %s %d %d\n", contentInfos[i].contentName, contentInfos[i].peerName, contentInfos[i].port, contentInfos[i].host);
    }
    // If content is found, shift all content down queue
    if (commonMatch && i < tailRegister - 1) {
        strcpy(contentInfos[i].contentName, contentInfos[i + 1].contentName);
        strcpy(contentInfos[i].peerName, contentInfos[i + 1].peerName);
        contentInfos[i].host = contentInfos[i + 1].host;
        contentInfos[i].port = contentInfos[i + 1].port;
    }
    i++;
}


          if (commonMatch) {
            distinctContentName = true; 
            // Check if distinctContentName, if so,
            tailRegister = tailRegister -1; 
    i = 0;
while (i < tailRegister) {
    if (strcmp(packetDeregisterT.contentName, contentInfos[i].contentName) == 0) {
        distinctContentName = false;
    }
    i++;
}
            commonMatch = false;
            if (distinctContentName){
i = 0;
while (i < headRegister) {
    if (strcmp(packetDeregisterT.contentName, registeredFiles[i]) == 0) {
        commonMatch = true;
    }
    if (commonMatch && i < headRegister - 1) {
        strcpy(registeredFiles[i], registeredFiles[i + 1]);
    }
    i++;
}
              headRegister = headRegister -1;
            }

            sendToPacket.type = Ack;
            memset(sendToPacket.data, '\0', 10);
            strcpy(sendToPacket.data, packetDeregisterT.peerName);
          }
          else {
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', 10);
            strcpy(sendToPacket.data, "File Removal Error");
          }
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
         }


    }
}

/*
Cases:
- Content Registration (R)
  - Error (E) 
  - Acknowledgement (A)
Search for Content and Associated Content Server (S)
  - (S) Type w/ Address Field
  - (E) Type content not avail
Content Listing (O)
  - (O) Type with list of registered contents
Content De-registration (T)
  - (A) Ack
  - (E) Error
  Note: when quitting, a bunch of T types may be sent
*/