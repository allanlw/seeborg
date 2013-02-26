/* Bot Net Utils file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#include "includes.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

int BN_DebugLevel = 0;
#ifdef CONTEXT
int BN_ContextLine;
char BN_ContextFile[512];

void set_context(char *file, int line)
{
  BN_ContextLine = line;
  strncpy(BN_ContextFile, file, sizeof(BN_ContextFile)-1);
  BN_ContextFile[sizeof(BN_ContextFile)-1] = 0;
}
#endif

char BN_StampDebug[10];

char *GetTimeStamp()
{
  struct tm *TM;
  time_t Tim;

  Tim = time(NULL);
  TM = localtime(&Tim);
  snprintf(BN_StampDebug,sizeof(BN_StampDebug),"%d:%d:%d",TM->tm_hour,TM->tm_min,TM->tm_sec);
  return BN_StampDebug;
}

char *TrimLeft(const char *S)
{
  int i;

  i = 0;
  while(S[i] == ' ')
  {
    i++;
  }
  return (char *)(S+i);
}

void BN_ExtractNick(const char *Prefix,char Nick[],int Length)
{
  int p;
  char *buf;

  if(Prefix == NULL)
  {
    Nick[0] = 0;
    return;
  }
  buf = strchr(Prefix,'!');
  if(buf == NULL)
    p = strlen(Prefix);
  else
    p = buf - Prefix;
  if(p >= Length)
    p = Length - 1;
  memcpy(Nick,Prefix,p);
  Nick[p] = 0;
}

void BN_ExtractHost(const char *Prefix,char Host[],int Length)
{
  int p;
  char *buf;

  if(Prefix == NULL)
  {
    Host[0] = 0;
    return;
  }
  buf = strchr(Prefix,'@');
  if(buf == NULL)
  {
    Host[0] = 0;
    return;
  }
  p = strlen(Prefix) - (buf - Prefix + 1);
  if(p >= Length)
    p = Length-1;
  memcpy(Host,buf+1,p);
  Host[p] = 0;
}

void BN_ExtractUserName(const char *Prefix,char UserName[],int Length)
{
  int p,i;
  char *buf,*buf2;

  if(Prefix == NULL)
  {
    UserName[0] = 0;
    return;
  }
  buf = strchr(Prefix,'!');
  buf2 = strchr(Prefix,'@');
  if((buf == NULL) || (buf2 == NULL))
  {
    UserName[0] = 0;
    return;
  }
  i = 0;
  /* Remove ident prefix */
  /* Prefixs for irc2 server
      none I line with ident
      ^    I line with OTHER type ident
      ~    I line, no ident
      +    i line with ident
      =    i line with OTHER type ident
      -    i line, no ident
  */
  if((buf[1] == '~') || (buf[1] == '^') || (buf[1] == '+') || (buf[1] == '=') || (buf[1] == '-'))
    i = 1;
  p = buf2 - buf - 1 - i;
  if(p >= Length)
    p = Length-1;
  memcpy(UserName,buf+1+i,p);
  UserName[p] = 0;
}

void BN_ExtractExactUserName(const char *Prefix,char UserName[],int Length)
{
  int p;
  char *buf,*buf2;

  if(Prefix == NULL)
  {
    UserName[0] = 0;
    return;
  }
  buf = strchr(Prefix,'!');
  buf2 = strchr(Prefix,'@');
  if((buf == NULL) || (buf2 == NULL))
  {
    UserName[0] = 0;
    return;
  }
  p = buf2 - buf - 1;
  if(p >= Length)
    p = Length-1;
  memcpy(UserName,buf+1,p);
  UserName[p] = 0;
}

char *BN_MakeMessage(const char *Prefixe,const char *Commande,const char *Params)
{
  int len,pos;
  char *buf;

  if(Prefixe != NULL)
    len = strlen(Prefixe) + 2;
  else
    len = 0;
  len += strlen(Commande) + strlen (Params) + 4;
  buf = (char *)malloc(len);
  if(buf == NULL)
    return NULL;
  pos = 0;
  if(Prefixe != NULL)
  {
    buf[pos++] = ':';
    strcpy(buf+pos,Prefixe); /* This one should be safe, cause we allocated buf with strlen */
    pos += strlen(Prefixe);
    buf[pos++] = ' ';
  }
  strcpy(buf+pos,Commande);
  pos += strlen(Commande);
  buf[pos++] = ' ';
  strcpy(buf+pos,Params);
  pos += strlen(Params);
  buf[pos++] = 13;
  buf[pos++] = 10;
  buf[pos++] = 0;

  return buf;
}

