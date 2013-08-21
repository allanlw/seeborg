/* Bot Net Dcc chat file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#include "includes.h"

typedef struct
{
  BN_PInfo I;
  BN_PChat Chat;
  char Nick[BN_NICKLENGTH];
  int Flags;
}BN_TTempoStruct, *BN_PTempoStruct;
#ifdef _WIN32
DWORD WINAPI ThreadProc_SendRequest(void *);
DWORD WINAPI ThreadProc_Accept(void *);
#elif __unix__
void *ThreadProc_SendRequest(void *);
void *ThreadProc_Accept(void *);
#endif

bool BN_CreateChatSocket(BN_PChat Chat,const char *Addr,int Port)
{
  socklen_t len;

  Chat->Socket = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
  if( Chat->Socket == -1 ) {
    free(Chat);
    return false;
  }

  memset(&(Chat->SAddr),0,sizeof(struct sockaddr_in));
  Chat->SAddr.sin_family = AF_INET;
  Chat->SAddr.sin_port = htons(Port);
  if(Port == 0) // Listen socket
  {
    Chat->SAddr.sin_addr.s_addr = 0;

    len = 1;
    setsockopt(Chat->Socket,SOL_SOCKET,SO_REUSEADDR,(char *)&len,sizeof(len));

    if( bind(Chat->Socket,(struct sockaddr *)&(Chat->SAddr), sizeof(Chat->SAddr)) == -1 )
    {
      CLOSE_SOCKET_FUNC(Chat->Socket);
      free(Chat);
      return false;
    }

    len = sizeof(struct sockaddr_in);
    if( getsockname(Chat->Socket,(struct sockaddr *)&(Chat->SAddr),&len) == -1 )
    {
      CLOSE_SOCKET_FUNC(Chat->Socket);
      free(Chat);
      return false;
    }
    Chat->Port = ntohs(Chat->SAddr.sin_port);
  }
  else
    Chat->SAddr.sin_addr.s_addr = inet_addr(Addr);
  Chat->BufPos = 0;
  Chat->Buf[0] = 0;
  return true;
}

void BN_ErrorCB(BN_PInfo I,int err)
{
  I->User = (void *)1;
}

void BN_SendDCCChatRequest(BN_PInfo I,const char *Nick,const int Flags)
{
  BN_PTempoStruct TS;
#ifdef _WIN32
  DWORD son;
#endif
  int Flg;

  Flg = Flags;
  if((Flags & PROCESS_NEW_PROCESS) == 0) /* By default, we create a thread */
    Flg |= PROCESS_NEW_THREAD;

  TS = (BN_PTempoStruct)malloc(sizeof(BN_TTempoStruct));
  TS->I = I;
  strncpy(TS->Nick,Nick,sizeof(TS->Nick)-1);
  TS->Nick[sizeof(TS->Nick)-1] = 0;
  TS->Flags = Flags;
