#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/time.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>                                                                         
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

// User defined files
#include "ds.h"

#define	BUFLEN      3000     // Maximum number of bytes per packet

// Globals
char peerName[10];                          // Contains the user name, same name per instance of client
char contentNamesArr[5][10];               // Creates an array that represent the name of the local content that has been registered
                                           
char localContentPort[5][6];                // Stores the port number associated with each piece of locally registered content
int contentListSize = 0;                  // Indicates quantity of content entries

// TCP Socket variables
struct sockaddr_in socketArr[5];
int fdArr[5] = {0};                       // indicates quantity of TCP ports that must be listened to
int max_sd = 0;
int activity;                               // Monitors socket's input/output tasks
fd_set  readfds;                            

// Variables relevant to connecting using UDP
char	*host = "localhost";                // Standard UDP host
int		port = 3000;                        // Standard UDP port                
int		s_udp, s_tcp, new_tcp, n, type;	    // socket descriptor and type	
struct 	hostent	*phe;	                    // points to host entry

void addToLocalContent(char contentName[], char port[], int socket, struct sockaddr_in server){
    strcpy(contentNamesArr[contentListSize], contentName);
    strcpy(localContentPort[contentListSize], port);
    fdArr[contentListSize] = socket;
    socketArr[contentListSize] = server;
    if (socket > max_sd){
        max_sd = socket;
    }
    
    // Log statements to highlight contents that were added
    fprintf(stderr, "These new contents were appended to the existing list of contents:\n");
    fprintf(stderr, "  Name of content: %s\n", contentNamesArr[contentListSize]);
    fprintf(stderr, "   Port: %s\n", localContentPort[contentListSize]);
    fprintf(stderr, "   Socket: %d\n", socket);

    contentListSize++;
}

void removeFromLocalContent(char contentName[]){
    // remove desired content from list, which is done by iterating through all the content entries
    int index = 0;
    bool isElement = false;

    // The while loop iterates through the local list of content names to find the desired content name
    fprintf(stderr, "Indicating removal of content below:\n");
    fprintf(stderr, "  Name of Content: %s\n", contentName);
    
    while(index < contentListSize){
        if(strcmp(contentNamesArr[index], contentName) == 0){
            // the requested name was found in the list, proceed to delete element in both name and port lists
            printf("The following content was deleted:\n");
            printf("    Content Name: %s\n", contentNamesArr[index]);
            printf("    Port: %s\n\n", localContentPort[index]);

             // All elements are changed to the terminating (\0) character
            memset(contentNamesArr[index], '\0', sizeof(contentNamesArr[index]));         
            memset(localContentPort[index], '\0', sizeof(localContentPort[index]));  
            fdArr[index] = 0;       
            fprintf(stderr, "Element successfully deleted from list of local content\n");

 // The while loop below translates all elements below the one removed up one to replace the empty index
            while(index < contentListSize - 1){
                strcpy(contentNamesArr[index], contentNamesArr[index+1]);
                strcpy(localContentPort[index], localContentPort[index+1]);
                fdArr[index] = fdArr[index+1];
                socketArr[index] =  socketArr[index+1];
                index = index + 1;
            }
            isElement = true;
            break;
        }
        index++;
    }

    // An error statement is messaged to the user suppose if the desired content was not detected
    if(isElement){
        contentListSize = contentListSize -1;
    }
    else{
        printf("Error, the list of registered content does not contain the desired content name:\n   Name of Content: %s\n\n",contentName);
    }
}

void printTasks(){
    printf("Please select a task:\n");
    printf("    R: Register content to the index server\n");
    printf("    T: De-register content\n");
    printf("    D: Download content\n");
    printf("    O: List all the content available on the index server\n");
    printf("    L: List all local content registered\n");
    printf("    Q: To de-register all local content from the server and abortApp\n");
}