void BN_FreeMessage(BN_PMessage Msg)
{
  int i;

  if(Msg->Prefix != NULL)
    free(Msg->Prefix);
  if(Msg->Command != NULL)
    free(Msg->Command);
  if(Msg->NbParams != 0)
  {
    for(i=0;i<Msg->NbParams;i++)
      free(Msg->Params[i]);
  }
  free(Msg);
}

void BN_CreateStatusLine(char *Params[],int min,int max,char Out[],int len)
{
  int i,pos;

  Out[0] = 0;
  pos = len;
  if((max-min) >= 0)
  {
    for(i=min;i<=max;i++)
    {
      strncat(Out,Params[i],pos);
      pos -= strlen(Params[i]) - 1; /* -1 for the following space */
      if(pos <= 1)
      {
        Out[len-1] = 0;
        return;
      }
      strncat(Out," ",pos);
    }
    Out[strlen(Out)-1] = 0;
  }
}

void BN_UnsetSigs(bool Fork)
{
#ifdef __unix__
  struct sigaction action;
  sigset_t mask;

  if(Fork)
  {
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_DFL;
    sigaction(SIGHUP,&action,NULL);
    sigaction(SIGINT,&action,NULL);
    sigaction(SIGQUIT,&action,NULL);
    sigaction(SIGILL,&action,NULL);
    sigaction(SIGABRT,&action,NULL);
    sigaction(SIGFPE,&action,NULL);
    sigaction(SIGSEGV,&action,NULL);
    sigaction(SIGPIPE,&action,NULL);
    sigaction(SIGALRM,&action,NULL);
    sigaction(SIGTERM,&action,NULL);
    sigaction(SIGUSR1,&action,NULL);
    sigaction(SIGUSR2,&action,NULL);
    sigaction(SIGCHLD,&action,NULL);
    sigaction(SIGCONT,&action,NULL);
    sigaction(SIGTSTP,&action,NULL);
    sigaction(SIGTTIN,&action,NULL);
    sigaction(SIGTTOU,&action,NULL);
  }
  else
  {
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK,&mask,NULL);
  }
#endif
}

void BN_UnsetEvents(BN_PInfo I,bool Chat)
{
  I->CB.OnConnected = NULL;
  I->CB.OnRegistered = NULL;
  I->CB.OnError = NULL;
  I->CB.OnPingPong = NULL;
  I->CB.OnStatus = NULL;
  I->CB.OnJoinChannel = NULL;
  I->CB.OnChannelTalk = NULL;
  I->CB.OnAction = NULL;
  I->CB.OnPrivateTalk = NULL;
  I->CB.OnCTCP = NULL;
  I->CB.OnCTCPReply = NULL;
  I->CB.OnNotice = NULL;
  I->CB.OnNick = NULL;
  I->CB.OnJoin = NULL;
  I->CB.OnPart = NULL;
  I->CB.OnQuit = NULL;
  I->CB.OnMode = NULL;
  I->CB.OnModeIs = NULL;
  I->CB.OnTopic = NULL;
  I->CB.OnTopicIs = NULL;
  I->CB.OnTopicSetBy = NULL;
  I->CB.OnList = NULL;
  I->CB.OnNames = NULL;
  I->CB.OnWhois = NULL;
  I->CB.OnWho = NULL;
  I->CB.OnBanList = NULL;
  I->CB.OnInvite = NULL;
  I->CB.OnKick = NULL;
  I->CB.OnKill = NULL;
  I->CB.OnDisconnected = NULL;
  I->CB.OnExcessFlood = NULL;
  I->CB.OnUnknown = NULL;
  I->CB.Chat.OnDCCChatRequest = NULL;
  I->CB.Send.OnDCCSendRequest = NULL;
  if(Chat)
  {
    I->CB.Send.OnDCCSendOpened = NULL;
    I->CB.Send.OnDCCSendClosed = NULL;
    I->CB.Send.OnDCCGetOpened = NULL;
    I->CB.Send.OnDCCGetClosed = NULL;
  }
  else
  {
    I->CB.Chat.OnDCCChatOpened = NULL;
    I->CB.Chat.OnDCCChatClosed = NULL;
    I->CB.Chat.OnDCCTalk = NULL;
    I->CB.Chat.OnDCCChatIdle = NULL;
  }
  I->CB.OnUserCallback1 = NULL;
  I->CB.OnUserCallback2 = NULL;
  I->CB.OnUserCallback3 = NULL;
  I->CB.OnUserCallback4 = NULL;
}

