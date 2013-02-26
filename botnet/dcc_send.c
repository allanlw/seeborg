/* Bot Net Dcc Send file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
    & Jimmy Cederholm
*/

#include "includes.h"

typedef struct
{
  BN_PInfo I;
  BN_PSend Send;
  char Nick[BN_NICKLENGTH];
  char Filename[PATH_MAX];
  int Flags;
  int TimeOut;
}BN_TTempoStruct, *BN_PTempoStruct;
#ifdef _WIN32
DWORD WINAPI ThreadProc_SendRequestSend(void *);
DWORD WINAPI ThreadProc_AcceptSend(void *);
#elif __unix__
void *ThreadProc_SendRequestSend(void *);
void *ThreadProc_AcceptSend(void *);
#endif

bool BN_CreateSendSocket(BN_PSend Send, const char *Addr, int Port)
{
  int len;

  Send->Socket = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
  if( Send->Socket == -1 ) {
    free(Send);
    return false;
  }

  memset(&(Send->SAddr),0,sizeof(struct sockaddr_in));
  Send->SAddr.sin_family = AF_INET;
  Send->SAddr.sin_port = htons(Port);
  if(Port == 0) // Listen socket
  {
    Send->SAddr.sin_addr.s_addr = 0;

    len = 1;
    setsockopt(Send->Socket,SOL_SOCKET,SO_REUSEADDR,(char *)&len,sizeof(len));

    if( bind(Send->Socket,(struct sockaddr *)&(Send->SAddr), sizeof(Send->SAddr)) == -1 )
    {
      CLOSE_SOCKET_FUNC(Send->Socket);
      free(Send);
      return false;
    }

    len = sizeof(struct sockaddr_in);
    if( getsockname(Send->Socket,(struct sockaddr *)&(Send->SAddr),&len) == -1 )
    {
      CLOSE_SOCKET_FUNC(Send->Socket);
      free(Send);
      return false;
    }
    Send->Port = ntohs(Send->SAddr.sin_port);
  }
  else
    Send->SAddr.sin_addr.s_addr = inet_addr(Addr);
  Send->BufPos = 0;
  return true;
}

void BN_ErrorCBSend(BN_PInfo I,int err)
{
  I->User = (void *)1;
}

void BN_SendDCCSendRequest(BN_PInfo I,const char *Nick,const int Flags, char *Filename, const int Timeout)
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
  strncpy(TS->Filename,Filename,sizeof(TS->Filename)-1);
  TS->Filename[sizeof(TS->Filename)-1] = 0;
  TS->Flags = Flags;
  TS->TimeOut = Timeout;
