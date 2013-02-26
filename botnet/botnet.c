/* Bot Net Main file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#include "includes.h"

struct BN_threadspecific {
  char tmp_tok[1024]; /* String for strtok_r, 1024 should be enough, message are not supposed to be > 512 */
  char **BN_ChanList, **BN_TopicList, **BN_ChanUsers;
  char **BN_BanList;
  char **BN_NameList;
  char ***BN_WhoisInfo;
  char **BN_WhoList;
  int BN_ChanCount,BN_TopicCount,BN_ChanUsersCount,BN_WhoisCount,BN_NameCount,BN_WhoCount,BN_BanCount;
#ifdef SERVER
  char **BN_NJoinList;
  int BN_NJoinCount;
#endif
};
#ifdef _WIN32
DWORD BN_tskey;
DWORD BN_once = 0;
#elif __unix__
pthread_key_t BN_tskey;
/* this is wrong!! why won't it compile without braces? */
pthread_once_t BN_once = {PTHREAD_ONCE_INIT};
#endif

#ifdef _WIN32
bool BN_WSAInit = false;
DWORD WINAPI ThreadProc_Connect(void *);
#elif __unix__
void *ThreadProc_Connect(void *);
#endif

#ifdef CONTEXT
int BN_segv;
void BN_handle_SIGNAL(int signal)
{
  if(signal == SIGSEGV)
  {
    if(BN_segv == 0)
    {
      BN_segv = 1;
      printf("Crashed :-(. Check: %s:%d\n", BN_ContextFile,BN_ContextLine);
    }
    else
      exit(1);
  }
}
#endif

char *BN_GetVersion(void)
{
  return BOTNET_VERSION;
}

char *BN_GetCopyright(void)
{
  return BOTNET_COPYRIGHT;
}

void BN_CreateWhoisInfo(const char *Nick);
void BN_FreeList(char **List,int C);
void BN_FreeWhoisInfos();
void BN_FreeWhoList(char *List[],int C);

void BN_destroyts(void *ptr)
{
  struct BN_threadspecific *ts = (struct BN_threadspecific *)ptr;

  BN_FreeList(ts->BN_ChanList,ts->BN_ChanCount);
  BN_FreeList(ts->BN_TopicList,ts->BN_TopicCount);
  BN_FreeList(ts->BN_ChanUsers, ts->BN_ChanUsersCount);
  BN_FreeList(ts->BN_BanList,ts->BN_BanCount);
  BN_FreeList(ts->BN_NameList,ts->BN_NameCount);
  BN_FreeWhoisInfos();
  BN_FreeWhoList(ts->BN_WhoList,ts->BN_WhoCount);
#ifdef SERVER
  BN_FreeList(ts->BN_NJoinList,ts->BN_NJoinCount);
#endif
  free(ptr);
}

void BN_tsinitkey(void)
{
#ifdef _WIN32
  if(BN_once == 0)
  {
    BN_tskey = TlsAlloc();
    BN_once++;
  }
#else
  pthread_key_create(&BN_tskey, BN_destroyts);
#endif
}

struct BN_threadspecific *BN_getthreadspecific(void)
{
  struct BN_threadspecific *ts;

#ifdef _WIN32
  BN_tsinitkey();
  ts = TlsGetValue(BN_tskey);
  if(ts == NULL)
  {
    ts=malloc(sizeof(struct BN_threadspecific));
    memset(ts, 0, sizeof(struct BN_threadspecific));
    TlsSetValue(BN_tskey,ts);
  }
#elif __unix__
  pthread_once(&BN_once, BN_tsinitkey);
  ts=pthread_getspecific(BN_tskey);
  if(!ts)
  {
    ts=malloc(sizeof(struct BN_threadspecific));
    memset(ts, 0, sizeof(struct BN_threadspecific));
    pthread_setspecific(BN_tskey, ts);
  }
#endif
  return ts;
}

bool BN_Connect(BN_PInfo I,const char *Server,const int Port,const int Flags)
{
  int ret;
#ifdef _WIN32
  DWORD son;
  WSADATA wsaData;
  if(!BN_WSAInit)
  {
    if(WSAStartup(0x0101,&wsaData) != 0)
      return false;
    BN_WSAInit = true;
  }
#endif

#ifdef SERVER
  I->IsServer = false;
  I->Serv[0] = 0;
#endif
  I->Flags = Flags;
  I->Server = strdup(Server);
  I->Port = Port;
#ifdef __unix__
  if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
  {
    if(fork() != 0)
      return true;
  }
  else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
  {
    if(pthread_create(&I->Thread,NULL,&ThreadProc_Connect,I) != 0)
      return false;
    return true;
  }
#elif _WIN32
  if((I->Flags & (PROCESS_NEW_PROCESS | PROCESS_NEW_THREAD)) != 0)
  {
    if((I->Thread = CreateThread(NULL,0,ThreadProc_Connect,I,0,&son)) == NULL)
      return false;
    return true;
  }
#endif
  ret = (int)ThreadProc_Connect(I);
  return (bool)ret;
}
#ifdef _WIN32
DWORD WINAPI ThreadProc_Connect(void *Info)
#elif __unix__
void *ThreadProc_Connect(void *Info)
#endif
{
  struct sockaddr_in SAddr;
  struct hostent *HostAddr;
  BN_PMessage Msg;
  BN_PInfo I;

  I = (BN_PInfo)Info;
  if(((I->Flags & PROCESS_KEEP_SIG) == 0) && ((I->Flags & (PROCESS_NEW_PROCESS | PROCESS_NEW_THREAD)) != 0))
    BN_UnsetSigs(true);

#ifdef CONTEXT
  BN_segv = 0;
  signal(SIGSEGV, BN_handle_SIGNAL);
  context;
#endif

  if(I->Socket != 0)
    CLOSE_SOCKET_FUNC(I->Socket);
  I->Socket = 0;
  I->BufPos = 0;
  I->Buf[0] = 0;
  I->Nick[0] = 0;
  I->User = 0;
  I->StampPos = 0;
  I->NbStamps = 0;

  I->Socket = socket(AF_INET, SOCK_STREAM, getprotobyname("tcp")->p_proto);
  if(I->Socket == -1)
  {
    if(I->CB.OnError != NULL)
      I->CB.OnError(I,errno);
    if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
      EXIT_THREAD_FUNC 1);
    else
      return false;
  }
  BN_PrintDebug(1,"Socket created\n");
  memset(&SAddr, 0, sizeof(SAddr));
  SAddr.sin_port = htons(I->Port);
  SAddr.sin_family = AF_INET;
  if(inet_addr(I->Server) != INADDR_NONE)
  {
    SAddr.sin_addr.s_addr = inet_addr(I->Server);
  }
  else
  {
    HostAddr = (struct hostent *)gethostbyname(I->Server);
    if(HostAddr == NULL)
    {
      if(I->CB.OnError != NULL)
        I->CB.OnError(I,99);
      if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
        EXIT_PROCESS_FUNC(1);
      else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
        EXIT_THREAD_FUNC 1);
      else
        return false;
    }
    memcpy((void *)&SAddr.sin_addr, HostAddr->h_addr, HostAddr->h_length);
  }
  if(connect(I->Socket, (struct sockaddr *)&SAddr, sizeof(SAddr)) == -1)
  {
    if(I->CB.OnError != NULL)
      I->CB.OnError(I,errno);
    if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
      EXIT_THREAD_FUNC 1);
    else
      return false;
  }
  if(I->CB.OnConnected != NULL)
    I->CB.OnConnected(I,I->Server);
  BN_PrintDebug(1,"Connected to \n",I->Server);

  while(true)
  {
    Msg = BN_WaitForData(I);
    if(Msg == NULL)
      break;
    BN_HandleMessage(I,Msg);
    BN_FreeMessage(Msg);
  }
  return false;
}

