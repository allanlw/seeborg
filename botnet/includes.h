/* Bot Net Includes file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef CONTEXT
extern int BN_ContextLine;
extern char BN_ContextFile[512];
#define context set_context(__FILE__, __LINE__)
void set_context(char *file, int line);
#else
#define context
#endif

#include "botnet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#ifdef _WIN32 /* Win32 */
#include <io.h>
#else        /* Else Win32 */
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif       /* End Win32 */
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32 /* If Win32 */
#define CLOSE_SOCKET_FUNC closesocket
#define STRICMP_FUNC stricmp
#define STRNICMP_FUNC strnicmp
#define EXIT_THREAD_FUNC ExitThread(
#define EXIT_PROCESS_FUNC ExitThread
#define open _open
#define read _read
#define close _close
#define write _write
#define snprintf _snprintf
#else         /* Else Win32 */
#define CLOSE_SOCKET_FUNC close
#define STRICMP_FUNC strcasecmp
#define STRNICMP_FUNC strncasecmp
#define EXIT_THREAD_FUNC pthread_exit((void *)
#define EXIT_PROCESS_FUNC exit
#endif        /* End Win32 */

#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif // INADDR_NONE

#define MAX_PARAMS 30

typedef struct
{
  char     *Prefix;
  char     *Command;
  int      NbParams;
  char     *Params[MAX_PARAMS];
} BN_TMessage,*BN_PMessage;

struct BN_SQueue;
typedef struct BN_SQueue
{
  char *Message;
  int Prio;
  struct BN_SQueue *Next;
} BN_TQueue, *BN_PQueue;

typedef struct BN_SFlood
{
  int Length;
  time_t FloodTime;
  int FloodSize;
  time_t *Time;
  int *Size;
  int Total;
  int spos,epos;
  BN_PQueue Queue;
#ifdef HAVE_SEM_INIT
  sem_t sem;
#endif
} BN_TFlood,*BN_PFlood;

/* From utils.c */
char *GetTimeStamp();
char *TrimLeft(const char *S);
void BN_CreateStatusLine(char *Params[],int min,int max,char Out[],int len);
void BN_UnsetSigs(bool);
void BN_UnsetEvents(BN_PInfo I,bool Chat);
bool BN_CheckForFlood(BN_PFlood,int len);
void BN_PrintDebug(int Level,char *Txt, ...);
#ifndef DEBUG
#ifdef __unix__
#define BN_PrintDebug(x,...) //
#else /* __unix__ */
#define BN_PrintDebug //
#endif /* __unix__ */
#endif /* DEBUG */


/* From dcc_chat.c */
#define CHAT_REQUEST  1
#define CHAT_ACTIVE   2
#define CHAT_CLOSED   3
/* Waits for a message on the DCC Chat's socket (BLOCKING) */
char * BN_WaitForDCCChatMessage(BN_PInfo,BN_PChat);
/* Creates a process for dcc connection */
void BN_CreateDCCChatProcess(BN_PInfo,BN_PChat);

/* From dcc_send.c */
#define SEND_REQUEST  1
#define SEND_ACTIVE   2
#define SEND_CLOSED   3
#define BN_TIMEOUT_TRANSFER 10
/* Creates a process for outgoing dcc connection */
void BN_CreateDCCSendProcess(BN_PInfo,BN_PSend,const int);
/* Creates a process for incoming dcc connection */
void BN_CreateDCCGetProcess(BN_PInfo,BN_PSend);

/* From botnet.c */
#define BOTNET_VERSION "1.6.3"
#define BOTNET_COPYRIGHT "Botnet version " BOTNET_VERSION " (c) Christophe CALMEJANE (Ze KiLleR / SkyTech) 1999'01"
/* Disconnects from the server */
void BN_Disconnect(BN_PInfo);
/* Parse un message. Retourne une structure PMessage remplie (INTERNAL USE) */
BN_PMessage BN_ParseMessage(const char *);
/* Creer une socket pour une connexion DCC CHAT (libere le PChat si erreur) (INTERNAL USE) */
bool BN_CreateChatSocket(BN_PChat,const char *,int);
/* Parse une structure BN_PMessage retournee par ParseMessage (INTERNAL USE) */
void BN_HandleMessage(BN_PInfo,BN_PMessage);
/* Attend que des données arrivent dans la socket et retourne un PMessage */
BN_PMessage BN_WaitForData(BN_PInfo);
/* Libere un PMessage (structure en elle meme, et son contenu) */
void BN_FreeMessage(BN_PMessage);

extern int BN_Flags;

#define RPL_NONE             300
#define RPL_USERHOST         302
#define RPL_ISON             303
#define RPL_AWAY             301
#define RPL_UNAWAY           305
#define RPL_NOWAWAY          306
#define RPL_WHOISUSER        311
#define RPL_WHOISSERVER      312
#define RPL_WHOISOPERATOR    313
#define RPL_WHOISIDLE        317
#define RPL_ENDOFWHOIS       318
#define RPL_WHOISCHANNELS    319
#define RPL_WHOWASUSER       314
#define RPL_ENDOFWHOWAS      369
#define RPL_LISTSTART        321
#define RPL_LIST             322
#define RPL_LISTEND          323
#define RPL_CHANNELMODEIS    324
#define RPL_NOTOPIC          331
#define RPL_TOPIC            332
#define RPL_INVITING         341
#define RPL_SUMMONING        342
#define RPL_VERSION          351
#define RPL_WHOREPLY         352
#define RPL_ENDOFWHO         315
#define RPL_NAMREPLY         353
#define RPL_ENDOFNAMES       366
#define RPL_LINKS            364
#define RPL_ENDOFLINKS       365
#define RPL_BANLIST          367
#define RPL_ENDOFBANLIST     368
#define RPL_INFO             371
#define RPL_ENDOFINFO        374
#define RPL_MOTDSTART        375
#define RPL_MOTD             372
#define RPL_ENDOFMOTD        376
#define RPL_YOUREOPER        381
#define RPL_REHASHING        382
#define RPL_TIME             391
#define RPL_USERSSTART       392
#define RPL_USERS            393
#define RPL_ENDOFUSERS       394
#define RPL_NOUSERS          395
#define RPL_TRACELINK        200
#define RPL_TRACECONNECTING  201
#define RPL_TRACEHANDSHAKE   202
#define RPL_TRACEUNKNOWN     203
#define RPL_TRACEOPERATOR    204
#define RPL_TRACEUSER        205
#define RPL_TRACESERVER      206
#define RPL_TRACENEWTYPE     208
#define RPL_TRACELOG         261
#define RPL_STATSLINKINFO    211
#define RPL_STATSCOMMANDS    212
#define RPL_STATSCLINE       213
#define RPL_STATSNLINE       214
#define RPL_STATSILINE       215
#define RPL_STATSKLINE       216
#define RPL_STATSYLINE       218
#define RPL_ENDOFSTATS       219
#define RPL_STATSLLINE       241
#define RPL_STATSUPTIME      242
#define RPL_STATSOLINE       243
#define RPL_STATSHLINE       244
#define RPL_UMODEIS          221
#define RPL_LUSERCLIENT      251
#define RPL_LUSEROP          252
#define RPL_LUSERUNKNOWN     253
#define RPL_LUSERCHANNELS    254
#define RPL_LUSERME          255
#define RPL_ADMINME          256
#define RPL_ADMINLOC1        257
#define RPL_ADMINLOC2        258
#define RPL_ADMINEMAIL       259

#endif
