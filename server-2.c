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

[Content] (unique)

*/


#define BUFLEN		101			/* buffer length */

struct tpacket {
  char type;
  char data[100];
};

// Index System Data Structures
typedef struct  {
  char contentName[10]; 
  char peerName[10];
  int host;
  int port;
} contentListing;

int main(int argc, char *argv[]) 
{
  // Packets
  struct  pduR packetR, *pktR;
  struct  pduS packetS;
  struct  pduO packetO;
  struct  pduT packetT;
  struct  pduE packetE;
  struct  pduA packetA;

  // Index System
  contentListing contentList[10];
  contentListing tempContentBlock;
  char lsList[10][10];
  
  int lsPointer = 0;
  int endPointer = 0;
  
  int match = 0;
  int lsmatch = 0;
  int unique = 1;

  // Transmission Variables
  struct  tpacket packetrecieve;
  struct  tpacket packetsend;
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
  int     s,sz, type;               /* socket descriptor and socket type */
	int 	  port = 3000;              /* default port */
	int 	  counter, n,m,i,bytes_to_read,bufsize;
  int     errorFlag = 0;


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
    
    while(1) {
      if (recvfrom(s, &packetrecieve, BUFLEN, 0,(struct sockaddr *)&fsin, &alen) < 0)
			  fprintf(stderr, "[ERROR] recvfrom error\n");
      fprintf(stderr, "Msg Recieved\n");
      switch(packetrecieve.type){
        /* Peer Server Content Registration */
        case 'R':
          memset(&packetR,'\0', sizeof(packetR));
          for(i=0;i<10;i++){
              packetR.peerName[i] = packetrecieve.data[i];        // 0:9
              packetR.contentName[i] = packetrecieve.data[i+10];  // 10:19
              if (i<5) {
                packetR.host[i] = packetrecieve.data[i+20];       // 20:25
              }
              if (i<6) {
                packetR.port[i] = packetrecieve.data[i+25];       // 26:31
              }
          }
          r_host = atoi(packetR.host);
          r_port = atoi(packetR.port);
         
         errorFlag=0; 
         for(i=0;i<endPointer;i++){
           fprintf(stderr, "ARRAY ELEMENT %s %s %d %d\n", contentList[i].contentName, contentList[i].peerName,contentList[i].port, contentList[i].host);
           // Content has already been uploaded by Peer
           if (strcmp(contentList[i].contentName, packetR.contentName) == 0 && strcmp(contentList[i].peerName, packetR.peerName) == 0) {
             errorFlag=1;
             break;
           }
           // PeerName and Address Mismatch
           else if (strcmp(contentList[i].peerName, packetR.peerName)==0 && !(contentList[i].host == r_host)) {
             errorFlag=2;
             break;
           }
         }

          /*  Send Reply  */
          if (errorFlag==0){

            // New Content? 
            lsmatch=0;
            for(i=0;i<lsPointer;i++){
              if (strcmp(lsList[i], packetR.contentName) == 0){
                lsmatch = lsmatch+1;
                break;
              }
            }
            // If So, Commit Content ls List
            if (!lsmatch) {
              strcpy(lsList[lsPointer],packetR.contentName);
              lsPointer = lsPointer + 1;
            }

            // Commit Content to contentList
            memset(&contentList[endPointer],'\0', sizeof(contentList[endPointer])); // Clean Struct
            strcpy(contentList[endPointer].contentName,packetR.contentName);
            strcpy(contentList[endPointer].peerName,packetR.peerName);
            contentList[endPointer].host = r_host;
            contentList[endPointer].port = r_port;

            fprintf(stderr, "Peer Name: %s\n", contentList[endPointer].contentName);
            fprintf(stderr, "Content Name: %s\n", contentList[endPointer].peerName);
            fprintf(stderr, "Port %d, Host %d\n", contentList[endPointer].port, contentList[endPointer].host);
            endPointer = endPointer +=1; // Increment pointer to NEXT block
            
            // Send Ack Packet
            packetsend.type = 'A';
            memset(packetsend.data, '\0', 100);
            strcpy(packetsend.data, packetR.peerName);
            fprintf(stderr, "Acked\n"); 
          }
          else {
            // Send Err Packet
            packetsend.type = 'E';
            memset(packetsend.data, '\0', 100);
            if (errorFlag==2) {
					    strcpy(packetsend.data,"PeerName has already been registered");
            }
            else if (errorFlag==1) {
              strcpy(packetsend.data,"File Already Registered Error");
            }
            fprintf(stderr,"Error\n");
          }
          sendto(s, &packetsend, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          for(i=0;i<lsPointer;i++){
              fprintf(stderr, "LS LIST %s\n", lsList[i]);
          }
          break;
        
        /* Peer Server Content Location Request */
        case 'S':
          //Localize Content
          memset(&packetS,'\0', sizeof(packetS));
          for(i=0;i<10;i++){
              packetS.peerName[i] = packetrecieve.data[i];        // 0:9
              packetS.contentNameOrAddress[i] = packetrecieve.data[i+10];  // 10:19
          }
          match = 0;
          for(i=0;i<endPointer;i++){
            if (strcmp(packetS.contentNameOrAddress, contentList[i].contentName) == 0){
              fprintf(stderr, "S MATCH %s %s %d %d\n", contentList[i].contentName, contentList[i].peerName,contentList[i].port, contentList[i].host);
              memset(&tempContentBlock,'\0', sizeof(tempContentBlock));
              strcpy(tempContentBlock.contentName, contentList[i].contentName);
              strcpy(tempContentBlock.peerName, contentList[i].peerName);
              tempContentBlock.host = contentList[i].host;
              tempContentBlock.port = contentList[i].port;
              // Have to send host and port here according to the matched content addr.
              match=1; 
            }
            // If content is found, shift all content down queue
            if (match && i < endPointer-1){
              strcpy(contentList[i].contentName,contentList[i+1].contentName);
              strcpy(contentList[i].peerName, contentList[i+1].peerName);
              contentList[i].host = contentList[i+1].host;
              contentList[i].port = contentList[i+1].port;
            }
          }

          if (match) {
            endPointer = endPointer-1;
            // Commit Content at EOQ
            memset(&contentList[endPointer],'\0', sizeof(contentList[endPointer])); // Clean Struct
            strcpy(contentList[endPointer].contentName,tempContentBlock.contentName);
            strcpy(contentList[endPointer].peerName,tempContentBlock.peerName);
            contentList[endPointer].host = tempContentBlock.host;
            contentList[endPointer].port = tempContentBlock.port;
            endPointer = endPointer+1;

            // TODO: Send S Packet with host and port
            packetsend.type = 'S';
            memset(packetsend.data, '\0', 100);
            memset(tcp_ip_addr, '\0',sizeof(tcp_ip_addr));
            memset(tcp_port, '\0', sizeof(tcp_port));
            strcpy(packetsend.data,tempContentBlock.peerName);
					  snprintf (tcp_ip_addr, sizeof(tcp_ip_addr), "%d", tempContentBlock.host);
		        snprintf (tcp_port, sizeof(tcp_port), "%d",tempContentBlock.port);
		        memcpy(packetsend.data+10, tcp_ip_addr, 5);
		        memcpy(packetsend.data+15, tcp_port, 6);
          }
          else {
            // Send Error Message
            packetsend.type = 'E';
            memset(packetsend.data, '\0', 100);
					  strcpy(packetsend.data,"Content Not Found");
          }
          sendto(s, &packetsend, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;
        
        /* Peer Server Content List Request */
        case 'O':
          fprintf(stderr, "O Type Packets \n");
          // We don't care about the data in this one
          memset(packetsend.data, '\0', 100);
          
          for(i=0;i<lsPointer;i++){
            memcpy(packetsend.data+i*10, lsList[i], 10);
            fprintf(stderr, "Here's an item: %s\n", packetsend.data+i*10);
          }
          packetsend.type = 'O';
          sendto(s, &packetsend, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;
        
        /* Peer Server Content De-registration */
        case 'T':
          // Localize Content
          memset(&packetT,'\0', sizeof(packetT));
          for(i=0;i<10;i++){
              packetT.peerName[i] = packetrecieve.data[i];        // 0:9
              packetT.contentName[i] = packetrecieve.data[i+10];  // 10:19
          }
          // Remove the Content
          match = 0;
          for(i=0;i<endPointer;i++){
            if ((strcmp(packetT.peerName, contentList[i].peerName) == 0) && strcmp(packetT.contentName, contentList[i].contentName)==0){
              match=1;
              fprintf(stderr, "File getting removed:  %s %s %d %d\n", contentList[i].contentName, contentList[i].peerName,contentList[i].port, contentList[i].host);
            }
            // If content is found, shift all content down queue
            if (match && i < endPointer-1){
              strcpy(contentList[i].contentName,contentList[i+1].contentName);
              strcpy(contentList[i].peerName, contentList[i+1].peerName);
              contentList[i].host = contentList[i+1].host;
              contentList[i].port = contentList[i+1].port;
            }
          }


          if (match) {
            unique = 1; 
            // Check if unique, if so,
            endPointer = endPointer -1; 
            for(i=0;i<endPointer;i++){
              if (strcmp(packetT.contentName, contentList[i].contentName)==0){
                unique = 0;
              }
            }
            match = 0;
            if (unique){
              for(i=0;i<lsPointer;i++){
                if (strcmp(packetT.contentName, lsList[i])==0) {
                  match=1;
                }
                if (match && i < lsPointer-1){
                  strcpy(lsList[i],lsList[i+1]);
                }
              }
              lsPointer = lsPointer -1;
            }

            packetsend.type = 'A';
            memset(packetsend.data, '\0', 10);
            strcpy(packetsend.data, packetT.peerName);
          }
          else {
            packetsend.type = 'E';
            memset(packetsend.data, '\0', 10);
            strcpy(packetsend.data, "File Removal Error");
          }
          sendto(s, &packetsend, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;

        default:
          break;
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