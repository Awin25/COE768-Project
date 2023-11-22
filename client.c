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

// User defined files
#include "ds.h"

#define	BUFLEN      3000     // Max bytes per packet

// Globals
char peerName[10];                          // Holds the user name, same name per instance of client
char localContentName[5][10];               // Creates an array of strings that represent the name of the local content that has been registered
                                            // via the index server. Can store 5 elements of 10 bytes per content (9 + null terminating char)
char localContentPort[5][6];                // Stores the port number associated with each piece of locally registered content
int numOfLocalContent = 0;                  // Stores number of saved local content

// TCP Socket variables
int fdArray[5] = {0};                       // 5 TCP ports to listen to
struct sockaddr_in socketArray[5];
int max_sd = 0;
int activity;                               // Tracks I/O activity on sockets
fd_set  readfds;                            // List of socket descriptors

// UDP connection variables
char	*host = "localhost";                // Default UDP host
int		port = 3000;                        // Default UDP port                
int		s_udp, s_tcp, new_tcp, n, type;	    // socket descriptor and socket type	
struct 	hostent	*phe;	                    // pointer to host information entry	

void addToLocalContent(char contentName[], char port[], int socket, struct sockaddr_in server){
    strcpy(localContentName[numOfLocalContent], contentName);
    strcpy(localContentPort[numOfLocalContent], port);
    fdArray[numOfLocalContent] = socket;
    socketArray[numOfLocalContent] = server;
    if (socket > max_sd){
        max_sd = socket;
    }
    
    // stderr Logging purposes only
    fprintf(stderr, "Added the following content to the local list of contents:\n");
    fprintf(stderr, "   Content Name: %s\n", localContentName[numOfLocalContent]);
    fprintf(stderr, "   Port: %s\n", localContentPort[numOfLocalContent]);
    fprintf(stderr, "   Socket: %d\n", socket);

    numOfLocalContent++;
}

void removeFromLocalContent(char contentName[]){
    // TODO: Find in array list and remove from both lists, decrement numOfLocalContent
    int j;
    int foundElement = 0;

    // This for loop parses the local list of content names for the requested content name
    fprintf(stderr, "Attempting to remove the following content:\n");
    fprintf(stderr, "   Content Name: %s\n", contentName);
    for(j = 0; j < numOfLocalContent; j++){
        if(strcmp(localContentName[j], contentName) == 0){
            // the requested name was found in the list, proceed to delete element in both name and port lists
            printf("The following content was deleted:\n");
            printf("    Content Name: %s\n", localContentName[j]);
            printf("    Port: %s\n\n", localContentPort[j]);

             // Sets terminating characters to all elements
            memset(localContentName[j], '\0', sizeof(localContentName[j]));         
            memset(localContentPort[j], '\0', sizeof(localContentPort[j]));  
            fdArray[j] = 0;       
            fprintf(stderr, "Element successfully deleted\n");

            // This loop moves all elements underneath the one deleted up one to fill the gap
            while(j < numOfLocalContent - 1){
                strcpy(localContentName[j], localContentName[j+1]);
                strcpy(localContentPort[j], localContentPort[j+1]);
                fdArray[j] = fdArray[j+1];
                socketArray[j] =  socketArray[j+1];
                j++;
            }
            foundElement = 1;
            break;
        }
    }

    // Sends an error message to the user if the requested content was not found
    if(!foundElement){
        printf("Error, the requested content name was not found in the list:\n");
        printf("    Content Name: %s\n\n", contentName);
    }else{
        numOfLocalContent = numOfLocalContent -1;
    }
}

void printTasks(){
    printf("Choose a task:\n");
    printf("    R: Register content to the index server\n");
    printf("    T: De-register content\n");
    printf("    D: Download content\n");
    printf("    O: List all the content available on the index server\n");
    printf("    L: List all local content registered\n");
    printf("    Q: To de-register all local content from the server and quit\n");
}