#ifdef __unix__
  if((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
  {
    if(fork() != 0)
      return;
    ThreadProc_SendRequest(TS);
  }
  else
  {
    pthread_create(&I->Thread,NULL,&ThreadProc_SendRequest,TS);
    pthread_detach(I->Thread);
  }
#elif _WIN32
  I->Thread = CreateThread(NULL,0,ThreadProc_SendRequest,TS,0,&son);
#endif
}
#ifdef _WIN32
DWORD WINAPI ThreadProc_SendRequest(void *Info)
#elif __unix__
void *ThreadProc_SendRequest(void *Info)
#endif
{
  unsigned long int Adrs;
  int Port;
  BN_PChat Chat;
  BN_PInfo I;
  int Flg;
  char Msg[150];
  struct sockaddr_in SockAddrIn;
  socklen_t size;
  void *Saf,*Saf2;
  BN_PTempoStruct TS;

  TS = (BN_PTempoStruct)Info;
  Flg = TS->Flags;
  I = TS->I;

  if((Flg & PROCESS_KEEP_SIG) == 0)
    BN_UnsetSigs(Flg & PROCESS_NEW_PROCESS);
  if(((Flg & PROCESS_KEEP_EVENT) == 0) && ((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    BN_UnsetEvents(I,true);
  if(((Flg & PROCESS_KEEP_SOCKET) == 0) && ((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    CLOSE_SOCKET_FUNC(I->Socket);

  Chat = (BN_PChat)malloc(sizeof(BN_TChat));
  Chat->Status = CHAT_REQUEST;
  Chat->Flags = Flg;
  strncpy(Chat->Nick,TS->Nick,sizeof(Chat->Nick)-1);
  Chat->Nick[sizeof(Chat->Nick)-1] = 0;
  free(TS);
  if(!BN_CreateChatSocket(Chat,NULL,0))
  {
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  size = sizeof(SockAddrIn);
  if(getsockname(I->Socket,(struct sockaddr *)&SockAddrIn,&size) == -1)
  {
    CLOSE_SOCKET_FUNC(Chat->Socket);
    free(Chat);
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  Adrs = ntohl(SockAddrIn.sin_addr.s_addr);
  Port = ntohs(Chat->SAddr.sin_port);
  if(listen(Chat->Socket,0) == -1)
  {
    CLOSE_SOCKET_FUNC(Chat->Socket);
    free(Chat);
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  snprintf(Msg,sizeof(Msg),"%cDCC CHAT chat %lu %d%c",1,Adrs,Port,1);
  Saf = I->CB.OnError;
  I->CB.OnError = BN_ErrorCB;
  Saf2 = I->User;
  I->User = 0;
  BN_SendPrivateMessage(I,Chat->Nick,Msg);
  if(I->User != 0)
  {
    CLOSE_SOCKET_FUNC(Chat->Socket);
    free(Chat);
    I->CB.OnError = Saf;
    I->User = Saf2;
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  I->CB.OnError = Saf;
  I->User = Saf2;
  BN_PrintDebug(2,"Successfuly sent a DCC CHAT REQUEST\n");
  BN_CreateDCCChatProcess(I,Chat);
  return 0;
}

void BN_AcceptDCCChat(BN_PInfo I,BN_PChat Chat,const int Flags)
{
  BN_PTempoStruct TS;
#ifdef _WIN32
  DWORD son;
#endif

  Chat->Flags = Flags;
  if((Flags & PROCESS_NEW_PROCESS) == 0) /* By default, we create a thread */
    Chat->Flags |= PROCESS_NEW_THREAD;

  TS = (BN_PTempoStruct)malloc(sizeof(BN_TTempoStruct));
  TS->I = I;
  TS->Chat = Chat;
#ifdef __unix__
  if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
  {
    if(fork() != 0)
      return;
    ThreadProc_Accept(TS);
  }
  else
  {
    pthread_create(&I->Thread,NULL,&ThreadProc_Accept,TS);
    pthread_detach(I->Thread);
  }
#elif _WIN32
  I->Thread = CreateThread(NULL,0,ThreadProc_Accept,TS,0,&son);
#endif
}
#ifdef _WIN32
DWORD WINAPI ThreadProc_Accept(void *Info)
#elif __unix__
void *ThreadProc_Accept(void *Info)
#endif
{
  BN_PTempoStruct TS;
  BN_PInfo I;
  BN_PChat Chat;

  TS = (BN_PTempoStruct)Info;
  I = TS->I;
  Chat = TS->Chat;
  free(TS);

  if((Chat->Flags & PROCESS_KEEP_SIG) == 0)
    BN_UnsetSigs(Chat->Flags & PROCESS_NEW_PROCESS);
  if(((Chat->Flags & PROCESS_KEEP_EVENT) == 0) && ((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    BN_UnsetEvents(I,true);
  if(((Chat->Flags & PROCESS_KEEP_SOCKET) == 0) && ((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    CLOSE_SOCKET_FUNC(I->Socket);

  Chat->Status = CHAT_ACTIVE;
  if(!BN_CreateChatSocket(Chat,Chat->Addr,Chat->Port))
  {
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  if(connect(Chat->Socket, (struct sockaddr *)&Chat->SAddr, sizeof(Chat->SAddr)) == -1)
  {
    CLOSE_SOCKET_FUNC(Chat->Socket);
    free(Chat);
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  BN_PrintDebug(2,"DCC connected with %s\n",Chat->Nick);
  BN_CreateDCCChatProcess(I,Chat);
  return 0;
}

void BN_SendDCCChatMessage(BN_PInfo I,BN_PChat Chat,const char *Message)
{
  int res,len;

  if(Message == NULL)
    return;

  len = strlen(Message);
  res = send(Chat->Socket,Message,len,0);
  BN_PrintDebug(4,"%d octets sent DCC (for a length of %d)\n",res,len);
  if(len != res)
  {
    if(Chat->Status != CHAT_CLOSED)
      BN_CloseDCCChat(I,Chat);
  }
  return;
}

void BN_CloseDCCChat(BN_PInfo I,BN_PChat Chat)
{
  int Flags;
  Chat->Status = CHAT_CLOSED;
  CLOSE_SOCKET_FUNC(Chat->Socket);
  Chat->Socket = 0;
  if(Chat->CB.OnDCCChatClosed != NULL)
    Chat->CB.OnDCCChatClosed(I,Chat);
  Flags = Chat->Flags;
  free(Chat);
  if((Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
    EXIT_PROCESS_FUNC(1);
  else
    EXIT_THREAD_FUNC 1);
}

char *BN_WaitForDCCChatMessage(BN_PInfo I,BN_PChat Chat)
{
  socklen_t len;
  char *buf,*pos;
  int blocking=0;
  int ajout=2;

  if(Chat->Status == CHAT_CLOSED)
  {
    free(Chat);
    if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  while(true)
  {
    pos = strstr(Chat->Buf,"\r\n");
    if(pos == NULL)
    {
      pos = strstr(Chat->Buf,"\n");
      ajout=1;
    }
    if(pos != NULL) /* if there is a new message */
    {
      len = pos - Chat->Buf;
      BN_PrintDebug(3,"DCC string found (%d octets)\n",len);
      buf = (char *)malloc(len+1);
      memcpy(buf,Chat->Buf,len);
      buf[len] = 0;
      memcpy(Chat->Buf,pos+ajout,Chat->BufPos-len-ajout);
      Chat->BufPos -= (len+ajout);
      Chat->Buf[Chat->BufPos] = 0;

      break;
    }
    else if((CHAT_BUFFERSIZE-Chat->BufPos) >= 0) /* Else if not a full string */
    {
      if(Chat->CB.OnDCCChatIdle != NULL)
      {
        struct timeval Timeout;
        fd_set FdSet;
        unsigned short timedout;

        do
        {
          Timeout.tv_sec = CHAT_IDLESEC;
          Timeout.tv_usec = 0;

          timedout = 0;
          FD_ZERO(&FdSet);
          FD_SET(Chat->Socket, &FdSet);
          if(select(256, &FdSet, NULL, NULL, &Timeout)==0)
          {
            /* Timeout */
            Chat->CB.OnDCCChatIdle(I, Chat);
            timedout = 1;
            BN_PrintDebug(2,"DCC chat timed out\n");
          }
        } while (timedout);
      }

      len = recv(Chat->Socket,Chat->Buf+Chat->BufPos,CHAT_BUFFERSIZE-Chat->BufPos-1,0);

      if(len == 0)
      {
        if((CHAT_BUFFERSIZE-Chat->BufPos-1) != 0) /* Socket probably closed */
        {
          if(Chat->Status != CHAT_CLOSED)
            BN_CloseDCCChat(I,Chat);
          free(Chat);
          if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
            EXIT_PROCESS_FUNC(1);
          else
            EXIT_THREAD_FUNC 1);
        }
        blocking++;
        if(blocking >= 10)
        {
          printf("DCC Process in infinite loop.... returning partial buffer\n");
          buf = (char *)malloc(Chat->BufPos+1);
          memcpy(buf,Chat->Buf,Chat->BufPos);
          buf[Chat->BufPos] = 0;
          Chat->BufPos = 0;
          break;
        }
      }
      else if(len == -1)
      {
        if(Chat->Status != CHAT_CLOSED)
          BN_CloseDCCChat(I,Chat);
        free(Chat);
        if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
          EXIT_PROCESS_FUNC(1);
        else
          EXIT_THREAD_FUNC 1);
      }
      Chat->Buf[Chat->BufPos+len] = 0;
      Chat->BufPos += len;
    }
  }
  return buf;
}

void BN_CreateDCCChatProcess(BN_PInfo I,BN_PChat Chat)
{
  char *S;
  int sock;
  socklen_t len;

  while(Chat->Status == CHAT_REQUEST)
  {
    len = sizeof(Chat->SAddr);
    sock = accept(Chat->Socket,(struct sockaddr *)&Chat->SAddr,&len);
    if(sock == -1)
    {
      printf("Erreur accept\n");
      if((Chat->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
        EXIT_PROCESS_FUNC(1);
      else
        EXIT_THREAD_FUNC 1);
    }
    CLOSE_SOCKET_FUNC(Chat->Socket);
    Chat->Socket = sock;
    Chat->Status = CHAT_ACTIVE;
    S = inet_ntoa(Chat->SAddr.sin_addr);
    strncpy(Chat->Addr,S,sizeof(Chat->Addr)-1);
    Chat->Addr[sizeof(Chat->Addr)-1] = 0;
  }

//  Chat->User = 0;
  if(Chat->CB.OnDCCChatOpened != NULL)
    Chat->CB.OnDCCChatOpened(I,Chat);

  while(true)
  {
    S = BN_WaitForDCCChatMessage(I,Chat);
    if(Chat->CB.OnDCCTalk != NULL)
      Chat->CB.OnDCCTalk(I,Chat,S);
    free(S);
  }
}

void BN_RejectDCCChat(BN_PChat Chat)
{
  free(Chat);
}