bool BN_CheckForFlood(BN_PFlood F,int len)
{
  time_t t;
  int np;

  t = time(NULL);
  while(((t - F->Time[F->spos]) > F->FloodTime) && (F->spos != F->epos))
  {
    F->Total -= F->Size[F->spos];
    F->spos = (F->spos+1)%F->Length;
  }
  if((F->Total + len) >= F->FloodSize)
    return true;
  np = (F->epos+1)%F->Length;
  if(np == F->spos)
  {
    printf("Flood buffer overflow\n");
    return true;
  }
  F->Total += len;
  F->Size[F->epos] = len;
  F->Time[F->epos] = t;
  F->epos = np;

  return false;
}

void BN_EnableFloodProtection(BN_PInfo I,int BufferSize,int SizeLimit,time_t TimeLimit)
{
  BN_PFlood F;

  BN_PrintDebug(1,"Flood protection has been enabled : %d:%ld\n",SizeLimit,TimeLimit);
  if(I->Flood != NULL)
    BN_DisableFloodProtection(I);
  F = (BN_PFlood)malloc(sizeof(BN_TFlood));
  memset(F,0,sizeof(BN_TFlood));
  F->Length = BufferSize;
  F->Time = (time_t *)malloc(BufferSize*sizeof(time_t));
  F->Size = (int *)malloc(BufferSize*sizeof(int));
  F->FloodTime = TimeLimit;
  F->FloodSize = SizeLimit;
  F->Queue = NULL;
#ifdef HAVE_SEM_INIT
  sem_init(&F->sem,0,1);
#endif
  I->Flood = F;
}

void BN_DisableFloodProtection(BN_PInfo I)
{
  BN_PFlood F;
  BN_PQueue Ptr;

  BN_PrintDebug(1,"Flood protection has been disabled\n");
  F = I->Flood;
  if(F == NULL)
    return;
  free(F->Time);
  free(F->Size);
#ifdef HAVE_SEM_INIT
  sem_destroy(&F->sem);
#endif
  Ptr = F->Queue;
  while(Ptr != NULL)
  {
    free(Ptr->Message);
    Ptr = Ptr->Next;
  }
  free(F);
  I->Flood = NULL;
}

/* Code ripped from XChat */
char *BN_StripColor(const char *text)
{
   int comma, done;
   int j = 0, i = 0, len = strlen(text);
   char *buf = malloc(len + 2);

   while(i < len)
   {
      switch(text[i])
      {
        case 2:
          break;
        case 3:
          comma = false;
          done = false;
          while(i < len && !done)
          {
            i++;
            if(!isdigit(text[i]))
            {
              switch(text[i])
              {
                case ',':
                  if(comma) done = true; else comma = true;
                  break;
                case ' ':
                  done = true;
                  i++;
                  break;
                default:
                  done = true;
              }
            }
          }
          i--;
          break;
        default:
          buf[j] = text[i];
          j++;
      }
      i++;
   }
   buf[j] = 0;

   return buf;
}

#undef BN_PrintDebug
void BN_PrintDebug(int Level,char *Txt, ...)
{
  va_list argptr;
  char Str[4096];

  if(Level <= BN_DebugLevel)
  {
    va_start(argptr,Txt);
#ifdef _WIN32
    _vsnprintf(Str,sizeof(Str),Txt,argptr);
#else /* _WIN32 */
    vsnprintf(Str,sizeof(Str),Txt,argptr);
#endif /* _WIN32 */
    va_end(argptr);
    printf("BOTNET(%d) : %s",Level,Str);
  }
}

void BN_SetDebugLevel(int Level)
{
  BN_DebugLevel = Level;
}