void BN_Disconnect(BN_PInfo I)
{
  CLOSE_SOCKET_FUNC(I->Socket);
  I->Socket = 0;
}

BN_PMessage BN_WaitForData(BN_PInfo I)
{
  int len,i;
  char *buf,*pos,*cr,*lf;
  BN_PMessage Msg;
  int blocking=0;
  int ajout=2;
  fd_set rfds;
  struct timeval tv;
  int retval;
#ifdef SERVER
  char tmp[512];
#endif

  while(true)
  {
    cr = strstr(I->Buf,"\r");
    lf = strstr(I->Buf,"\n");
    if((cr == NULL) && (lf == NULL))
      pos = NULL;
    else if((cr != NULL) && (lf != NULL))
    {
      if(abs(cr-lf) != 1)
        ajout = 1;
      if(cr < lf)
        pos = cr;
      else
        pos = lf;
    }
    else
    {
      ajout = 1;
      if(cr != NULL)
        pos = cr;
      else
        pos = lf;
    }
    if(pos != NULL) /* if there is a new command */
    {
      len = pos - I->Buf;
      buf = (char *)malloc(len+1);
#ifdef MEMCPY_CHECK
      if((len > BN_BUFFERSIZE) || (len < 0))
        printf("ERROR IN MEMCPY N° 1 (%d)\n",len);
#endif
      memcpy(buf,I->Buf,len);
      buf[len] = 0;
#ifdef MEMCPY_CHECK
      if(((I->BufPos-len-ajout) > BN_BUFFERSIZE) || ((I->BufPos-len-ajout) < 0))
        printf("ERROR IN MEMCPY N° 2 (%d %d %d)\n",I->BufPos,len,ajout);
#endif
      memmove(I->Buf,pos+ajout,I->BufPos-len-ajout);
      I->BufPos -= (len+ajout);
      I->Buf[I->BufPos] = 0;

      /* Updating time stamps */
      I->Time = I->Stamps[I->StampPos];
      while(((len+ajout) >= I->StampLen[I->StampPos]) && (I->NbStamps != 0))
      {
        I->StampPos = (I->StampPos + 1) % BN_STAMPSIZE;
        I->NbStamps--;
      }
      if(I->NbStamps != 0)
      {
        for(i=0;i<I->NbStamps;i++)
          I->StampLen[(I->StampPos+i)%BN_STAMPSIZE] -= len+ajout;
      }
      BN_PrintDebug(3,"(%s - %ld) : string found (%d octets) (%s)\n",GetTimeStamp(),I->Time,len,buf);

      if(len != 0)
      {
        Msg = BN_ParseMessage(buf);
        free(buf);
        if(Msg == NULL)
        {
          BN_PrintDebug(0,"WARNING, MALFORMED MESSAGE\n");
          continue;
        }
        else
          break;
      }
      else
      {
        free(buf);
        continue;
      }
    }
    else if((BN_BUFFERSIZE-I->BufPos) >= 0) /* Else if not a full command */
    {
      FD_ZERO(&rfds);
      FD_SET(I->Socket,&rfds);
#ifdef SERVER
      if(I->IsServer)
        tv.tv_sec = I->PingDelay;
      else
#endif
      tv.tv_sec = BN_SOCKET_TIMEOUT;
      tv.tv_usec = 0;
      retval = select(I->Socket+1,&rfds,NULL,NULL,&tv);
      if(!retval)
      {
#ifdef SERVER
        if(I->IsServer)
        {
          /* Time to PING remote server */
          snprintf(tmp,sizeof(tmp),"PING :%s%c%c",I->Serv,13,10);
          if(send(I->Socket,tmp,strlen(tmp),0) != strlen(tmp))
          {
            /* Inform other servers that a server has quit */
            if(I->SCB.OnForeignSQuit != NULL)
              I->SCB.OnForeignSQuit(I,I->Serv,I->Server,"Write error");
            /* Write error */
            BN_ServerSendErrorMessage(I,"Write error"); /* This function exits */
          }
          FD_ZERO(&rfds);
          FD_SET(I->Socket,&rfds);
          tv.tv_sec = I->PingDelay;
          tv.tv_usec = 0;
          retval = select(I->Socket+1,&rfds,NULL,NULL,&tv);
          if(!retval)
          {
            /* Inform other servers that a server has quit */
            if(I->SCB.OnForeignSQuit != NULL)
              I->SCB.OnForeignSQuit(I,I->Serv,I->Server,"Ping timeout");
            /* Time out */
            BN_ServerSendErrorMessage(I,"Ping timeout"); /* This function exits */
          }
        }
        else
        {
#endif
        if(I->CB.OnError != NULL)
#ifdef _WIN32
          I->CB.OnError(I,110);
#else
          I->CB.OnError(I,ETIMEDOUT);
#endif
        BN_Disconnect(I);
        if(I->CB.OnDisconnected != NULL)
          I->CB.OnDisconnected(I,"Socket timed out");
        if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
          EXIT_PROCESS_FUNC(1);
        else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
          EXIT_THREAD_FUNC 1);
        else
          return NULL;
#ifdef SERVER
        }
#endif
      }
      len = recv(I->Socket,I->Buf+I->BufPos,BN_BUFFERSIZE-I->BufPos-1,0);

      /* Setting time stamp for received message */
      I->Stamps[(I->StampPos+I->NbStamps)%BN_STAMPSIZE] = time(NULL);
      I->StampLen[(I->StampPos+I->NbStamps)%BN_STAMPSIZE] = I->BufPos+len;
      I->NbStamps++;

#ifdef __unix__
      waitpid(-1,NULL,WNOHANG); // We kill dcc chat (if any defunct waiting)
#endif
      BN_SendMessage(I,NULL,0); // Using NULL message will empty message queue

      if(len == 0)
      {
        blocking++;
        if(blocking >= 10)
        {
          if(I->CB.OnError != NULL)
            I->CB.OnError(I,errno);
          BN_Disconnect(I);
          if(I->CB.OnDisconnected != NULL)
            I->CB.OnDisconnected(I,"Process in infinite loop...auto killing");
          if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
            EXIT_PROCESS_FUNC(1);
          else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
            EXIT_THREAD_FUNC 1);
          else
            return NULL;
        }
      }
      if( len == -1)
      {
        if(I->Socket == 0)
        {
          if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
            EXIT_PROCESS_FUNC(1);
          else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
            EXIT_THREAD_FUNC 1);
          else
            return NULL;
        }
        if(I->CB.OnError != NULL)
          I->CB.OnError(I,errno);
        BN_Disconnect(I);
        if(I->CB.OnDisconnected != NULL)
          I->CB.OnDisconnected(I,"Read error on socket");
        if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
          EXIT_PROCESS_FUNC(1);
        else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
          EXIT_THREAD_FUNC 1);
        else
          return NULL;
      }
      I->Buf[I->BufPos+len] = 0;
      I->BufPos += len;
    }
  }
  return Msg;
}