void registerContent(char contentName[]){
    struct  pduR    packetR;                // The PDU packet to send to the index server
    struct  pdu     sendPacket;

    char    writeMsg[101];                  // Temp placeholder for outgoing message to index server
    int     readLength;                     // Length of incoming data bytes from index server
    char    readPacket[101];                // Temp placeholder for incoming message from index server
    
    //Verify that the file exists locally
    if(access(contentName, F_OK) != 0){
        // File does not exist
        printf("Error: File %s does not exist locally\n", contentName);
        return;
    }

    // TCP connection variables        
    struct 	sockaddr_in server, client;
    struct  pdu     incomingPacket;
    char    sendContent[sizeof(struct pduC)];
    char    tcp_host[5];
    char    tcp_port[6];             // The generated TCP host and port into easier to call variables
    int     alen;
    int     bytesRead;
    int     m;

    // Create a TCP stream socket	
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

    fprintf(stderr, "Successfully generated a TCP socket\n");
	fprintf(stderr, "TCP socket address %s\n", tcp_host);
	fprintf(stderr, "TCP socket port %s\n", tcp_port);

    // Build R type PDU to send to index server
    memset(&packetR, '\0', sizeof(packetR));          // Sets terminating characters to all elements
    memcpy(packetR.contentName, contentName, 10);
    memcpy(packetR.peerName, peerName, 10);
    memcpy(packetR.host, tcp_host, sizeof(tcp_host));
    memcpy(packetR.port, tcp_port, sizeof(tcp_port));
    packetR.type = 'R';

    fprintf(stderr, "Built the following R PDU packet:\n");
    fprintf(stderr, "    Type: %c\n", packetR.type);
    fprintf(stderr, "    Peer Name: %s\n", packetR.peerName);
    fprintf(stderr, "    Content Name: %s\n", packetR.contentName);
    fprintf(stderr, "    Host: %s\n", packetR.host);
    fprintf(stderr, "    Port: %s\n", packetR.port);
    fprintf(stderr, "\n");

    // Parse the pduR type into default pdu type for transmission
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
    for(m = 0; m <= sizeof(sendPacket.data)-1; m++){
        fprintf(stderr, "%d: %c\n", m, sendPacket.data[m]);
    }
    fprintf(stderr, "\n");

    // Sends the data packet to the index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));     
    
    // Wait for message from the server and check the first byte of packet to determine the PDU type (A or E)
    readLength = read(s_udp, readPacket, BUFLEN);

    // Logging purposes only
    int i = 0;
    fprintf(stderr, "Received the following packet from the index server:\n");
    while(readPacket[i] != '\0'){ 
        fprintf(stderr, "%d: %c\n", i, readPacket[i]);
        i++;
    }

    i = 1;
    struct pduE packetE;                    // Potential responses from the index server
    struct pduA packetA;
    switch(readPacket[0]){       
        case 'E':
            // Copies incoming packet into a PDU-E struct
            packetE.type = readPacket[0];
            while(readPacket[i] != '\0'){ 
                packetE.errMsg[i-1] = readPacket[i];
                i++;
            }

            // Output to user
            printf("Error registering content:\n");
            printf("    %s\n", packetE.errMsg);
            printf("\n");
            break;
        case 'A':
            // Copies incoming packet into a PDU-A struct
            packetA.type = readPacket[0];
            while(readPacket[i] != '\0'){ 
                packetA.peerName[i-1] = readPacket[i];
                i++;
            }

            // Output to user
            printf("The following content has been successfully registered:\n");
            printf("    Peer Name: %s\n", packetA.peerName);
            printf("    Content Name: %s\n", packetR.contentName);
            printf("    Host: %s\n", packetR.host);
            printf("    Port: %s\n", packetR.port);
            printf("\n");

            listen(s_tcp, 5);
            switch(fork()){
                case 0: // Child
                    // Continuously listens and uploads file to all connecting clients
                    fprintf(stderr, "Content server listening for incoming connections\n");
                    while(1){
                        int client_len = sizeof(client);
                        char fileName[10];
                        new_tcp = accept(s_tcp, (struct sockaddr *)&client, &client_len);
                        memset(&incomingPacket, '\0', sizeof(incomingPacket));
                        memset(&fileName, '\0', sizeof(fileName));
                        bytesRead = read(new_tcp, &incomingPacket, sizeof(incomingPacket));
                        switch(incomingPacket.type){
                            case 'D':
                                fprintf(stderr, "Content server received a D type packet:\n");
                                for(m = 0; m < sizeof(incomingPacket.data); m++){
                                    fprintf(stderr, "   %d: %c\n", m, incomingPacket.data[m]);
                                }
                                fprintf(stderr, "strlen(incomingPacket.data+10) = %d\n", strlen(incomingPacket.data+10));
                                memcpy(fileName, incomingPacket.data+10, strlen(incomingPacket.data+10));
                                //fileName[strlen(incomingPacket.data+10)] =  '\0';
                                fprintf(stderr, "Recieved the file name from the D data type:\n");
                                fprintf(stderr, "   %s\n", fileName);
                                for(m = 0; m < sizeof(fileName); m++){
                                    fprintf(stderr, "   %d: %c\n", m, &fileName+m);
                                }
                                FILE *file;
                                file = fopen(fileName, "rb");
                                if(!file){
                                    fprintf(stderr, "File: %s does not exist locally\n", fileName);
                                }else{
                                    fprintf(stderr, "Content server found file locally, prepping to send to client\n");
                                    memset(sendContent, '\0', sizeof(sendContent));
                                    sendContent[0] = 'C';
                                    while(fgets(sendContent+1, sizeof(sendContent)-1, file) > 0){
                                        write(new_tcp, sendContent, sizeof(sendContent));
                                        fprintf(stderr, "Wrote the following to the client:\n %s\n", sendContent);
                                        //memset(sendContent.content, '\0', sizeof(sendContent.content));
                                    }
                                    /*
                                    while(fread(&sendContent.content, 1, sizeof(sendContent.content), file) > 0){
                                        fprintf(stderr, "Sending the following C type content data:\n");
                                        for(m = 0; m < sizeof(sendContent); m++){
                                            fprintf(stderr, "   %d: %c\n", m, &sendContent+m);
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
                                fprintf(stderr, "Content server recieved an unknown packet\n");
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
            printf("Unable to read incoming message from server\n\n");
    }
}

void deregisterContent(char contentName[], int q){
    struct pduT packetT;
    struct pdu sendPacket;
    struct pdu readPacket;

    //  Build the T type PDU
    memset(&packetT, '\0', sizeof(packetT));          // Sets terminating characters to all elements (initializer)
    packetT.type = 'T';
    memcpy(packetT.peerName, peerName, sizeof(packetT.peerName));
    memcpy(packetT.contentName, contentName, sizeof(packetT.contentName));

    // Parse the T type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements (initializer)
    int dataOffset = 0;

    sendPacket.type = packetT.type;
    memcpy(sendPacket.data + dataOffset, 
            packetT.peerName, 
            sizeof(packetT.peerName));
    dataOffset += sizeof(packetT.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetT.contentName,
            sizeof(packetT.contentName));
    
    // stderr output, log purposes only
    fprintf(stderr, "Parsed the T type PDU into the following general PDU:\n");
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
    int m;
    // for(m = 0; m <= sizeof(sendPacket.data)-1; m++){
    //     fprintf(stderr, "%d: %c\n", m, sendPacket.data[m]);
    // }
    // fprintf(stderr, "\n");

    // Sends the data packet to the index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
    // Removes the content from the list of locally registered content
    if(!q){
        removeFromLocalContent(packetT.contentName);
    }
    read(s_udp, &readPacket, sizeof(readPacket));
}

void listLocalContent(){
    int j;

    printf("List of names of the locally registered content:\n");
    printf("Number\t\tName\t\tPort\t\tSocket\n");
    for(j = 0; j < numOfLocalContent; j++){
        printf("%d\t\t%s\t\t%s\t\t%d\n", j, localContentName[j], localContentPort[j], fdArray[j]);
    }
    printf("\n");
}

void listIndexServerContent(){
    struct pduO sendPacket;
    char        readPacket[sizeof(struct pduO)];
    
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements
    sendPacket.type = 'O';

    // Send request to index server
    write(s_udp, &sendPacket, sizeof(sendPacket));

    // Read and list contents
    printf("The Index Server contains the following:\n");
    int i;

    // Info logging purposes only
    /*
    fprintf(stderr, "Received the following packet from the server: ");
    fprintf(stderr, "%s\n", readPacket);
    fprintf(stderr, "Long form:\n");
    for(i = 0; i < sizeof(readPacket); i++){
        fprintf(stderr, "   %d: %.1s\n", i, readPacket+i);
    }
    */

    read(s_udp, readPacket, sizeof(readPacket));
    for(i = 1; i < sizeof(readPacket); i+=10){
        if(readPacket[i] == '\0'){
            break;
        }else{
            printf("    %d: %.10s\n", i/10, readPacket+i);
        }
    }
    printf("\n");
}

void pingIndexFor(char contentName[]){
    struct pduS packetS;
    struct pdu sendPacket;

    // Build the S type packet to send to the index server
    memset(&packetS, '\0', sizeof(packetS));          // Sets terminating characters to all elements
    packetS.type = 'S';
    memcpy(packetS.peerName, peerName, sizeof(packetS.peerName));
    memcpy(packetS.contentNameOrAddress, contentName, strlen(contentName));

    // Parse the S type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements
    int dataOffset = 0;

    sendPacket.type = packetS.type;
    memcpy(sendPacket.data + dataOffset, 
            packetS.peerName, 
            sizeof(packetS.peerName));
    dataOffset += sizeof(packetS.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetS.contentNameOrAddress,
            sizeof(packetS.contentNameOrAddress));

    // stderr output, log purposes only
    fprintf(stderr, "Parsed the S type PDU into the following general PDU:\n");
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
    int m;
    for(m = 0; m <= sizeof(sendPacket.data)-1; m++){
        fprintf(stderr, "%d: %c\n", m, sendPacket.data[m]);
    }
    fprintf(stderr, "\n");

    // Send request to index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
}

void downloadContent(char contentName[], char address[]){  
    // TODO, parse address parameter into the host and port variables     
    // TCP connection variables
    struct 	sockaddr_in server;
    struct	hostent		*hp;
    char    *serverHost = "0";  // The address of the peer that we'll download from  
    char    serverPortStr[6];           // Temp middle man to conver string to int        
    int     serverPort;     
    int     downloadSocket;    
    int     m;                          // Variable used for iterative processes

    fprintf(stderr, "Found required content on index server, preparing for download\n");
    for(m = 0; m < 20; m++){
        fprintf(stderr, "   %d: %c\n", m, address[m]);
    }

    // Parsing the address into host and port
    //memcpy(serverHost, address, sizeof(serverHost));
    for(m = 5; m<10; m++){
        serverPortStr[m-5] = address[m];
        fprintf(stderr, "serverPortStr[%d] = %c\n", m-5, serverPortStr[m-5]);
    }
    //memcpy(serverPortStr, address+4, sizeof(serverPortStr));
    serverPort = atoi(serverPortStr);
    printf("Trying to download content from the following address:\n");
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
        fprintf(stderr, "Can't connect to server: %s\n", hp->h_name);
        return;
	}
    fprintf(stderr, "Successfully connected to server at address: %s:%d\n", serverHost, serverPort);

    // Variables to construct and send a packet to the content server        
    struct  pduD    packetD;
    struct  pdu     sendPacket; 

    // Construct a D-PDU to send to the content server
    memset(&packetD, '\0', sizeof(packetD));          // Sets terminating characters to all elements
    packetD.type = 'D';
    memcpy(packetD.peerName, peerName, 10);
    memcpy(packetD.content, contentName, 90);

    // Parse the D-PDU type into a general PDU for transmission
    // sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));          // Sets terminating characters to all elements
    int dataOffset = 0;

    sendPacket.type = packetD.type;
    memcpy(sendPacket.data + dataOffset, 
            packetD.peerName, 
            sizeof(packetD.peerName));
    dataOffset += sizeof(packetD.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetD.content,
            sizeof(packetD.content));

    // stderr output, log purposes only
    fprintf(stderr, "Parsed the D type PDU into the following general PDU to send to the content server:\n");
    fprintf(stderr, "    Server: %s:%d\n", serverHost, serverPort);
    fprintf(stderr, "    Type: %c\n", sendPacket.type);
    fprintf(stderr, "    Data:\n");
    for(m = 0; m <= sizeof(sendPacket.data)-1; m++){
        fprintf(stderr, "%d: %c\n", m, sendPacket.data[m]);
    }

    // Send request to content server
    write(downloadSocket, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
    
    // File download variables
    FILE    *fp;
    struct  pduC     readBuffer; 
    int     receivedContent = 0;

    // Download the data
    fp = fopen(contentName, "w+");
    memset(&readBuffer, '\0', sizeof(readBuffer)); // Clearing readBuffer  
    while ((m = read(downloadSocket, (struct pdu*)&readBuffer, sizeof(readBuffer))) > 0){
        fprintf(stderr, "Received the following data from the content server\n");
        fprintf(stderr, "   Type: %c\n", readBuffer.type);
        fprintf(stderr, "   Data: %s\n", readBuffer.content);
        if(readBuffer.type == 'C'){
            fprintf(stderr, "Successfully received a C type packet from the content server, commencing download\n");
            fprintf(fp, "%s", readBuffer.content);      // Write info from content server to local file
            receivedContent = 1;
        }else if(readBuffer.type == 'E'){
            printf("Error parsing C PDU data:\n");
        }else{
            fprintf(stderr, "Received garbage from the content server, discarding\n");
        }
        memset(&readBuffer, '\0', sizeof(readBuffer)); // Clearing readBuffer 
    }
    fclose(fp);
    if(receivedContent){
        printf("Successfully downloaded content:\n");
        printf("   Content Name: %s\n", contentName);
        registerContent(contentName);
    }
}

int uploadFile(int sd){
    struct  pdu readPacket;
    int         bytesRead;

    bytesRead = read(sd, (struct pdu*)&readPacket, sizeof(readPacket));
    fprintf(stderr, "Currently handling socket %d\n", sd);
    fprintf(stderr, "   Read in %d bytes of data\n", bytesRead);
    fprintf(stderr, "   Type: %c\n", readPacket.type);
    fprintf(stderr, "   Data: %s\n", readPacket.data);

    return 0;
}

int main(int argc, char **argv){
    
    // Checks input arguments and assigns host:port connection to server
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

    // Generate a UDP connection to the index server
    // Map host name to IP address, allowing for dotted decimal 
	if ( phe = gethostbyname(host) ){
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");                                                               
    
    // Allocate a socket 
	s_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_udp < 0)
		fprintf(stderr, "Can't create socket \n");       

    // Connect the socket 
	if (connect(s_udp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s\n", host);
	}
    
    printf("=============================\n");
    printf("Welcome to the P2P Network!\n");
	printf("=============================\n");
    
    // TODO: Handle edge case for when username matches another in the server, should be handled by
    // the registration component
    printf("Please enter a username:\n");
    while(read (0, peerName, sizeof(peerName)) > 10){
        printf("User name is too long, please keep it 9 characters or less:\n");
    }
    
    // Change last char to a null termination instead of newline, for purely aesthetic formatting purposes only
    peerName[strlen(peerName)-1] = '\0'; 
    printf("Welcome %s! ", peerName);

    // Main control loop
    char            userChoice[2];          // Typically to store task choice
    char            userInput[10];          // Typically to store additional infor required from the user to complete specific tasks
    char            readPacket[101];        // Buffer for incoming messages from the index server            
    int             quit = 0;               // Flag thats enabled when the user wants to quit the app
    int             j;                      // Used for any basic iterative processes
    struct pduE     packetE;                // Used to parse incoming Error messages
    struct pduS     packetS;                // Used to parse incoming S type PDUs   

    while(!quit){
        printTasks(); 
        read(0, userChoice, 2);
        memset(userInput, 0, sizeof(userInput));
        // Perform task
        switch(userChoice[0]){
            case 'R':   // Register content to the index server
                printf("Enter a valid content name, 9 characters or less:\n");
                scanf("%9s", userInput);      
                registerContent(userInput);
                break;
            case 'T':   // De-register content
                printf("Enter the name of the content you would like to de-register:\n");
                scanf("%9s", userInput);   
                deregisterContent(userInput,0);
                break;
            case 'D':   // Download content
                printf("Enter the name of the content you would like to download:\n");
                scanf("%9s", userInput);

                // Info logging purposes only
                fprintf(stderr, "User wants to download the following content name:\n");
                for(j = 0; j < sizeof(userInput); j++){
                    fprintf(stderr, "   %d: %c\n", j, userInput[j]);
                }

                pingIndexFor(userInput);                // Ask the index for a specific piece of content
                
                // Read index answer, either an S or E type PDU
                read(s_udp, readPacket, sizeof(readPacket));
                
                // Info logging purposes only
                fprintf(stderr, "Received a message from the index server:\n");
                for(j = 0; j <= sizeof(readPacket)-1; j++){
                    fprintf(stderr, "%d: %c\n", j, readPacket[j]);
                }

                switch(readPacket[0]){
                    case 'E':
                        // Copies incoming packet into a PDU-E struct
                        j = 1;
                        packetE.type = readPacket[0];
                        while(readPacket[j] != '\0'){ 
                            packetE.errMsg[j-1] = readPacket[j];
                            j++;
                        }
                        // Output to user
                        printf("Error downloading content:\n");
                        printf("    %s\n", packetE.errMsg);
                        printf("\n");
                        break;
                    case 'S':
                        // Copies incoming packet into a PDU-S struct
                        packetS.type = readPacket[0];
                        for(j = 0; j < sizeof(readPacket); j++){
                            if (j < 10){
                                packetS.peerName[j] = readPacket[j+1]; // 1 to 10
                            }
                            packetS.contentNameOrAddress[j] = readPacket[j+11]; // 11 to 100
                        }

                        // Info logging purposes only
                        fprintf(stderr, "Parsed the incoming message into the following S-PDU:\n");
                        fprintf(stderr, "   Type: %c\n", packetS.type);
                        fprintf(stderr, "   Peer Name: %s\n", packetS.peerName);
                        fprintf(stderr, "   Address: %s\n", packetS.contentNameOrAddress);

                        // Handles requesting a download from peer with address [packetS.contentNameOrAddress] 
                        // with content name [userInput]
                        downloadContent(userInput, packetS.contentNameOrAddress);
                        break;
                }
                break;
            case 'O':   // List all the content available on the index server
                listIndexServerContent();
                break;
            case 'L':   // List all local content registered
                listLocalContent();
                break;
            case 'Q':   // De-register all local content from the server and quit
                for(j = 0; j < numOfLocalContent; j++){
                    deregisterContent(localContentName[j], 1);
                }
                quit = 1;
                break;
            default:
                printf("Invalid choice, try again\n");
        } 
    }
    close(s_udp);
    close(s_tcp);
    exit(0);
}