/* Bot Net Include file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#ifndef __BOTNET_H__
#define __BOTNET_H__

#ifdef BAHAMUT
#include <time.h>
#endif
#include <sys/types.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock.h> /* HMage: was <winsock2.h> */
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

/* HMage: had to ifndef this since it's already defined in C++ */
#ifndef __cplusplus
typedef unsigned char bool;
#endif

#define false 0
#define true 1

#define BN_SOCKET_TIMEOUT 300 /* Wait for data up to 5 min... else time out */

#define BN_LOW_PRIORITY      0
#define BN_HIGH_PRIORITY     1

#define PROCESS_KEEP_SOCKET  1
#define PROCESS_KEEP_SIG     2
#define PROCESS_KEEP_EVENT   4
#define PROCESS_NEW_PROCESS  8
#define PROCESS_NEW_THREAD   16

#define BN_SERVLENGTH   64
#define BN_NICKLENGTH   32
#define BN_UNLENGTH     20
#define BN_ADRSLENGTH   16
#define BN_BUFFERSIZE   1024
#define BN_STAMPSIZE    256
#define CHAT_BUFFERSIZE 1024
#define CHAT_IDLESEC      30
#define DCC_PACKETSIZE  1024
#define CTCP_PING     "PING"
#define CTCP_TIME     "TIME"
#define CTCP_VERSION  "VERSION"
#define CTCP_FINGER   "FINGER"
#define CTCP_SOUND    "SOUND"
#define CTCP_USERINFO "USERINFO"
#define CTCP_BOTNET   "BOTNET"

#define WHOIS_NICKNAME   0
#define WHOIS_USERNAME   1
#define WHOIS_HOSTNAME   2
#define WHOIS_REALNAME   3
#define WHOIS_SERVER     4
#define WHOIS_OPERATOR   5
#define WHOIS_IDLE       6
#define WHOIS_CHANNELS   7
#define WHOIS_AWAY       8
#define WHOIS_INFO_COUNT 9

#define WHO_NICKNAME     0
#define WHO_USERNAME     1
#define WHO_HOSTNAME     2
#define WHO_REALNAME     3
#define WHO_SERVER       4
#define WHO_FLAGS        5
#define WHO_INFO_COUNT   6

#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_TOOMANYCHANNELS  405
#define ERR_WASNOSUCHNICK    406
#define ERR_TOOMANYTARGETS   407
#define ERR_NOORIGIN         409
#define ERR_NORECIPIENT      411
#define ERR_NOTEXTTOSEND     412
#define ERR_NOTOPLEVEL       413
#define ERR_WILDTOPLEVEL     414
#define ERR_UNKNOWNCOMMAND   421
#define ERR_NOMOTD           422
#define ERR_NOADMININFO      423
#define ERR_FILEERROR        424
#define ERR_NONICKNAMEGIVEN  431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE    433
#define ERR_NICKCOLLISION    436
#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL     442
#define ERR_USERONCHANNEL    443
#define ERR_NOLOGIN          444
#define ERR_SUMMONDISABLED   445
#define ERR_USERSDISABLED    446
#define ERR_NOTREGISTERED    451
#define ERR_NEEDMOREPARAMS   461
#define ERR_ALREADYREGISTRED 462
#define ERR_NOPERMFORHOST    463
#define ERR_PASSWDMISMATCH   464
#define ERR_YOUREBANNEDCREEP 465
#define ERR_KEYSET           467
#define ERR_CHANNELISFULL    471
#define ERR_UNKNOWNMODE      472
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_NOPRIVILEGES     481
#define ERR_CHANOPRIVSNEEDED 482
#define ERR_CANTKILLSERVER   483
#define ERR_NOOPERHOST       491
#define ERR_UMODEUNKNOWNFLAG 501
#define ERR_USERSDONTMATCH   502

#define DCC_COMPLETED 1
#define DCC_FAILED    2

struct BN_SFlood;
struct BN_SInfo;
struct BN_SChat;
struct BN_SSend;

typedef struct
{
  void (*OnDCCChatRequest)(struct BN_SInfo *,struct BN_SChat *); // Chat
  void (*OnDCCChatOpened)(struct BN_SInfo *,struct BN_SChat *); // Chat
  void (*OnDCCChatClosed)(struct BN_SInfo *,struct BN_SChat *); // Chat
  void (*OnDCCTalk)(struct BN_SInfo *,struct BN_SChat *,const char []); // Chat Msg
  void (*OnDCCChatIdle)(struct BN_SInfo *,struct BN_SChat *); // Chat
} BN_TChatCallbacks,*BN_PChatCallbacks;