BN_PMessage BN_ParseMessage(const char *Message)
{
  BN_PMessage Msg;
  int Pos,i;
  char *buf;
#ifdef NON_STANDARD
  char *p;
#endif

  Msg = (BN_PMessage)malloc(sizeof(BN_TMessage));
  for(i=0;i<MAX_PARAMS;i++)
    Msg->Params[i] = "";
  Pos = 0;

  if(Message[0] == ':')
  {
    buf = strstr(Message," ");
    Pos = buf - Message - 1;
    Msg->Prefix = (char *)malloc(Pos+1);
#ifdef MEMCPY_CHECK
    if((Pos > BN_BUFFERSIZE) || (Pos < 0))
      printf("ERROR IN MEMCPY N° 3 (%d) : %s\n",Pos,Message);
#endif
    memcpy(Msg->Prefix,Message+1,Pos);
    Msg->Prefix[Pos] = 0;
    Message = Message + Pos + 1;
  }
  else
    Msg->Prefix = NULL;

  Message = TrimLeft(Message);

  buf = strstr(Message," ");
  if(buf == NULL)
  {
    free(Msg);
    return NULL;
  }
  Pos = buf - Message;
  Msg->Command = (char *)malloc(Pos+1);
#ifdef MEMCPY_CHECK
  if((Pos > BN_BUFFERSIZE) || (Pos < 0))
    printf("ERROR IN MEMCPY N° 4 (%d) : %s\n",Pos,Message);
#endif
  memcpy(Msg->Command,Message,Pos);
  Msg->Command[Pos] = 0;
  Message = Message + Pos + 1;

  Msg->NbParams = 0;
#ifdef NON_STANDARD
  p = strchr(Message,':');
  if(p == NULL)
    p = (char *)(Message + BN_BUFFERSIZE + 1);
  else
  {
    if(p[-1] != ' ')
      p[0] = ' ';
  }
#endif

  while(Message[0] != 0)
  {
    if(Msg->NbParams >= MAX_PARAMS)
    {
      printf("BOTNET ERROR : MAX_PARAMS is not enough here, please mailto:zekiller@skytech.org\n");
      break;
    }
#ifdef NON_STANDARD
    if(Message >= p)
#else
    if(Message[0] == ':')
#endif
    {
      Msg->Params[Msg->NbParams] = strdup(Message+1);
      Msg->NbParams++;
      break;
    }
    else
    {
      buf = strstr(Message," ");
      if(buf == NULL) // Last parameter
      {
        Msg->Params[Msg->NbParams] = strdup(Message);
        Msg->NbParams++;
        break;
      }
      else
      {
        Pos = buf - Message;
        Msg->Params[Msg->NbParams] = (char *)malloc(Pos+1);
#ifdef MEMCPY_CHECK
        if((Pos > BN_BUFFERSIZE) || (Pos < 0))
          printf("ERROR IN MEMCPY N° 5 (%d)\n",Pos);
#endif
        memcpy(Msg->Params[Msg->NbParams],Message,Pos);
        Msg->Params[Msg->NbParams][Pos] = 0;
        Message = Message + Pos + 1;
        while(Message[0] == ' ')
          Message++;
        Msg->NbParams++;
      }
    }
  }

  return Msg;
}