void registerContent(char contentName[]){
    struct  pduR    packetR;                // The PDU packet object to be received by the index server
    struct  pdu     sendPacket;

    char    writeMsg[101];                  // Temporary array for storing message to be sent to index server
    int     readLength;                     // Size of input bytes received from the index server
    char    incomingData[101];                // Temporary array for storing message received from index server
    
    //Check whether the content file is present in the local space
    if(access(contentName, F_OK) != 0){
        // File does not exist
        printf("Error: File %s does not exist locally\n", contentName);
        return;
    }

   
 //Variables used to establish TCP Connection      
    struct 	sockaddr_in server, client;
    struct  pdu     incomingPacket;
    char    sendContent[sizeof(struct pduC)];
    char    tcp_host[5];
    char    tcp_port[6];             // The TCP host and port in the form of more convenient variables
    int     alen;
    int     bytesRead;
    int     m;

    // Establish stream socket for TCP
	if ((s_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Can't create a TCP socket\n");
		exit(1);
	}
    bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(s_tcp, (struct sockaddr *)&server, sizeof(server)); 
    alen = sizeof(struct sockaddr_in);
    getsockname(s_tcp, (struct sockaddr *)&server, &alen);

    snprintf(tcp_host, sizeof(tcp_host), "%d", server.sin_addr.s_addr);
    snprintf(tcp_port, sizeof(tcp_port), "%d", htons(server.sin_port));

    fprintf(stderr, "A TCP socket was successfully created\n");
	fprintf(stderr, "TCP socket address %s\n", tcp_host);
	fprintf(stderr, "TCP socket port %s\n", tcp_port);

    // Create R-type PDU to be sent to the index server
    memset(&packetR, '\0', sizeof(packetR));  // Terminating character assigned to all elements in packet
    memcpy(packetR.contentName, contentName, 10);
    memcpy(packetR.peerName, peerName, 10);
    memcpy(packetR.host, tcp_host, sizeof(tcp_host));
    memcpy(packetR.port, tcp_port, sizeof(tcp_port));
    packetR.type = 'R';

    fprintf(stderr, "Constructed the R-PDU packet below:\n");
    fprintf(stderr, "    Packet Type: %c\n", packetR.type);
    fprintf(stderr, "    Name of Peer: %s\n", packetR.peerName);
    fprintf(stderr, "    Name of Content: %s\n", packetR.contentName);
    fprintf(stderr, "    Host: %s\n", packetR.host);
    fprintf(stderr, "    Port: %s\n", packetR.port);
    fprintf(stderr, "\n");

    // Parse the pduR type into default PDU type for transmission
    // sendPacket.data = [peerName]+[contentName]+[host]+[port]
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements
    int dataOffset = 0;

    sendPacket.type = packetR.type;
    memcpy(sendPacket.data + dataOffset, 
            packetR.peerName, 
            sizeof(packetR.peerName));
    dataOffset += sizeof(packetR.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetR.contentName,
            sizeof(packetR.contentName));
    dataOffset += sizeof(packetR.contentName);
    memcpy(sendPacket.data + dataOffset, 
            packetR.host,
            sizeof(packetR.host));
    dataOffset += sizeof(packetR.host);
    memcpy(sendPacket.data + dataOffset, 
            packetR.port,
            sizeof(packetR.port));

    fprintf(stderr, "Parsed the R type PDU into the following general PDU:\n");
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
     m = 0;
    while(m <= sizeof(sendPacket.data)-1){
        fprintf(stderr, "%d: %c\n", m, sendPacket.data[m]);
        m++;
    }
    fprintf(stderr, "\n");

    // Built data packet is transmitted to the index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));     
    
    // From incoming message from the server, check the packet's first byte to determine the PDU type (A or E)
    readLength = read(s_udp, incomingData, BUFLEN);

    // Extra log statements to indicate the packets that were transmitted by the index server
    int i = 0;
    fprintf(stderr, "The below packet was received from the index server:\n");
    while(incomingData[i] != '\0'){ 
        fprintf(stderr, "%d: %c\n", i, incomingData[i]);
        i++;
    }

    i = 1;
    struct pduE errorPacket;                    // Possible types of replies from the index server
    struct pduA packetA;
    switch(incomingData[0]){       
        case 'E':
            // Copies incoming packet into a PDU-E structure
            errorPacket.type = incomingData[0];
            while(incomingData[i] != '\0'){ 
                errorPacket.errMsg[i-1] = incomingData[i];
                i++;
            }

            // Output to user
            printf("Error encountered when registering content:\n");
            printf("    %s\n\n", errorPacket.errMsg);
            break;
        case 'A':
            // PDU-A structure stores incoming packet
            packetA.type = incomingData[0];
            while(incomingData[i] != '\0'){ 
                packetA.peerName[i-1] = incomingData[i];
                i++;
            }

            // Output to user
            printf("The below content has been successfully registered:\n");
            printf("   Name of Peer: %s\n", packetA.peerName);
            printf("    Name of Content: %s\n", packetR.contentName);
            printf("    Host: %s\n", packetR.host);
            printf("    Port: %s\n\n", packetR.port);

            listen(s_tcp, 5);
            switch(fork()){
                case 0: // Child
                    // Listens and uploads file to all connecting clients
                    fprintf(stderr, "Content server listening for incoming connections\n");
                    while(1){
                        int client_len = sizeof(client);
                        char fileName[10]; //Array to store name of file
                        new_tcp = accept(s_tcp, (struct sockaddr *)&client, &client_len);
                        memset(&incomingPacket, '\0', sizeof(incomingPacket));
                        memset(&fileName, '\0', sizeof(fileName));
                        bytesRead = read(new_tcp, &incomingPacket, sizeof(incomingPacket));
                        switch(incomingPacket.type){
                            case 'D':
                                fprintf(stderr, "Content server received a D-type packet:\n");
                                m = 0;
                                while(m < sizeof(incomingPacket.data)){
                                    fprintf(stderr, "   %d: %c\n", m, incomingPacket.data[m]);
                                    m++;
                                }
                                fprintf(stderr, "strlen(incomingPacket.data+10) = %d\n", strlen(incomingPacket.data+10));
                                memcpy(fileName, incomingPacket.data+10, strlen(incomingPacket.data+10));
                                //fileName[strlen(incomingPacket.data+10)] =  '\0';
                                fprintf(stderr, "Received the file name from the D data type:\n");
                                fprintf(stderr, "   %s\n", fileName);
                                m = 0;
                                while(m < sizeof(fileName)){
                                    fprintf(stderr, "   %d: %c\n", m, &fileName+m);
                                    m++;
                                }
                                FILE *file;
                                file = fopen(fileName, "rb");
                                if(!file){
                                    fprintf(stderr, "File: %s does not exist locally\n", fileName);
                                }else{
                               fprintf(stderr, "Content server found file locally, getting ready to transmit to client\n");
                                    memset(sendContent, '\0', sizeof(sendContent));
                                    sendContent[0] = 'C';
                                    while(fgets(sendContent+1, sizeof(sendContent)-1, file) > 0){
                                        write(new_tcp, sendContent, sizeof(sendContent));
                                        fprintf(stderr, "Wrote the following to the client:\n %s\n", sendContent);
                                        //memset(sendContent.content, '\0', sizeof(sendContent.content));
                                    }
                                    /*
                                    while(fread(&sendContent.content, 1, sizeof(sendContent.content), file) > 0){
                                        fprintf(stderr, "Transmitting the C-type content data below:\n");
                                        m = 0;
                                        while (m < sizeof(sendContent)){
                                            fprintf(stderr, "   %d: %c\n", m, &sendContent+m);
                                             m = m + 1 
                                        }
                                        write(new_tcp, &sendContent, sizeof(sendContent));
                                        memset(sendContent.content, '\0', sizeof(sendContent.content));
                                    }
                                    */
                                }
                                fprintf(stderr, "Closing content server\n");
                                fclose(file);
                                close(new_tcp);
                                break;
                            default:
                                fprintf(stderr, "Content server encountered  a packet of unknown type.\n");
                                break;
                        }
                    }
                    fprintf(stderr, "Content server escaped the while loop for some reason\n");
                    break;
                case -1: // Parent
                    fprintf(stderr, "Error creating child process");
                    break;
            }

            addToLocalContent(packetR.contentName, packetR.port, s_tcp, server);
            break;
        default:
            printf("Unable to read incoming message transmitted from server\n\n");
    }
}

void deregisterContent(char contentName[], int q){
    struct pduT packetT;
    struct pdu sendPacket;
    struct pdu incomingData;

    //  Build the T type PDU
    memset(&packetT, '\0', sizeof(packetT));          // Initialize all elements with terminating character
    packetT.type = 'T';
    memcpy(packetT.peerName, peerName, sizeof(packetT.peerName));
    memcpy(packetT.contentName, contentName, sizeof(packetT.contentName));

    // Parse the T type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Initialize all elements with terminating character
    int dataOffset = 0;

    sendPacket.type = packetT.type;
    memcpy(sendPacket.data + dataOffset, 
            packetT.peerName, 
            sizeof(packetT.peerName));
    dataOffset += sizeof(packetT.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetT.contentName,
            sizeof(packetT.contentName));
    
    // Log statements for T-type packet parsing
    fprintf(stderr, "Parsed the T type PDU into the following general PDU:\n");
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
    int m;

    // Sends the data packet to the index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
    // Removes the content from the list of locally registered content
    if(!q){
        removeFromLocalContent(packetT.contentName);
    }
    read(s_udp, &incomingData, sizeof(incomingData));
}

void listLocalContent(){
    int j;

    printf("List of names of the locally registered content:\n");
    printf("Number\t\tName\t\tPort\t\tSocket\n");
    j = 0;
    while(j < contentListSize){
        printf("%d\t\t%s\t\t%s\t\t%d\n", j, contentNamesArr[j], localContentPort[j], fdArr[j]);
        j++;
    }
    printf("\n");
}

void listIndexServerContent(){
    struct pduO sendPacket; //Initializing packet to that indicates to list available content
    char        incomingData[sizeof(struct pduO)];
    
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements
    sendPacket.type = 'O';

    // Send request to index server
    write(s_udp, &sendPacket, sizeof(sendPacket));

    // Read and output content names
    printf("The index server contains the following contents:\n");
    int i;

    read(s_udp, incomingData, sizeof(incomingData));
    i = 1;
    while(i < sizeof(incomingData)){
        if(incomingData[i] != '\0'){
            printf("    %d: %.10s\n", i/10, incomingData+i);
        }else{
            break;
        }
        i = i + 10;
    }
    printf("\n");
}

void requestContent(char contentName[]){
    struct pduS sTypePacket;
    struct pdu sendPacket;

    // Build the S type packet to send to the index server
    memset(&sTypePacket, '\0', sizeof(sTypePacket));          
    sTypePacket.type = 'S';
    memcpy(sTypePacket.peerName, peerName, sizeof(sTypePacket.peerName));
    memcpy(sTypePacket.contentNameOrAddress, contentName, strlen(contentName));

    // Parse the S type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          
    int dataOffset = 0;

    sendPacket.type = sTypePacket.type;
    memcpy(sendPacket.data + dataOffset, 
            sTypePacket.peerName, 
            sizeof(sTypePacket.peerName));
    dataOffset += sizeof(sTypePacket.peerName);
    memcpy(sendPacket.data + dataOffset, 
            sTypePacket.contentNameOrAddress,
            sizeof(sTypePacket.contentNameOrAddress));

    // Log statements for S-type packets that were parsed into the standard PDU structure
    fprintf(stderr, "S type PDU was parsed into the general PDU below:\n");
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
    int ind = 0;
    while( ind <= sizeof(sendPacket.data)-1){
        fprintf(stderr, "%d: %c\n", ind, sendPacket.data[ind]);
        ind++;
    }
    fprintf(stderr, "\n");

    //Requested packet is transmitted to index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
}

void downloadContent(char contentName[], char address[]){  
  
    // Variables used for TCP connection
    struct 	sockaddr_in server;
    struct	hostent		*hp;
    char    *serverHost = "0";  // The address of the peer that we'll download from  
    char    serverPortStr[6];           // Intermediate variable that will be used to convert a string to an integer    
    int     serverPort;     
    int     downloadSocket;    
    int     indexPos = 0;                          

    fprintf(stderr, "Found required content on index server, getting ready for download\n");
    while(indexPos < 20){
        fprintf(stderr, "   %d: %c\n", indexPos, address[indexPos]);
        indexPos++;
    }

    // Parsing the address into host and port
    //memcpy(serverHost, address, sizeof(serverHost));
    indexPos = 5;
    while(indexPos < 10){
        serverPortStr[indexPos-5] = address[indexPos];
        fprintf(stderr, "serverPortStr[%d] = %c\n", indexPos - 5, serverPortStr[indexPos-5]);
        indexPos++;
    }
    //memcpy(serverPortStr, address+4, sizeof(serverPortStr));
    serverPort = atoi(serverPortStr);
    printf("Attempting to download content from the following address:\n");
    printf("   Host: %s\n", serverHost);
    printf("   Port: %d\n", serverPort);
    
    // Create a TCP stream socket	
	if ((downloadSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Can't create a TCP socket\n");
		exit(1);
	}

    bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(serverPort);
	if (hp = gethostbyname(serverHost)) 
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(serverHost, (struct in_addr *) &server.sin_addr) ){
        fprintf(stderr, "Can't get server's address\n");
	}

	// Connecting to the server 
	if (connect(downloadSocket, (struct sockaddr *)&server, sizeof(server)) == -1){
        fprintf(stderr, "Was not able to connect to server: %s\n", hp->h_name);
        return;
	}
    fprintf(stderr, "Successfully connected to server at address: %s:%d\n", serverHost, serverPort);

    // Variables used to create and transmit a packet to the content server        
    struct  pduD    packetD;
    struct  pdu     sendPacket; 

    // Construct a D-PDU to send to the content server
    memset(&packetD, '\0', sizeof(packetD));          // Terminating character assigned to all elements
    packetD.type = 'D';
    memcpy(packetD.peerName, peerName, 10);
    memcpy(packetD.content, contentName, 90);

    // Parse the D-PDU type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          
    int dataOffset = 0;

    sendPacket.type = packetD.type;
    memcpy(sendPacket.data + dataOffset, 
            packetD.peerName, 
            sizeof(packetD.peerName));
    dataOffset += sizeof(packetD.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetD.content,
            sizeof(packetD.content));

    // Log statements to indicate the parsing of D-type PDU into the standard structure
    fprintf(stderr, "Parsed the D type PDU into the following general PDU to send to the content server:\n");
    fprintf(stderr, "    Server: %s:%d\n", serverHost, serverPort);
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");

    indexPos = 0;
    while(indexPos <= sizeof(sendPacket.data)-1){
        fprintf(stderr, "%d: %c\n", indexPos, sendPacket.data[indexPos]);
        indexPos++;
    }

    // Send request to content server
    write(downloadSocket, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
    
    // Variables used in file downloading scheme
    FILE    *fp;
    struct  pduC     bufferToBeRead; 
    int     contentResponse = 0;

    // Download the data
    fp = fopen(contentName, "w+");
    memset(&bufferToBeRead, '\0', sizeof(bufferToBeRead)); // Clearing bufferToBeRead  
    while ((indexPos = read(downloadSocket, (struct pdu*)&bufferToBeRead, sizeof(bufferToBeRead))) > 0){
        fprintf(stderr, "The data below was received from the content server\n");
        fprintf(stderr, "   Type: %c\n", bufferToBeRead.type);
        fprintf(stderr, "   Data: %s\n", bufferToBeRead.content);

       if(bufferToBeRead.type != 'C' && bufferToBeRead.type != 'E'){
            fprintf(stderr, "Garbage received from the content server, and will be discarded\n");
        }
         else if(bufferToBeRead.type == 'C'){
            fprintf(stderr, "Successfully received a C type packet from the content server, commencing download\n");
            fprintf(fp, "%s", bufferToBeRead.content);      // Write info from content server to local file
            contentResponse = 1;
        }
        else{
            printf("Error parsing C PDU data:\n");
        }
        memset(&bufferToBeRead, '\0', sizeof(bufferToBeRead)); // Clearing bufferToBeRead 
    }
    fclose(fp);
    if(contentResponse){
        printf("The content was downloaded successfully:\n   Name of content: %s\n", contentName);
        registerContent(contentName);
    }
}

int fileUpload(int sd){
     int         bytesRead;
    struct  pdu incomingData;

    bytesRead = read(sd, (struct pdu*)&incomingData, sizeof(incomingData));
    fprintf(stderr, "Currently handling socket %d\n", sd);
    fprintf(stderr, "   Read in %d bytes of data\n", bytesRead);
    fprintf(stderr, "   Type: %c\n", incomingData.type);
    fprintf(stderr, "   Data: %s\n", incomingData.data);

    return 0;
}

int main(int argc, char **argv){
    
    // Regards various input arguments and assigns host:port connection to server
    switch (argc) {
		case 1:
			break;
		case 2:
			host = argv[1];
            break;
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "usage: UDP Connection to Index Server [host] [port]\n");
			exit(1);
	}

    struct 	sockaddr_in sin;            

    memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                                                                
	sin.sin_port = htons(port);

    // Establish UDP Connection to the index server
    // Enable mapping from the host name to IP address
	if ( phe = gethostbyname(host) ){
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");                                                               
    
    // Socket is allocated
	s_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_udp < 0)
		fprintf(stderr, "Can't create socket \n");       

    // The socket is connected
	if (connect(s_udp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s\n", host);
	}
    
    printf("Peer to Peer Network\n");
    
    
    printf("Enter your username:\n");
    while(read (0, peerName, sizeof(peerName)) > 10){
        printf("User name exceeded maximum length. Please ensure that username is 9 characters or less:\n");
    }
    
    // Change last char to a null termination instead of newline, for purely aesthetic formatting purposes only
    peerName[strlen(peerName)-1] = '\0'; 
    printf("Welcome %s! ", peerName);

    // Main control loop
    char            taskOption[2];          // Indicates the task that the user wants to execute
     int             j;                      // Used for indexing for loop procedures
     char            incomingData[101];        // Buffer for incoming messages from the index server  
     int             abortApp = 0;               // Flag that is enabled when the user wants to quit the app
    char            extraInfo[10];          // Input from user that serves as extra information regarding task completion          
    struct pduE     errorPacket;                // For parsing Error messages
    struct pduS     sTypePacket;                // For parsing incoming S type PDUs   

    while(!abortApp){
        printTasks(); 
        read(0, taskOption, 2);
        memset(extraInfo, 0, sizeof(extraInfo));
        // Perform task
        switch(taskOption[0]){
            case 'R':   // Register content to the index server
                printf("Enter a valid content name that can have a maximum length of 9 characters:\n");
                scanf("%9s", extraInfo);      
                registerContent(extraInfo);
                break;
            case 'T':   // De-register content
                printf("Enter the name of the content you would like to de-register:\n");
                scanf("%9s", extraInfo);   
                deregisterContent(extraInfo,0);
                break;
            case 'D':   // Download content
                printf("Please input the content you wish to download:\n");
                scanf("%9s", extraInfo);

                requestContent(extraInfo);                // Ask the index for a specific piece of content
                
                // Read index answer, either an S or E type PDU
                read(s_udp, incomingData, sizeof(incomingData));
                
                // Indicates what messages were transmitted by the index server
                fprintf(stderr, "Received incoming message from the index server:\n");
                j = 0;
                while(j <= sizeof(incomingData)-1){
                    fprintf(stderr, "%d: %c\n", j, incomingData[j]);
                    j++;
                }

                switch(incomingData[0]){
                    case 'E':
                        // input packet passed into a PDU-E struct
                        j = 1;
                        errorPacket.type = incomingData[0];
                        while(incomingData[j] != '\0'){ 
                            errorPacket.errMsg[j-1] = incomingData[j];
                            j++;
                        }
                        // Output to user
                        printf("Error downloading content:\n");
                        printf("    %s\n\n", errorPacket.errMsg);
                        break;
                    case 'S':
                        // input packet passed into a PDU-S struct
                        sTypePacket.type = incomingData[0];
                        j = 0;
                        while(j < sizeof(incomingData)){
                            if (j < 10){
                                //covers index scenarios from 1 to 10
                                sTypePacket.peerName[j] = incomingData[j+1]; 
                            }
                            sTypePacket.contentNameOrAddress[j] = incomingData[j+11]; //covers index scenarios from 11 to 100
                            j++;
                        }

                        // Indicate messages that were converted into S-type PDU
                        fprintf(stderr, "Incoming message was parsed into the following S-PDU structure:\n");
                        fprintf(stderr, "   Type: %c\n", sTypePacket.type);
                        fprintf(stderr, "   Name of Peer: %s\n", sTypePacket.peerName);
                        fprintf(stderr, "   Address: %s\n", sTypePacket.contentNameOrAddress);

                        // Handles peer's download request with address of s-type packet (sTypePacket.contentNameOrAddress) with content name (extraInfo variable)
                        downloadContent(extraInfo, sTypePacket.contentNameOrAddress);
                        break;
                }
                break;
            case 'O':   //Return all the content available on the index server
                listIndexServerContent();
                break;
            case 'L':   // List all local content registered
                listLocalContent();
                break;
            case 'Q':   // For all local content, perform de-registration and abortApp
            j = 0;
                while(j < contentListSize){
                    deregisterContent(contentNamesArr[j], 1);
                    j++;
                }
                abortApp = 1;
                break;
            default:
                printf("This is an invalid option. Please try again.\n");
        } 
    }
    close(s_udp);
    close(s_tcp);
    exit(0);
}