typedef struct
{
  void (*OnDCCSendOpened)(struct BN_SInfo *,struct BN_SSend *); // Send
  void (*OnDCCSendPacket)(struct BN_SInfo *,struct BN_SSend *,long int); // Send PacketLength
  void (*OnDCCSendClosed)(struct BN_SInfo *,struct BN_SSend *,int); // Send Code
  void (*OnDCCSendRequest)(struct BN_SInfo *,struct BN_SSend *); // Send
  void (*OnDCCGetOpened)(struct BN_SInfo *,struct BN_SSend *); // Send
  void (*OnDCCGetPacket)(struct BN_SInfo *,struct BN_SSend *,long int); // Send PacketLength
  void (*OnDCCGetClosed)(struct BN_SInfo *,struct BN_SSend *,int); // Send Code
} BN_TSendCallbacks,*BN_PSendCallbacks;

typedef struct
{
  void (*OnConnected)(struct BN_SInfo *,const char []); // Host
  void (*OnRegistered)(struct BN_SInfo *); // Void
  void (*OnError)(struct BN_SInfo *,int); // Num
  void (*OnPingPong)(struct BN_SInfo *); // Void
  void (*OnStatus)(struct BN_SInfo *,const char [],int); // Msg Code
  void (*OnJoinChannel)(struct BN_SInfo *,const char []); // Chan
  void (*OnChannelTalk)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Msg
  void (*OnAction)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Msg
  void (*OnPrivateTalk)(struct BN_SInfo *,const char [],const char [],const char []); // Who Whom Msg
  char * (*OnCTCP)(struct BN_SInfo *,const char [],const char [],const char []); // Who Whom Type .. returned string is freed after use
  void (*OnCTCPReply)(struct BN_SInfo *,const char [],const char [],const char []); // Who Whom Msg
  void (*OnNotice)(struct BN_SInfo *,const char [],const char [],const char []); // Who Whom Msg
  void (*OnNick)(struct BN_SInfo *,const char [],const char []); // Who Msg
  void (*OnJoin)(struct BN_SInfo *,const char [],const char []); // Chan Who
  void (*OnPart)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Msg
  void (*OnQuit)(struct BN_SInfo *,const char [],const char []); // Who Msg
  void (*OnMode)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Msg
  void (*OnModeIs)(struct BN_SInfo *,const char [],const char []); // Chan Msg
  void (*OnTopic)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Msg
  void (*OnTopicIs)(struct BN_SInfo *,const char [],const char []); // Chan Msg
  void (*OnTopicSetBy)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Msg Time
  void (*OnList)(struct BN_SInfo *,const char *[],const char *[],const char *[],const int); // Channels Usercounts Topics Count
  void (*OnNames)(struct BN_SInfo *,const char [],const char *[],const int); // Chan List Count
  void (*OnWhois)(struct BN_SInfo *,const char *[]); // Infos
  void (*OnWho)(struct BN_SInfo *,const char [],const char *[],const int); // Chan Infos-list Count
  void (*OnBanList)(struct BN_SInfo *,const char [],const char *[],const int); // Chan BanList Count
  void (*OnInvite)(struct BN_SInfo *,const char [],const char [],const char []); // Chan Who Whom
  void (*OnKick)(struct BN_SInfo *,const char [],const char [],const char [],const char []); // Chan Who Whom Msg
  void (*OnKill)(struct BN_SInfo *,const char [],const char [],const char []); // Who Whom Msg
  void (*OnDisconnected)(struct BN_SInfo *,const char []); // Msg
  void (*OnExcessFlood)(struct BN_SInfo *,const char []); // Msg
  void (*OnUnknown)(struct BN_SInfo *,const char [],const char [],const char []); // Who Command Msg

  BN_TChatCallbacks Chat;
  BN_TSendCallbacks Send;

  void * (*OnUserCallback1)(struct BN_SInfo *,void *Info);
  void * (*OnUserCallback2)(struct BN_SInfo *,void *Info);
  void * (*OnUserCallback3)(struct BN_SInfo *,void *Info);
  void * (*OnUserCallback4)(struct BN_SInfo *,void *Info);
} BN_TCallbacks,*BN_PCallbacks;