void BN_CTCPFonction(BN_PInfo I,BN_PMessage Msg)
{
  char *Reply;
  char S[1024];
  char Nick[BN_NICKLENGTH];
  char *p;
  char Ctcp[50];
  int len;

  p = strchr(Msg->Params[1]+1,' ');
  if(p == NULL)
    len = strlen(Msg->Params[1]+1) - 1; /* Less one, because of the \1 */
  else
    len = p - Msg->Params[1] - 1; /* Less one, because of the \1 */
  if(len >= sizeof(Ctcp))
    len = sizeof(Ctcp) - 1;
  strncpy(Ctcp,Msg->Params[1]+1,len);
  Ctcp[len] = 0;

  if(strcmp(Ctcp,CTCP_BOTNET) == 0)
  {
    BN_ExtractNick(Msg->Prefix,Nick,sizeof(Nick));
    snprintf(S,sizeof(S),"%s :%c%s %s",Nick,1,Ctcp,BOTNET_COPYRIGHT);
#ifdef SERVER
    if(I->IsServer)
      BN_SendMessage(I,BN_MakeMessage(I->Serv,"NOTICE",S),BN_LOW_PRIORITY);
    else
#endif
    BN_SendMessage(I,BN_MakeMessage(NULL,"NOTICE",S),BN_LOW_PRIORITY);
    return;
  }

  Reply = NULL;
  if(I->CB.OnCTCP != NULL)
    Reply = I->CB.OnCTCP(I,Msg->Prefix,Msg->Params[0],Ctcp);

  BN_ExtractNick(Msg->Prefix,Nick,sizeof(Nick));
  if(Reply == NULL)
  {
    if(strcmp(Ctcp,CTCP_VERSION) == 0)
      snprintf(S,sizeof(S),"%s :%c%s %s",Nick,1,Ctcp,BOTNET_COPYRIGHT);
    else
      snprintf(S,sizeof(S),"%s :%c%s No answer",Nick,1,Ctcp);
  }
  else
  {
    snprintf(S,sizeof(S),"%s :%c%s %s",Nick,1,Ctcp,Reply);
    free(Reply);
  }
#ifdef SERVER
  if(I->IsServer)
    BN_SendMessage(I,BN_MakeMessage(I->Serv,"NOTICE",S),BN_LOW_PRIORITY);
  else
#endif
  BN_SendMessage(I,BN_MakeMessage(NULL,"NOTICE",S),BN_LOW_PRIORITY);
}

void BN_AddStringToList(char ***List,char S[],int *C)
{
  int c;
  char **l;

  c = *C;
  l = *List;

  c++;
  if(c == 1)
    l = (char **)malloc(sizeof(char *));
  else
    l = (char **)realloc(l,c*sizeof(char *));
  l[c-1] = strdup(S);

  *C = c;
  *List = l;
}

void BN_FreeList(char **List,int C)
{
  int i;
  for(i=0;i<C;i++)
    free(List[i]);
  if(C != 0)
    free(List);
}

void BN_FreeWhoList(char *List[],int C)
{
  int i,j;
  for(i=0;i<(C*WHO_INFO_COUNT);i+=WHO_INFO_COUNT)
  {
    for(j=0;j<WHO_INFO_COUNT;j++)
      free(List[i+j]);
  }
  if(C != 0)
    free(List);
}

void BN_AddInfoToWho(char *Params[])
{
  int i,len;
  struct BN_threadspecific *ts;

  ts=BN_getthreadspecific();
  ts->BN_WhoCount++;
  if(ts->BN_WhoCount == 1)
    ts->BN_WhoList = (char **)malloc(sizeof(char *)*WHO_INFO_COUNT);
  else
    ts->BN_WhoList = (char **)realloc(ts->BN_WhoList,sizeof(char *)*WHO_INFO_COUNT*ts->BN_WhoCount);
  i = (ts->BN_WhoCount - 1) * WHO_INFO_COUNT;
  ts->BN_WhoList[i+WHO_USERNAME] = strdup(Params[2]);
  ts->BN_WhoList[i+WHO_NICKNAME] = strdup(Params[5]);
  ts->BN_WhoList[i+WHO_HOSTNAME] = strdup(Params[3]);
  ts->BN_WhoList[i+WHO_SERVER] = strdup(Params[4]);
  ts->BN_WhoList[i+WHO_FLAGS] = strdup(Params[6]);
  len = strlen(Params[7]);
  if(len >= 2)
    ts->BN_WhoList[i+WHO_REALNAME] = strdup(Params[7]+2);
  else
    ts->BN_WhoList[i+WHO_REALNAME] = strdup(Params[7]);
}

void BN_AddInfoToWhois(const char *Nick,char *S,int Info)
{
  int i;
  struct BN_threadspecific *ts;

  ts=BN_getthreadspecific();
  for(i=0;i<ts->BN_WhoisCount;i++)
  {
    if(strcmp(ts->BN_WhoisInfo[i][WHOIS_NICKNAME],Nick) == 0)
    {
      ts->BN_WhoisInfo[i][Info] = strdup(S);
      break;
    }
  }
}

int BN_SearchWhoisInfo(const char *Nick)
{
  int i;
  struct BN_threadspecific *ts;

  ts=BN_getthreadspecific();
  for(i=0;i<ts->BN_WhoisCount;i++)
  {
    if(strcmp(ts->BN_WhoisInfo[i][WHOIS_NICKNAME],Nick) == 0)
      return i;
  }
  BN_CreateWhoisInfo(Nick);
  return 0;
}

void BN_CreateWhoisInfo(const char *Nick)
{
  int i;
  struct BN_threadspecific *ts;

  ts=BN_getthreadspecific();
  ts->BN_WhoisCount++;
  if(ts->BN_WhoisCount == 1)
    ts->BN_WhoisInfo = (char ***)malloc(sizeof(char **));
  else
    ts->BN_WhoisInfo = (char ***)realloc(ts->BN_WhoisInfo,ts->BN_WhoisCount*sizeof(char**));
  ts->BN_WhoisInfo[ts->BN_WhoisCount-1] = (char **)malloc(WHOIS_INFO_COUNT*sizeof(char *));
  for(i=0;i<WHOIS_INFO_COUNT;i++)
    ts->BN_WhoisInfo[ts->BN_WhoisCount-1][i] = NULL;
  ts->BN_WhoisInfo[ts->BN_WhoisCount-1][WHOIS_NICKNAME] = strdup(Nick);
}

