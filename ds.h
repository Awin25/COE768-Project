/*
Packet Types:
R Content Registration
D Content Download Request
S Search for content and the associated
content server
T Content De-Registration
C Content Data
O List of On-Line Registered Content A Acknowledgement
E Error

All packetSearch have a 'type' field (1 Byte) and data fields (<=100 Bytes)
*/

// UNILATERAL packetSearch 
struct pdu {
  char type;
  char data[100];
};

struct pduR {
	char type;
  char peerName[10];
  char contentName[10];
  char host[5]; // Schema: `IP:Port` (ie. 192.168.0.1:4000)
  char port[6];
};

struct pduD {
  char type;
  char peerName[10];
  char content[90];
};

struct pduT { 
  char type;
  char peerName[10];
  char contentName[10];
};

struct pduC {
	char type;
  char content[1459]; // Max Content Shard Size due to Ethernet Constraint (1460 Bytes)
};

struct pduA {
	char type;
  char peerName[10];
};

// BILATERAL packetSearch
struct pduS {
	char type;
  char peerName[10];
  char contentNameOrAddress[90]; // Yeah I know, but that's how it's defined
};

struct pduO {
  /* 
  Each content name is 10 char max which means you can only 
  have 10 pieces of content at a given time assuming you don't 
  send multiple contentList type packetSearch
  */
	char type;
  char contentInfos[100]; 
};

struct pduE {
	char type;
  char errMsg[100];
};