/* Server callbacks */
typedef struct
{
  void (*OnPass)(struct BN_SInfo *,const char [],const char [],const char [],const char []); // Pass Version Flags Options
  void (*OnServer)(struct BN_SInfo *,const char [],const int,const int,const char []); // Server HopCount Token Info
  void (*OnForeignServer)(struct BN_SInfo *,const char [],const char [],const int,const int,const char []); // Server Foreign HopCount Token Info
  void (*OnService)(struct BN_SInfo *,const char [],const int,const char [],const char [],const int,const char []); // ServiceName ServToken Distribution Type HopCount Info
  void (*OnSQuit)(struct BN_SInfo *,const char [],const char []); // Server Msg
  void (*OnForeignSQuit)(struct BN_SInfo *,const char [],const char [],const char []); // Server ServLost Msg
  void (*OnAdmin)(struct BN_SInfo *,const char [],const char []); // Who (NULL if user on this server) Server (if NULL, user on this server)
#ifdef BAHAMUT
  void (*OnSJoin)(struct BN_SInfo *,const char [],time_t,time_t,const char [],const char [],const char *[],const int); // Chan Time1 Time2 Modes Keyword Nicks Count
  void (*OnNick)(struct BN_SInfo *,const char [],const int,const time_t,const char [],const char [],const char [],const char [],const char []); // Nick HopCount TimeStamp UMode UserName HostName ServerName RealName
#else
  void (*OnNJoin)(struct BN_SInfo *,const char [],const char *[],const int); // Chan Nicks Count
  void (*OnNick)(struct BN_SInfo *,const char [],const int,const char [],const char [],const int,const char [],const char []); // Nick HopCount UserName HostName ServToken UMode RealName
#endif
} BN_TServerCallbacks,*BN_PServerCallbacks;
/* *********** */

typedef struct BN_SChat
{
/* Server data */
  char To[BN_NICKLENGTH+1];
  BN_TChatCallbacks CB;
/* *********** */
  char Nick[BN_NICKLENGTH+1];
  char UN[BN_UNLENGTH];
  int  Status;
  char Addr[BN_ADRSLENGTH];
  int  Port;
  int  Socket;
  int  Flags;
  char Buf[CHAT_BUFFERSIZE];
  int  BufPos;
  struct sockaddr_in SAddr;
  void *User;
} BN_TChat,*BN_PChat;

typedef struct BN_SSend
{
/* Server data */
  char To[BN_NICKLENGTH+1];
  BN_TSendCallbacks CB;
/* *********** */
  char Nick[BN_NICKLENGTH+1];
  char UN[BN_UNLENGTH];
  int  Status;
  char Addr[BN_ADRSLENGTH];
  int  Port;
  int  Socket;
  int  Flags;
  char Buf[CHAT_BUFFERSIZE];
  int  BufPos;
  char Filename[PATH_MAX];
  unsigned long int Length;
  struct sockaddr_in SAddr;
  void *User;
} BN_TSend,*BN_PSend;

typedef struct BN_SInfo
{
  int Socket;
  char Nick[BN_NICKLENGTH+1];
  char Buf[BN_BUFFERSIZE];
  int BufPos;
  BN_TCallbacks CB;
/* Server data */
  BN_TServerCallbacks SCB;
  char Serv[BN_SERVLENGTH];
  bool IsServer;
  int PingDelay;
  int MyToken;
/* *********** */
  struct BN_SFlood *Flood;
  void *User;
  int Flags;
  char *Server;
  int Port;
  time_t Stamps[BN_STAMPSIZE];
  int    StampPos,NbStamps,StampLen[BN_STAMPSIZE];
  time_t Time;
#ifdef _WIN32
  HANDLE Thread;
#elif __unix__
  pthread_t Thread;
#endif
} BN_TInfo,*BN_PInfo;


/* Return version number */
char * BN_GetVersion(void);

/* Return copyright */
char *BN_GetCopyright(void);

/* Sets botnet debug level */
void BN_SetDebugLevel(int Level);

/* Connect to the specified server:port (BN_PInfo must be initialized) */
bool BN_Connect(BN_PInfo,const char *,const int,const int);

/* Enable the flood detection mechanism */
void BN_EnableFloodProtection(BN_PInfo,int,int,time_t);

/* Disable the flood detection mechanism */
void BN_DisableFloodProtection(BN_PInfo);

/* Send a QUIT message, and disconnect from server */
void BN_SendQuitMessage(BN_PInfo,const char *);

/* Create an IRC message (see RFC) with Prefix, Command and Parameters */
char *BN_MakeMessage(const char *,const char *,const char *);

/* Send your password message (BEFORE registering) */
void BN_SendPassword(BN_PInfo,const char *);

/* Register yourself to the server (see RFC) */
void BN_Register(BN_PInfo,const char *,const char *,const char *);