void BN_FreeWhoisInfos()
{
  int i,j;
  struct BN_threadspecific *ts;

  ts=BN_getthreadspecific();
  for(i=0;i<ts->BN_WhoisCount;i++)
  {
    if(ts->BN_WhoisInfo[i] != NULL)
    {
      for(j=0;j<WHOIS_INFO_COUNT;j++)
        if(ts->BN_WhoisInfo[i][j] != NULL)
          free(ts->BN_WhoisInfo[i][j]);
      free(ts->BN_WhoisInfo[i]);
    }
  }
  if(ts->BN_WhoisCount != 0)
  {
    free(ts->BN_WhoisInfo);
    ts->BN_WhoisCount = 0;
  }
}

void BN_ParseStringList(char *S,char ***List,int *Count,char *delim,char *tmp_tok)
{
  char *tok;

  context;
#ifdef HAVE_STRTOK_R
  tok = strtok_r(S,delim,&tmp_tok);
#else
  tok = strtok(S,delim);
#endif
  while(tok != NULL)
  {
    BN_AddStringToList(List,tok,Count);
#ifdef HAVE_STRTOK_R
    tok = strtok_r(NULL,delim,&tmp_tok);
#else
    tok = strtok(NULL,delim);
#endif
  }
}

void BN_HandleMessage(BN_PInfo I,BN_PMessage Msg)
{
  char Tmp[1024];
  time_t tm;
  struct BN_threadspecific *ts;
  int i;

  ts=BN_getthreadspecific();

  if(STRICMP_FUNC(Msg->Command,"PING") == 0)
  {
    BN_SendMessage(I,BN_MakeMessage(NULL,"PONG",Msg->Params[0]),BN_HIGH_PRIORITY);
    if(I->CB.OnPingPong != NULL)
      I->CB.OnPingPong(I);
  }
  else if(atoi(Msg->Command) != 0 )
  {
    int Code = atoi(Msg->Command);
    BN_PrintDebug(2,"Found a %d reply from server\n",Code);
    switch(Code)
    {
      case RPL_LIST      :
        BN_AddStringToList(&ts->BN_ChanList,Msg->Params[1],&ts->BN_ChanCount);
        BN_AddStringToList(&ts->BN_ChanUsers,Msg->Params[2],&ts->BN_ChanUsersCount);
        BN_AddStringToList(&ts->BN_TopicList,Msg->Params[3],&ts->BN_TopicCount);
      break;
      case RPL_LISTEND   :
        if(I->CB.OnList != NULL)
          I->CB.OnList(I,(const char **)ts->BN_ChanList,
                         (const char **)ts->BN_ChanUsers,
                         (const char **)ts->BN_TopicList, ts->BN_ChanCount);
        BN_FreeList(ts->BN_ChanList,ts->BN_ChanCount);
        BN_FreeList(ts->BN_TopicList,ts->BN_TopicCount);
        BN_FreeList(ts->BN_ChanUsers, ts->BN_ChanUsersCount);
        ts->BN_ChanCount = 0; ts->BN_TopicCount = 0; ts->BN_ChanUsersCount = 0;
        break;
      case RPL_WHOISUSER     : BN_CreateWhoisInfo(Msg->Params[1]);BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_USERNAME);BN_AddInfoToWhois(Msg->Params[1],Msg->Params[3],WHOIS_HOSTNAME);BN_AddInfoToWhois(Msg->Params[1],Msg->Params[5],WHOIS_REALNAME); break;
      case RPL_WHOISSERVER   : BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_SERVER); break;
      case RPL_WHOISOPERATOR : BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_OPERATOR); break;
      case RPL_WHOISIDLE     : BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_IDLE); break;
      case ERR_NOSUCHNICK    : BN_CreateWhoisInfo(Msg->Params[1]);break;
      case RPL_ENDOFWHOIS    :
        for(i=0;i<ts->BN_WhoisCount;i++)
        {
          if(I->CB.OnWhois != NULL)
            I->CB.OnWhois(I,(const char **)ts->BN_WhoisInfo[i]);
        }
        BN_FreeWhoisInfos();
        break;
      case RPL_WHOISCHANNELS : BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_CHANNELS); break;
      case RPL_AWAY          : BN_AddInfoToWhois(Msg->Params[1],Msg->Params[2],WHOIS_AWAY); break;
      case 1: if(I->CB.OnRegistered != NULL) I->CB.OnRegistered(I);BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));if(I->CB.OnStatus != NULL) I->CB.OnStatus(I,Tmp,Code); break;
      case RPL_ENDOFNAMES: if(I->CB.OnNames != NULL) I->CB.OnNames(I,Msg->Params[1],(const char **)ts->BN_NameList,ts->BN_NameCount);BN_FreeList(ts->BN_NameList,ts->BN_NameCount);ts->BN_NameCount=0;break;
      case RPL_NAMREPLY: BN_ParseStringList(Msg->Params[3],&ts->BN_NameList,&ts->BN_NameCount," ",ts->tmp_tok);break;
      case RPL_ENDOFWHO: if(I->CB.OnWho != NULL) I->CB.OnWho(I,Msg->Params[1],(const char **)ts->BN_WhoList,ts->BN_WhoCount);BN_FreeWhoList(ts->BN_WhoList,ts->BN_WhoCount);ts->BN_WhoCount=0;break;
      case RPL_WHOREPLY: BN_AddInfoToWho(Msg->Params);break;
      case RPL_BANLIST      : BN_AddStringToList(&ts->BN_BanList,Msg->Params[2],&ts->BN_BanCount); break;
      case RPL_ENDOFBANLIST : if(I->CB.OnBanList != NULL) I->CB.OnBanList(I,Msg->Params[1],(const char **)ts->BN_BanList,ts->BN_BanCount);BN_FreeList(ts->BN_BanList,ts->BN_BanCount);ts->BN_BanCount=0;break;
      case RPL_CHANNELMODEIS : BN_CreateStatusLine(Msg->Params,2,Msg->NbParams-1,Tmp,sizeof(Tmp));if(I->CB.OnModeIs != NULL) I->CB.OnModeIs(I,Msg->Params[1],Tmp);break;
      case 2:
      case 3:
      case 4:
      case RPL_LUSERCLIENT:
      case RPL_LUSERCHANNELS:
      case RPL_LUSERME:
      case RPL_MOTDSTART:
      case RPL_MOTD:
      case RPL_ENDOFMOTD:
      case ERR_ALREADYREGISTRED:
      case ERR_NICKNAMEINUSE : BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));if(I->CB.OnStatus != NULL) I->CB.OnStatus(I,Tmp,Code); break;
      case RPL_TOPIC : if(I->CB.OnTopicIs != NULL) I->CB.OnTopicIs(I,Msg->Params[1],Msg->Params[2]); break;
      case 333 : tm=atoi(Msg->Params[3]);if(I->CB.OnTopicSetBy != NULL) I->CB.OnTopicSetBy(I,Msg->Params[1],Msg->Params[2],(const char *)ctime(&tm)); break;
      default : BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));if(I->CB.OnStatus != NULL) I->CB.OnStatus(I,Tmp,Code); break;
    }
  }
  else if(STRICMP_FUNC(Msg->Command,"PRIVMSG") == 0)
  {
    BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(Msg->Params[0][0] == '#') // Message to channel
    {
      if(Msg->Params[1][0] == 1) // Special command
      {
        if(STRNICMP_FUNC(Msg->Params[1]+1,"ACTION",6) == 0) // Action message
        {
          Tmp[strlen(Tmp)-1] = 0; /* Remove #1 character */
          if(I->CB.OnAction != NULL)
            I->CB.OnAction(I,Msg->Params[0],Msg->Prefix,Tmp+8);
        }
        else
          BN_CTCPFonction(I,Msg);
      }
      else
      {
        if(I->CB.OnChannelTalk != NULL)
          I->CB.OnChannelTalk(I,Msg->Params[0],Msg->Prefix,Tmp);
      }
    }
    else if(Msg->Params[1][0] == 1) // Special command
    {
      if(STRNICMP_FUNC(Msg->Params[1]+1,"DCC",3) == 0) // DCC request
      {
        if(STRNICMP_FUNC(Msg->Params[1]+5,"CHAT",4) == 0)
        {
          BN_PChat Chat;
          char str[1000],*s;
          unsigned long int host;
          int port;

          Chat = (BN_PChat)malloc(sizeof(BN_TChat));
          Chat->Status = CHAT_REQUEST;
          BN_ExtractNick(Msg->Prefix,Chat->Nick,sizeof(Chat->Nick));
          BN_ExtractExactUserName(Msg->Prefix,Chat->UN,sizeof(Chat->UN));
          sscanf(Msg->Params[1]+10,"%999s %lu %d",str,&host,&port);
          Chat->Port = port;
          Chat->SAddr.sin_addr.s_addr = htonl(host);
          s = inet_ntoa(Chat->SAddr.sin_addr);
          strncpy(Chat->Addr,s,sizeof(Chat->Addr)-1);
          Chat->Addr[sizeof(Chat->Addr)-1] = 0;
          memcpy(&Chat->CB,&I->CB.Chat,sizeof(I->CB.Chat));
#ifdef SERVER
          strncpy(Chat->To,Msg->Params[0],sizeof(Chat->To)-1);
          Chat->To[sizeof(Chat->To)-1] = 0;
#endif
          BN_PrintDebug(2,"DCC CHAT REQUEST FOUND : %s %s\n",Chat->Nick,Chat->Addr);
          if(Chat->CB.OnDCCChatRequest != NULL)
            Chat->CB.OnDCCChatRequest(I,Chat);
          else
            BN_RejectDCCChat(Chat);
        }
        if(STRNICMP_FUNC(Msg->Params[1]+5,"SEND",4) == 0)
        {
          BN_PSend Send;
          char str[1000],*s;
          unsigned long int host,size;
          int port;

          Send = (BN_PSend)malloc(sizeof(BN_TSend));
          Send->Status = SEND_REQUEST;
          BN_ExtractNick(Msg->Prefix,Send->Nick,sizeof(Send->Nick));
          BN_ExtractExactUserName(Msg->Prefix,Send->UN,sizeof(Send->UN));
          sscanf(Msg->Params[1]+10,"%999s %lu %d %ld",str,&host,&port,&size);
          Send->Port = port;
          Send->Length = size;
          Send->SAddr.sin_addr.s_addr = htonl(host);
          s = inet_ntoa(Send->SAddr.sin_addr);
          strncpy(Send->Addr,s,sizeof(Send->Addr)-1);
          Send->Addr[sizeof(Send->Addr)-1] = 0;
          Send->Addr[BN_ADRSLENGTH-1] = 0;
          strncpy(Send->Filename, str,sizeof(Send->Filename)-1);
          Send->Filename[sizeof(Send->Filename)-1] = 0;
          memcpy(&Send->CB,&I->CB.Send,sizeof(I->CB.Send));
#ifdef SERVER
          strncpy(Send->To,Msg->Params[0],sizeof(Send->To)-1);
          Send->To[sizeof(Send->To)-1] = 0;
#endif
          BN_PrintDebug(2,"DCC SEND REQUEST FOUND : %s %s (%s:%ld)\n",Send->Nick,Send->Addr,Send->Filename,Send->Length);
          if(Send->CB.OnDCCSendRequest != NULL)
            Send->CB.OnDCCSendRequest(I, Send);
          else
            BN_RejectDCCSend(Send);
        }
      }
      else
        BN_CTCPFonction(I,Msg);
    }
    else // Private msg
    {
      if(I->CB.OnPrivateTalk != NULL)
        I->CB.OnPrivateTalk(I,Msg->Prefix,Msg->Params[0],Tmp);
    }
  }
  else if(STRICMP_FUNC(Msg->Command,"JOIN") == 0)
  {
    char Nick[BN_NICKLENGTH];

    BN_ExtractNick(Msg->Prefix,Nick,sizeof(Nick));
    if(strcmp(Nick,I->Nick) == 0)
    {
      if(I->CB.OnJoinChannel != NULL)
        I->CB.OnJoinChannel(I,Msg->Params[0]);
    }
    else
    {
      if(I->CB.OnJoin != NULL)
        I->CB.OnJoin(I,Msg->Params[0],Msg->Prefix);
    }
  }
  else if(STRICMP_FUNC(Msg->Command,"QUIT") == 0)
  {
    BN_CreateStatusLine(Msg->Params,0,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnQuit != NULL)
      I->CB.OnQuit(I,Msg->Prefix,Tmp);
  }
  else if(STRICMP_FUNC(Msg->Command,"KICK") == 0)
  {
    BN_CreateStatusLine(Msg->Params,2,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnKick != NULL)
      I->CB.OnKick(I,Msg->Params[0],Msg->Prefix,Msg->Params[1],Tmp);
  }
  else if(STRICMP_FUNC(Msg->Command,"NICK") == 0)
  {
    char Nick[BN_NICKLENGTH];

#ifdef SERVER
    if(Msg->Prefix == NULL)
    {
#ifdef BAHAMUT
      if(Msg->NbParams == 9)
      {
        if(I->SCB.OnNick != NULL)  // Nick HopCount TimeStamp UMode UserName HostName ServerName RealName
          I->SCB.OnNick(I,Msg->Params[0],atoi(Msg->Params[1]),atol(Msg->Params[2]),Msg->Params[3],Msg->Params[4],Msg->Params[5],Msg->Params[6],Msg->Params[8]);
      }
      else if(Msg->NbParams < 9)
        BN_ServerSendErrorMessage(I,"Not enough parameters for NICK command");
      else
        BN_ServerSendErrorMessage(I,"Too many parameters for NICK command");
#else
      if(Msg->NbParams == 7)
      {
        if(I->SCB.OnNick != NULL)
          I->SCB.OnNick(I,Msg->Params[0],atoi(Msg->Params[1]),Msg->Params[2],Msg->Params[3],atoi(Msg->Params[4]),Msg->Params[5],Msg->Params[6]);
      }
      else if(Msg->NbParams < 7)
        BN_ServerSendErrorMessage(I,"Not enough parameters for NICK command");
      else
        BN_ServerSendErrorMessage(I,"Too many parameters for NICK command");
#endif
    }
    else
    {
#endif
    BN_CreateStatusLine(Msg->Params,0,Msg->NbParams-1,Tmp,sizeof(Tmp));
    BN_ExtractNick(Msg->Prefix,Nick,sizeof(Nick));
    if(strcmp(Nick,I->Nick) == 0)
    {
      strncpy(I->Nick,Msg->Params[0],sizeof(I->Nick)-1);
      I->Nick[sizeof(I->Nick)-1] = 0;
    }
    if(I->CB.OnNick != NULL)
      I->CB.OnNick(I,Msg->Prefix,Tmp);
#ifdef SERVER
  }
#endif
  }
  else if(STRICMP_FUNC(Msg->Command,"NOTICE") == 0)
  {
    if(Msg->Params[1][0] == 1) // CTCP reply
    {
      BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
      if(I->CB.OnCTCPReply != NULL)
        I->CB.OnCTCPReply(I,Msg->Prefix,Msg->Params[0],Tmp);
    }
    else
    {
      BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
      if(I->CB.OnNotice != NULL)
        I->CB.OnNotice(I,Msg->Prefix,Msg->Params[0],Tmp);
    }
  }
  else if(STRICMP_FUNC(Msg->Command,"PART") == 0)
  {
    BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnPart != NULL)
      I->CB.OnPart(I,Msg->Params[0],Msg->Prefix,Tmp);
  }
  else if(STRICMP_FUNC(Msg->Command,"MODE") == 0)
  {
    BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnMode != NULL)
      I->CB.OnMode(I,Msg->Params[0],Msg->Prefix,Tmp);
  }
  else if(STRICMP_FUNC(Msg->Command,"TOPIC") == 0)
  {
    BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnTopic != NULL)
      I->CB.OnTopic(I,Msg->Params[0],Msg->Prefix,Tmp);
  }
  else if(STRICMP_FUNC(Msg->Command,"INVITE") == 0)
  {
    if(I->CB.OnInvite != NULL)
      I->CB.OnInvite(I,Msg->Params[1],Msg->Prefix,Msg->Params[0]);
  }
  else if(STRICMP_FUNC(Msg->Command,"KILL") == 0)
  {
    BN_CreateStatusLine(Msg->Params,1,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnKill != NULL)
      I->CB.OnKill(I,Msg->Prefix,Msg->Params[0],Tmp);
  }
