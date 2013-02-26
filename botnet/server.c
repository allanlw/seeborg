/* Bot Net Server file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#ifdef SERVER

#include "includes.h"


void BN_ServerSendErrorMessage(BN_PInfo I,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),":%s",Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"ERROR",buf),BN_HIGH_PRIORITY);

  BN_Disconnect(I);
  if(I->CB.OnDisconnected != NULL)
    I->CB.OnDisconnected(I,Message);
  if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
    EXIT_PROCESS_FUNC(1);
  else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
    EXIT_THREAD_FUNC 1);
  else
    return exit(1);
}

/* *********** LEAF SERVERS ********** */

void BN_ServerSendPassMessage(BN_PInfo I,const char *Password,const char *Version,bool Compressed,bool Secured)
{
  char buf[1024];

#ifdef BAHAMUT
  snprintf(buf,sizeof(buf),"%s :TS\n",Password);
#else
  snprintf(buf,sizeof(buf),"%s %s IRC| %s%s\n",Password,Version,Compressed?"Z":"",Secured?"P":"");
#endif
  BN_SendMessage(I,BN_MakeMessage(NULL,"PASS",buf),BN_HIGH_PRIORITY);
}

void BN_ServerSendServerMessage(BN_PInfo I,const char *ServerName,const char *Info)
{
  char buf[1024];

  /* When connecting, we only use a "1" parameter, and our token is 1 */
  strncpy(I->Serv,ServerName,sizeof(I->Serv)-1);
  I->Serv[sizeof(I->Serv)-1] = 0;
  I->IsServer = true;
  I->MyToken = 1;
  snprintf(buf,sizeof(buf),"%s %d :%s",ServerName,1,Info);
  BN_SendMessage(I,BN_MakeMessage(NULL,"SERVER",buf),BN_HIGH_PRIORITY);
}

#ifdef BAHAMUT
void BN_ServerSendNickMessage(BN_PInfo I,const char *NickName,const int LocalHopCount,const char *UserName,const char *HostName,const char *ServerName,const char *UMode,const char *RealName)
{
  char buf[1024];

  /* A user that is local for me (HopCount=0) must have a HopCount of 1 for remote peer */
  snprintf(buf,sizeof(buf),"%s %d %ld %s %s %s %s 0 :%s",NickName,LocalHopCount+1,time(NULL),UMode,UserName,HostName,ServerName,RealName);
  BN_SendMessage(I,BN_MakeMessage(NULL,"NICK",buf),BN_HIGH_PRIORITY);
}

void BN_ServerSendSJoinMessage(BN_PInfo I,const char *Channel,const char *Modes,const char *Nicks,time_t Time1,time_t Time2)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%ld %ld %s +%s :%s",Time1,Time2,Channel,Modes,Nicks);
  BN_SendMessage(I,BN_MakeMessage(NULL,"SJOIN",buf),BN_HIGH_PRIORITY);
}
#else
void BN_ServerSendNickMessage(BN_PInfo I,const char *NickName,const int LocalHopCount,const char *UserName,const char *HostName,const int ServToken,const char *UMode,const char *RealName)
{
  char buf[1024];

  /* A user that is local for me (HopCount=0) must have a HopCount of 1 for remote peer */
  snprintf(buf,sizeof(buf),"%s %d %s %s %d %s :%s",NickName,LocalHopCount+1,UserName,HostName,ServToken,UMode,RealName);
  BN_SendMessage(I,BN_MakeMessage(NULL,"NICK",buf),BN_HIGH_PRIORITY);
}

void BN_ServerSendNJoinMessage(BN_PInfo I,const char *Channel,const char *Nicks)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%s",Channel,Nicks);
  BN_SendMessage(I,BN_MakeMessage(NULL,"NJOIN",buf),BN_HIGH_PRIORITY);
}
#endif

void BN_ServerSendSQuitMessage(BN_PInfo I,const char *Server,const char *Msg)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%s",Server,Msg);
  BN_SendMessage(I,BN_MakeMessage(NULL,"SQUIT",buf),BN_HIGH_PRIORITY);
}


/* *********** HUB SERVERS ********** */

void BN_ServerForwardServerMessage(BN_PInfo I,const char *ConnectedServer,const char *ConnectingServer,const int LocalHopCount,const int Token,const char *Info)
{
  char buf[1024];

  /* 1) When a new server connects, hub servers must forward this message (ConnectedServer = I->Prefix) */
  /* 2) But they also must reply to this new server, giving info on every server connected to the network */
  /*    Hub uses ConnectedServer to tell on wich server is connected ConnectingServer */
  /*    And so ConnectedServer MAY be different of I->Prefix, if there is another hub behind the hub */
  snprintf(buf,sizeof(buf),"%s %d %d :%s",ConnectingServer,LocalHopCount+1,Token,Info);
  BN_SendMessage(I,BN_MakeMessage(ConnectedServer,"SERVER",buf),BN_HIGH_PRIORITY);
}

#endif