/* Register yourself as a service */
void BN_RegisterService(BN_PInfo I,const char *,const char *,const char *,const char *);

/* Change your nick */
void BN_SendNickMessage(BN_PInfo,const char *);

/* Send a message to a channel */
void BN_SendChannelMessage(BN_PInfo,const char *,const char *);

/* Send an action message to a channel (/me command) */
void BN_SendActionMessage(BN_PInfo,const char *,const char *);

/* Send a private message */
void BN_SendPrivateMessage(BN_PInfo,const char *,const char *);

/* Send a service query message */
void BN_SendSQueryMessage(BN_PInfo,const char *,const char *);

/* Send a Wallop message */
void BN_SendWallopMessage(BN_PInfo,const char *);

/* Send a message to the server (low level). Buffer is freed after. (CB.OnError if error, then exits)
   The last parameter is a priority value in case of excess flood */
bool BN_SendMessage(BN_PInfo,char *,int);

/* Join a channel (with key if needed)*/
void BN_SendJoinMessage(BN_PInfo,const char*,const char *);

/* Part a channel (with reason)*/
void BN_SendPartMessage(BN_PInfo,const char*,const char *);

/* Extract nickname from Prefix */
void BN_ExtractNick(const char *,char [],int);

/* Extract hostname from Prefix */
void BN_ExtractHost(const char *,char [],int);

/* Extract username from Prefix */
void BN_ExtractUserName(const char *,char [],int);

/* Extract exact username from Prefix (with ~ or ^) */
void BN_ExtractExactUserName(const char *,char [],int);

/* Strip mirc color from text */
char *BN_StripColor(const char *);

/* Send a DCC Chat request */
void BN_SendDCCChatRequest(BN_PInfo,const char *,const int);

/* Accept a DCC CHAT connection */
void BN_AcceptDCCChat(BN_PInfo,BN_PChat,const int);

/* Reject a DCC CHAT connection */
void BN_RejectDCCChat(BN_PChat);

/* Send a message through a DCC CHAT connection */
void BN_SendDCCChatMessage(BN_PInfo,BN_PChat,const char *);

/* Close a DCC CHAT connection */
void BN_CloseDCCChat(BN_PInfo,BN_PChat);

/* Send a DCC Send request */
void BN_SendDCCSendRequest(BN_PInfo,const char *,const int, char *,const int);

/* Accept a DCC SEND connection */
void BN_AcceptDCCSend(BN_PInfo, BN_PSend, const int);

/* Reject a DCC SEND connection */
void BN_RejectDCCSend(BN_PSend);


/* ************* Server Functions ************** */
/* Server functions */

void BN_ServerSendErrorMessage(BN_PInfo,const char *);

/* ******* Leaf servers ******** */

/* Send a PASS message, needed before ANY other message */
void BN_ServerSendPassMessage(BN_PInfo,const char *,const char *,bool,bool);

/* Send a SERVER message, the next message after PASS */
void BN_ServerSendServerMessage(BN_PInfo,const char *,const char *);

#ifdef BAHAMUT
/* Send a NICK message, used when connecting, to tell other servers of my users */
void BN_ServerSendNickMessage(BN_PInfo,const char *,const int,const char *,const char *,const char *,const char *,const char *);

/* Send a SJOIN message, used when connecting, to tell other servers of my users' channels */
void BN_ServerSendSJoinMessage(BN_PInfo,const char *,const char *,const char *,time_t,time_t);
#else

/* Send a NICK message, used when connecting, to tell other servers of my users */
/* A user that is local for me have a LocalHopCount of 0 */
void BN_ServerSendNickMessage(BN_PInfo,const char *,const int,const char *,const char *,const int,const char *,const char *);

/* Send a NJOIN message, used when connecting, to tell other servers of my users' channels */
void BN_ServerSendNJoinMessage(BN_PInfo,const char *,const char *);
#endif

/* Send a SQUIT message to the specified server */
void BN_ServerSendSQuitMessage(BN_PInfo,const char *,const char *);


/* ******* Hub servers ******** */

/* 1) When a new server connects, hub servers must forward this message (ConnectedServer = I->Prefix) */
/* 2) But they also must reply to this new server, giving info on every server connected to the network */
/*    Hub uses ConnectedServer to tell on wich server is connected ConnectingServer */
/*    And so ConnectedServer MAY be different of I->Prefix, if there is another hub behind the hub */
void BN_ServerForwardServerMessage(BN_PInfo,const char *,const char *,const int,const int,const char *);

/* *********** */

#endif