#ifdef SERVER
  else if(STRICMP_FUNC(Msg->Command,"PASS") == 0)
  {
    if(Msg->NbParams == 1)
    {
      /* Client PASS */
      if(I->SCB.OnPass != NULL)
        I->SCB.OnPass(I,Msg->Params[0],NULL,NULL,NULL);
    }
#ifdef BAHAMUT
    else if(Msg->NbParams == 2)
    {
      /* Server PASS with options */
      if(I->SCB.OnPass != NULL)
        I->SCB.OnPass(I,Msg->Params[0],NULL,NULL,Msg->Params[1]);
    }
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for PASS command");
#else
    else if(Msg->NbParams == 3)
    {
      /* Server PASS without options */
      if(I->SCB.OnPass != NULL)
        I->SCB.OnPass(I,Msg->Params[0],Msg->Params[1],Msg->Params[2],NULL);
    }
    else if(Msg->NbParams == 4)
    {
      /* Server PASS with options */
      if(I->SCB.OnPass != NULL)
        I->SCB.OnPass(I,Msg->Params[0],Msg->Params[1],Msg->Params[2],Msg->Params[3]);
    }
    else if(Msg->NbParams < 3)
      BN_ServerSendErrorMessage(I,"Not enough parameters for PASS command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for PASS command");
#endif
  }
  else if(STRICMP_FUNC(Msg->Command,"SERVER") == 0)
  {
    if(Msg->NbParams == 3)
    {
      /* SERVER without token */
      if(Msg->Prefix == NULL)
      {
        if(I->SCB.OnServer != NULL)
          I->SCB.OnServer(I,Msg->Params[0],atoi(Msg->Params[1]),1,Msg->Params[2]);
      }
      else
#ifdef BAHAMUT
      {
        if(I->SCB.OnForeignServer != NULL)
          I->SCB.OnForeignServer(I,Msg->Prefix,Msg->Params[0],atoi(Msg->Params[1]),1,Msg->Params[2]);
      }
#else
        BN_ServerSendErrorMessage(I,"Not enough parameters for SERVER command");
#endif
    }
    else if(Msg->NbParams == 4)
    {
      /* SERVER with token */
      if(Msg->Prefix == NULL)
      {
        if(I->SCB.OnServer != NULL)
          I->SCB.OnServer(I,Msg->Params[0],atoi(Msg->Params[1]),atoi(Msg->Params[2]),Msg->Params[3]);
      }
      else
      {
        if(I->SCB.OnForeignServer != NULL)
          I->SCB.OnForeignServer(I,Msg->Prefix,Msg->Params[0],atoi(Msg->Params[1]),atoi(Msg->Params[2]),Msg->Params[3]);
      }
    }
    else if(Msg->NbParams < 3)
      BN_ServerSendErrorMessage(I,"Not enough parameters for SERVER command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for SERVER command");
  }
  else if(STRICMP_FUNC(Msg->Command,"SERVICE") == 0)
  {
    if(Msg->NbParams == 6)
    {
      if(I->SCB.OnService != NULL)
        I->SCB.OnService(I,Msg->Params[0],atoi(Msg->Params[1]),Msg->Params[2],Msg->Params[3],atoi(Msg->Params[4]),Msg->Params[5]);
    }
    else if(Msg->NbParams < 6)
      BN_ServerSendErrorMessage(I,"Not enough parameters for SERVICE command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for SERVICE command");
  }
  else if(STRICMP_FUNC(Msg->Command,"SQUIT") == 0)
  {
    if(Msg->NbParams == 2)
    {
      if(Msg->Prefix == NULL)
      {
        if(I->SCB.OnSQuit != NULL)
          I->SCB.OnSQuit(I,Msg->Params[0],Msg->Params[1]);
      }
      else
      {
        if(I->SCB.OnForeignSQuit != NULL)
          I->SCB.OnForeignSQuit(I,Msg->Prefix,Msg->Params[0],Msg->Params[1]);
      }
    }
    else if(Msg->NbParams < 2)
      BN_ServerSendErrorMessage(I,"Not enough parameters for SQUIT command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for SQUIT command");
  }
