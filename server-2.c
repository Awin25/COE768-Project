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


#define lengthBuffer		101			/* maximum lengths */
#define maxDataSize 100
#define maxContentNameSize 10
#define maxPeerNameSize 10
#define maxSize 10

/* Different options*/
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

//struct for storing info in server
typedef struct  {
  char contentName[maxContentNameSize]; 
  char peerName[maxPeerNameSize];
  int host;
  int port;
} contentInfo;

int main(int argc, char *argv[]) 
{
  struct  pduR packetRegister, *pktR;  //different packets
  struct  pduS packetSearch; 
  struct  pduO packetListO;
  struct  pduT packetDeregisterT;
  struct  pduE packetError;
  struct  pduA packetAck;

  // Info stored in server
  contentInfo contentInfos[maxSize]; 
  contentInfo contentTemporaryInfo;
  char registeredFiles[maxSize][maxSize];
  
  int headRegister = 0;
  int tailRegister = 0;
  
  bool commonMatch = false;
  bool fileIsRegistered = false;
bool  distinctContentName = true;

  // transmission packets
  struct  transmissionPDU recPacket;
  struct  transmissionPDU sendToPacket;
  int     r_host, r_port;
  char tcp_port[6];
	char tcp_ip_addr[5];

  // Socket connection
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
    sin.sin_family = AF_INET;         // IPv4 address
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
while (i < maxSize) {
    packetRegister.peerName[i] = recPacket.data[i];   //first 10 indexes
    packetRegister.contentName[i] = recPacket.data[i + 10]; //next 10 indexes
    if (i < 5) {
        packetRegister.host[i] = recPacket.data[i + 20]; //next 5 indexes
    }
    if (i < 6) {
        packetRegister.port[i] = recPacket.data[i + 25]; //next 6 indexes
    }
    i++;
}
        r_host = atoi(packetRegister.host);
        r_port = atoi(packetRegister.port);

        FlagE = 0;
i = 0;
while (i < tailRegister) { //print change
    fprintf(stderr, "Content Info %s %s %d %d\n", contentInfos[i].contentName, contentInfos[i].peerName, contentInfos[i].port, contentInfos[i].host);
    // Content alreday exists on server done by Peer
    if (strcmp(contentInfos[i].contentName, packetRegister.contentName) == 0 && strcmp(contentInfos[i].peerName, packetRegister.peerName) == 0) {
        FlagE = 1;
        break;
    }
    // peername and host do not match 
    else if (strcmp(contentInfos[i].peerName, packetRegister.peerName) == 0 && !(contentInfos[i].host == r_host)) {
        FlagE = 2;
        break;
    }
    i++;
}

        /*  Send Reply  */
        if (FlagE == 0) {
            // file has not been registered so new content
            fileIsRegistered = false;
i = 0;
while (i < headRegister) {
    if (strcmp(registeredFiles[i], packetRegister.contentName) == 0) {
        fileIsRegistered = true;
        break;
    }
    i++;
}
            // file is not registered we add it to registeredfilelist
            if (!fileIsRegistered) {
                strcpy(registeredFiles[headRegister], packetRegister.contentName);
                headRegister = headRegister + 1;
            }

            // add content to contentinfos list
            memset(&contentInfos[tailRegister], '\0', sizeof(contentInfos[tailRegister])); // Clean Struct
            strcpy(contentInfos[tailRegister].contentName, packetRegister.contentName);
            strcpy(contentInfos[tailRegister].peerName, packetRegister.peerName);
            contentInfos[tailRegister].host = r_host;
            contentInfos[tailRegister].port = r_port;

           fprintf(stderr, "Sending Registration PDU:\n");
            fprintf(stderr, "Peer Name: %s\n", contentInfos[tailRegister].peerName);
            fprintf(stderr, "Content Name: %s\n", contentInfos[tailRegister].contentName);
            fprintf(stderr, "Port %d, Host %d\n", contentInfos[tailRegister].port, contentInfos[tailRegister].host);
            tailRegister = tailRegister += 1; // Increment tailpointer to the next block

            // Send acknowledement packet back
            sendToPacket.type = Ack;
            memset(sendToPacket.data, '\0', maxDataSize);
            strcpy(sendToPacket.data, packetRegister.peerName);
            fprintf(stderr, "ACK\n");
        } else {
            // Send Error Packet
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', maxDataSize);
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
        /* Peer Server Looking to Search for content */
       else if (recPacket.type == contentSearch) {
          //Localize Content
          memset(&packetSearch,'\0', sizeof(packetSearch));
 i = 0;
while (i < maxSize) {
    packetSearch.peerName[i] = recPacket.data[i]; // first 10 index peername
    packetSearch.contentNameOrAddress[i] = recPacket.data[i + 10]; // next 10 index contentnameoradderss
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
        // match found in search now we must send host and port numbers
        commonMatch = true;
    }
    //  content found, shift all content down queue
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
            // content stored at end of queue
            memset(&contentInfos[tailRegister],'\0', sizeof(contentInfos[tailRegister])); // Clean Struct
            strcpy(contentInfos[tailRegister].contentName,contentTemporaryInfo.contentName);
            strcpy(contentInfos[tailRegister].peerName,contentTemporaryInfo.peerName);
            contentInfos[tailRegister].host = contentTemporaryInfo.host;
            contentInfos[tailRegister].port = contentTemporaryInfo.port;
            tailRegister = tailRegister+1;

            // search packet sent with host and port
            sendToPacket.type = contentSearch;
            memset(sendToPacket.data, '\0', maxDataSize);
            memset(tcp_ip_addr, '\0',sizeof(tcp_ip_addr));
            memset(tcp_port, '\0', sizeof(tcp_port));
            strcpy(sendToPacket.data,contentTemporaryInfo.peerName);
					  snprintf (tcp_ip_addr, sizeof(tcp_ip_addr), "%d", contentTemporaryInfo.host);
		        snprintf (tcp_port, sizeof(tcp_port), "%d",contentTemporaryInfo.port);
		        memcpy(sendToPacket.data+10, tcp_ip_addr, 5);
		        memcpy(sendToPacket.data+15, tcp_port, 6);
          }
          else {
            // error message sent 
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', maxDataSize);
					  strcpy(sendToPacket.data,"Content Does Not Exist");
          }
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
       }
        /* list content per peer request */
        else if (recPacket.type == contentList) {
          fprintf(stderr, "packet O received \n");
          // data not set just want to list content
          memset(sendToPacket.data, '\0', maxDataSize);
          
i = 0;
while (i < headRegister) {
    memcpy(sendToPacket.data + i * 10, registeredFiles[i], maxSize);
    fprintf(stderr, "Content on Index Server: %s\n", sendToPacket.data + i * 10);
    i++;
}
          sendToPacket.type = contentList;
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
        }
        /* deregister content */
         else if (recPacket.type == contentDeregister) {
          // localize Content
          memset(&packetDeregisterT,'\0', sizeof(packetDeregisterT));
i = 0;
while (i < maxSize) {
    packetDeregisterT.peerName[i] = recPacket.data[i]; // first 10 index
    packetDeregisterT.contentName[i] = recPacket.data[i + 10]; // last 10 index
    i++;
}
          // content removed
          commonMatch = false;
i = 0;
while (i < tailRegister) {
    if ((strcmp(packetDeregisterT.peerName, contentInfos[i].peerName) == 0) && strcmp(packetDeregisterT.contentName, contentInfos[i].contentName) == 0) {
        commonMatch = true;
        fprintf(stderr, "Removing file:  %s %s %d %d\n", contentInfos[i].contentName, contentInfos[i].peerName, contentInfos[i].port, contentInfos[i].host);
    }
    //  content  found, shift  content down queue
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
            // Check if distinctContentName, 
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
            memset(sendToPacket.data, '\0',maxSize);
            strcpy(sendToPacket.data, packetDeregisterT.peerName);
          }
          else {
            sendToPacket.type = Error;
            memset(sendToPacket.data, '\0', maxSize);
            strcpy(sendToPacket.data, "File Removal Error");
          }
          sendto(s, &sendToPacket, lengthBuffer, 0,(struct sockaddr *)&fsin, sizeof(fsin));
         }


    }
}