#ifdef __unix__
  if((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
  {
    if(fork() != 0)
      return;
    ThreadProc_SendRequestSend(TS);
  }
  else
  {
    pthread_create(&I->Thread,NULL,&ThreadProc_SendRequestSend,TS);
    pthread_detach(I->Thread);
  }
#elif _WIN32
  I->Thread = CreateThread(NULL,0,ThreadProc_SendRequestSend,TS,0,&son);
#endif
}
#ifdef _WIN32
DWORD WINAPI ThreadProc_SendRequestSend(void *Info)
#elif __unix__
void *ThreadProc_SendRequestSend(void *Info)
#endif
{
  unsigned long int Adrs;
  int Port;
  BN_PSend Send;
  BN_PInfo I;
  int Flg,Timeout;
  char Msg[150];
  struct sockaddr_in SockAddrIn;
  int size;
  void *Saf,*Saf2;
#ifdef _WIN32 /* HMage: in MSVC, this is called '_stat', oddly */
  struct _stat statbuf;
#elif __unix__
  struct stat statbuf;
#endif
  BN_PTempoStruct TS;

  TS = (BN_PTempoStruct)Info;
  Flg = TS->Flags;
  I = TS->I;
  Timeout = TS->TimeOut;

  if((Flg & PROCESS_KEEP_SIG) == 0)
    BN_UnsetSigs(Flg & PROCESS_NEW_PROCESS);
  if(((Flg & PROCESS_KEEP_EVENT) == 0) && ((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    BN_UnsetEvents(I,false);
  if(((Flg & PROCESS_KEEP_SOCKET) == 0) && ((Flg & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    CLOSE_SOCKET_FUNC(I->Socket);

  Send = (BN_PSend)malloc(sizeof(BN_TSend));
  Send->Status = SEND_REQUEST;
  Send->Flags = Flg;
  strncpy(Send->Nick,TS->Nick,sizeof(Send->Nick)-1);
  Send->Nick[sizeof(Send->Nick)-1] = 0;
  strncpy(Send->Filename,TS->Filename,sizeof(Send->Filename)-1);
  Send->Filename[sizeof(Send->Filename)-1] = 0;
  free(TS);
#ifdef _WIN32
  if (_stat(Send->Filename, &statbuf) != 0)
#elif __unix__
  if (lstat(Send->Filename, &statbuf) < 0)
#endif
  {
    CLOSE_SOCKET_FUNC(Send->Socket);
    free(Send);
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  Send->Length = statbuf.st_size;

  if(!BN_CreateSendSocket(Send,NULL,0))
  {
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  size = sizeof(SockAddrIn);
  if(getsockname(I->Socket,(struct sockaddr *)&SockAddrIn,&size) == -1)
  {
    CLOSE_SOCKET_FUNC(Send->Socket);
    free(Send);
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  Adrs = ntohl(SockAddrIn.sin_addr.s_addr);
  Port = ntohs(Send->SAddr.sin_port);
  if(listen(Send->Socket,0) == -1)
  {
    CLOSE_SOCKET_FUNC(Send->Socket);
    free(Send);
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }

  snprintf(Msg,sizeof(Msg),"%cDCC SEND %s %lu %d %ld%c", 1, Send->Filename, Adrs, Port, Send->Length, 1);
  Saf = I->CB.OnError;
  I->CB.OnError = BN_ErrorCBSend;
  Saf2 = I->User;
  I->User = 0;
  BN_SendPrivateMessage(I,Send->Nick,Msg);
  if(I->User != 0)
  {
    CLOSE_SOCKET_FUNC(Send->Socket);
    free(Send);
    I->CB.OnError = Saf;
    I->User = Saf2;
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  I->CB.OnError = Saf;
  I->User = Saf2;
  BN_PrintDebug(2,"Successfully sent a DCC SEND REQUEST (with %d timeout value)\n",Timeout);
  BN_CreateDCCSendProcess(I,Send,Timeout);
  return 0;
}

void BN_AcceptDCCSend(BN_PInfo I,BN_PSend Send,const int Flags)
{
  BN_PTempoStruct TS;
#ifdef _WIN32
  DWORD son;
#endif

  Send->Flags = Flags;
  if((Flags & PROCESS_NEW_PROCESS) == 0) /* By default, we create a thread */
    Send->Flags |= PROCESS_NEW_THREAD;

  TS = (BN_PTempoStruct)malloc(sizeof(BN_TTempoStruct));
  TS->I = I;
  TS->Send = Send;
#ifdef __unix__
  if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
  {
    if(fork() != 0)
      return;
    ThreadProc_AcceptSend(TS);
  }
  else
  {
    pthread_create(&I->Thread,NULL,&ThreadProc_AcceptSend,TS);
    pthread_detach(I->Thread);
  }
#elif _WIN32
  I->Thread = CreateThread(NULL,0,ThreadProc_AcceptSend,TS,0,&son);
#endif
}
#ifdef _WIN32
DWORD WINAPI ThreadProc_AcceptSend(void *Info)
#elif __unix__
void *ThreadProc_AcceptSend(void *Info)
#endif
{
  BN_PTempoStruct TS;
  BN_PInfo I;
  BN_PSend Send;

  TS = (BN_PTempoStruct)Info;
  I = TS->I;
  Send = TS->Send;
  free(TS);

  if((Send->Flags & PROCESS_KEEP_SIG) == 0)
    BN_UnsetSigs(Send->Flags & PROCESS_NEW_PROCESS);
  if(((Send->Flags & PROCESS_KEEP_EVENT) == 0) && ((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    BN_UnsetEvents(I,false);
  if(((Send->Flags & PROCESS_KEEP_SOCKET) == 0) && ((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS))
    CLOSE_SOCKET_FUNC(I->Socket);

  Send->Status = SEND_ACTIVE;
  if(!BN_CreateSendSocket(Send,Send->Addr,Send->Port))
  {
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  if(connect(Send->Socket, (struct sockaddr *)&Send->SAddr, sizeof(Send->SAddr)) == -1)
  {
    CLOSE_SOCKET_FUNC(Send->Socket);
    free(Send);
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  BN_PrintDebug(2,"DCC connected with %s\n",Send->Nick);
  BN_CreateDCCGetProcess(I,Send);
  return 0;
}

void BN_CloseDCCSend(BN_PInfo I,BN_PSend Send)
{
  int Flags;
  Send->Status = SEND_CLOSED;
  CLOSE_SOCKET_FUNC(Send->Socket);
  Send->Socket = 0;
  Flags = Send->Flags;
  free(Send);
  if((Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
    EXIT_PROCESS_FUNC(1);
  else
    EXIT_THREAD_FUNC 1);
}

void BN_CreateDCCSendProcess(BN_PInfo I,BN_PSend Send,const int TimeOut)
{
  char *S;
  int sock;
  int len;
  FILE *File;
  int res;
  unsigned int sent;
  unsigned int received;
  char Buffer[DCC_PACKETSIZE];
  fd_set rfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&rfds);
  FD_SET(Send->Socket,&rfds);
  tv.tv_sec = TimeOut;
  tv.tv_usec = 0;
  retval = select(Send->Socket+1,&rfds,NULL,NULL,&tv);
  if(!retval)
  {
    BN_PrintDebug(2,"DCC send not taken by %s (timed out %d)\n",Send->Nick,TimeOut);
    CLOSE_SOCKET_FUNC(Send->Socket);
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  len = sizeof(Send->SAddr);
  sock = accept(Send->Socket,(struct sockaddr *)&Send->SAddr,&len);
  if(sock == -1)
  {
    printf("Error accept\n");
    if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else
      EXIT_THREAD_FUNC 1);
  }
  CLOSE_SOCKET_FUNC(Send->Socket);
  Send->Socket = sock;
  Send->Status = SEND_ACTIVE;
  S = inet_ntoa(Send->SAddr.sin_addr);
  strncpy(Send->Addr,S,sizeof(Send->Addr)-1);
  Send->Addr[sizeof(Send->Addr)-1] = 0;

  if(Send->CB.OnDCCSendOpened != NULL)
    Send->CB.OnDCCSendOpened(I,Send);

  File = fopen(Send->Filename,"rb");

  if(File != NULL)
  {
    sent = 0;
    received = 0;
    while(sent < Send->Length)
    {
      len = fread(Buffer,1,DCC_PACKETSIZE,File);
      FD_ZERO(&rfds);
      FD_SET(Send->Socket,&rfds);
      tv.tv_sec = BN_TIMEOUT_TRANSFER;
      tv.tv_usec = 0;
      retval = select(Send->Socket+1,NULL,&rfds,NULL,&tv);
      if(!retval)
      {
        fclose(File);
        if(Send->CB.OnDCCSendClosed != NULL)
          Send->CB.OnDCCSendClosed(I,Send,DCC_FAILED);
        BN_PrintDebug(2,"DCC send failed (timeout)\n");
        BN_CloseDCCSend(I, Send);
        return;
      }
      res = send(Send->Socket, Buffer, len, 0);
      if(res != len)
      {
        fclose(File);
        if(Send->CB.OnDCCSendClosed != NULL)
          Send->CB.OnDCCSendClosed(I,Send,DCC_FAILED);
        BN_PrintDebug(2,"DCC send failed (send error)\n");
        BN_CloseDCCSend(I, Send);
        return;
      }
      if(Send->CB.OnDCCSendPacket != NULL)
        Send->CB.OnDCCSendPacket(I,Send,len);
      sent+=len;
      /* Wait for ACK */
      do
      {
        FD_ZERO(&rfds);
        FD_SET(Send->Socket,&rfds);
        tv.tv_sec = BN_TIMEOUT_TRANSFER;
        tv.tv_usec = 0;
        retval = select(Send->Socket+1,&rfds,NULL,NULL,&tv);
        if(!retval)
        {
          fclose(File);
          if(Send->CB.OnDCCSendClosed != NULL)
            Send->CB.OnDCCSendClosed(I,Send,DCC_FAILED);
          BN_PrintDebug(2,"DCC send failed (timeout)\n");
          BN_CloseDCCSend(I, Send);
          return;
        }
        len = recv(Send->Socket, Buffer, 4, 0);
        received = (Buffer[0] * 256 * 256 * 256) + (Buffer[1] * 256 * 256) + (Buffer[2] * 256) + Buffer[3];
      } while(received < sent);
    }
    fclose(File);
    if(Send->CB.OnDCCSendClosed != NULL)
      Send->CB.OnDCCSendClosed(I,Send,DCC_COMPLETED);
    BN_PrintDebug(2,"DCC send completed\n");
  }
  else
  {
    /* Sending file failed */
    Send->Length=0;
    if(Send->CB.OnDCCSendClosed != NULL)
      Send->CB.OnDCCSendClosed(I,Send,DCC_FAILED);
    BN_PrintDebug(2,"DCC send failed (can't open file)\n");
  }
  BN_CloseDCCSend(I, Send);
}

void BN_CreateDCCGetProcess(BN_PInfo I,BN_PSend Send)
{
  char *S;
  int sock;
  FILE *File;
  int res;
  unsigned int len;
  unsigned int received;
  unsigned int overflow;
  char Buffer[DCC_PACKETSIZE];
  fd_set rfds;
  struct timeval tv;
  int retval;

  while(Send->Status == SEND_REQUEST)
  {
    len = sizeof(Send->SAddr);
    sock = accept(Send->Socket,(struct sockaddr *)&Send->SAddr,&len);
    if(sock == -1)
    {
      printf("Error accept\n");
      if((Send->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
        EXIT_PROCESS_FUNC(1);
      else
        EXIT_THREAD_FUNC 1);
    }
    CLOSE_SOCKET_FUNC(Send->Socket);
    Send->Socket = sock;
    Send->Status = SEND_ACTIVE;
    S = inet_ntoa(Send->SAddr.sin_addr);
    strncpy(Send->Addr,S,sizeof(Send->Addr)-1);
    Send->Addr[sizeof(Send->Addr)-1] = 0;
  }

  if(Send->CB.OnDCCGetOpened != NULL)
    Send->CB.OnDCCGetOpened(I,Send);

  File = fopen(Send->Filename,"wb");

  if(File != NULL)
  {
    received = 0;
    while(Send->Length != received)
    {
      FD_ZERO(&rfds);
      FD_SET(Send->Socket,&rfds);
      tv.tv_sec = BN_TIMEOUT_TRANSFER;
      tv.tv_usec = 0;
      retval = select(Send->Socket+1,&rfds,NULL,NULL,&tv);
      if(!retval)
      {
        fclose(File);
        if(Send->CB.OnDCCGetClosed != NULL)
          Send->CB.OnDCCGetClosed(I,Send,DCC_FAILED);
        BN_PrintDebug(2,"DCC get failed (timeout)\n");
        BN_CloseDCCSend(I, Send);
        return;
      }
      len = recv(Send->Socket, Buffer, DCC_PACKETSIZE, 0);
      if(len > 0)
      {
        if(Send->CB.OnDCCGetPacket != NULL)
          Send->CB.OnDCCGetPacket(I,Send,len);
        fwrite(Buffer,1,len,File);
        /* Send ack */
        received+=len;
        Buffer[0] = received / (256 * 256 * 256);
        overflow = Buffer[0] * (256 * 256 * 256);
        Buffer[1] = (received - overflow) / (256 * 256);
        overflow += Buffer[1] * (256 * 256);
        Buffer[2] = (received - overflow) / (256);
        overflow += Buffer[2] * (256);
        Buffer[3] = received  - overflow;
        FD_ZERO(&rfds);
        FD_SET(Send->Socket,&rfds);
        tv.tv_sec = BN_TIMEOUT_TRANSFER;
        tv.tv_usec = 0;
        retval = select(Send->Socket+1,NULL,&rfds,NULL,&tv);
        if(!retval)
        {
          fclose(File);
          if(Send->CB.OnDCCGetClosed != NULL)
            Send->CB.OnDCCGetClosed(I,Send,DCC_FAILED);
          BN_PrintDebug(2,"DCC get failed (timeout)\n");
          BN_CloseDCCSend(I, Send);
          return;
        }
        res = send(Send->Socket, Buffer, 4, 0);
        if(res != 4)
        {
          fclose(File);
          if(Send->CB.OnDCCGetClosed != NULL)
            Send->CB.OnDCCGetClosed(I,Send,DCC_FAILED);
          BN_PrintDebug(2,"DCC get failed (send error)\n");
          BN_CloseDCCSend(I, Send);
          return;
        }
      }
      else
      {
        fclose(File);
        if(Send->CB.OnDCCGetClosed != NULL)
          Send->CB.OnDCCGetClosed(I,Send,DCC_FAILED);
        BN_PrintDebug(2,"DCC get failed (recv error)\n");
        BN_CloseDCCSend(I, Send);
        return;
      }
    }
    fclose(File);
    if(Send->CB.OnDCCGetClosed != NULL)
      Send->CB.OnDCCGetClosed(I,Send,DCC_COMPLETED);
    BN_PrintDebug(2,"DCC get completed\n");
  }
  else
  {
    /* Sending file failed */
    Send->Length = 0;
    if(Send->CB.OnDCCGetClosed != NULL)
      Send->CB.OnDCCGetClosed(I,Send,DCC_FAILED);
    BN_PrintDebug(2,"DCC send failed (can't open file)\n");
  }
  BN_CloseDCCSend(I, Send);
}

void BN_RejectDCCSend(BN_PSend Send)
{
  free(Send);
}