#ifdef BAHAMUT
  else if(STRICMP_FUNC(Msg->Command,"SJOIN") == 0)
  {
    if(Msg->NbParams == 5)
    {
      context;
      ts->BN_NJoinCount = 0;
      BN_ParseStringList(Msg->Params[4],&ts->BN_NJoinList,&ts->BN_NJoinCount," ",ts->tmp_tok);
      if(I->SCB.OnSJoin != NULL)
        I->SCB.OnSJoin(I,Msg->Params[2],atol(Msg->Params[0]),atol(Msg->Params[1]),Msg->Params[3],NULL,(const char **)ts->BN_NJoinList,ts->BN_NJoinCount);
      BN_FreeList(ts->BN_NJoinList,ts->BN_NJoinCount);
    }
    else if(Msg->NbParams == 6)
    {
      context;
      ts->BN_NJoinCount = 0;
      BN_ParseStringList(Msg->Params[5],&ts->BN_NJoinList,&ts->BN_NJoinCount," ",ts->tmp_tok);
      if(I->SCB.OnSJoin != NULL)
        I->SCB.OnSJoin(I,Msg->Params[2],atol(Msg->Params[0]),atol(Msg->Params[1]),Msg->Params[3],Msg->Params[4],(const char **)ts->BN_NJoinList,ts->BN_NJoinCount);
      BN_FreeList(ts->BN_NJoinList,ts->BN_NJoinCount);
    }
    else if(Msg->NbParams < 5)
      BN_ServerSendErrorMessage(I,"Not enough parameters for SJOIN command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for SJOIN command");
  }
#else
  else if(STRICMP_FUNC(Msg->Command,"NJOIN") == 0)
  {
    if(Msg->NbParams == 2)
    {
      context;
      ts->BN_NJoinCount = 0;
      BN_ParseStringList(Msg->Params[1],&ts->BN_NJoinList,&ts->BN_NJoinCount,",",ts->tmp_tok);
      if(I->SCB.OnNJoin != NULL)
        I->SCB.OnNJoin(I,Msg->Params[0],(const char **)ts->BN_NJoinList,ts->BN_NJoinCount);
      BN_FreeList(ts->BN_NJoinList,ts->BN_NJoinCount);
    }
    else if(Msg->NbParams < 2)
      BN_ServerSendErrorMessage(I,"Not enough parameters for NJOIN command");
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for NJOIN command");
  }
#endif
  else if(STRICMP_FUNC(Msg->Command,"PONG") == 0)
  {
    if(I->CB.OnPingPong != NULL)
      I->CB.OnPingPong(I);
  }
  else if(STRICMP_FUNC(Msg->Command,"ADMIN") == 0)
  {
    if(Msg->NbParams == 1)
    {
      /* ADMIN must be forwarded message if Params[0] != us */
      if(I->SCB.OnAdmin != NULL)
        I->SCB.OnAdmin(I,Msg->Prefix,Msg->Params[0]);
    }
    else if(Msg->NbParams == 0) /* No params.. local server's Admin info requested */
    {
      if(I->SCB.OnAdmin != NULL)
        I->SCB.OnAdmin(I,Msg->Prefix,NULL);
    }
    else
      BN_ServerSendErrorMessage(I,"Too many parameters for ADMIN command");
  }
#endif
  else if(STRICMP_FUNC(Msg->Command,"ERROR") == 0)
  {
    BN_CreateStatusLine(Msg->Params,0,Msg->NbParams-1,Tmp,sizeof(Tmp));
    BN_Disconnect(I);
    if(I->CB.OnDisconnected != NULL)
      I->CB.OnDisconnected(I,Tmp);
  }
  else // Unknown command
  {
    BN_CreateStatusLine(Msg->Params,0,Msg->NbParams-1,Tmp,sizeof(Tmp));
    if(I->CB.OnUnknown != NULL)
      I->CB.OnUnknown(I,Msg->Prefix,Msg->Command,Tmp);
  }
}

