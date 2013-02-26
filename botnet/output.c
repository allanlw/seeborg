/* Bot Net Output file
  (c) Christophe CALMEJANE - 1999'01
  aka Ze KiLleR / SkyTech
*/

#include "includes.h"

void BN_SendPassword(BN_PInfo I,const char *Password)
{
  BN_SendMessage(I,BN_MakeMessage(NULL,"PASS",Password),BN_HIGH_PRIORITY);
}

void BN_Register(BN_PInfo I,const char *Nick,const char *UserName,const char *RealName)
{
  char buf[1024];

  I->Nick[0] = 0;
  BN_SendNickMessage(I,Nick);
  /* HMage: there was an unnecessary whitespace in front of RealName */
  snprintf(buf,sizeof(buf),"%s 0 0 :%s",UserName,RealName);
  BN_SendMessage(I,BN_MakeMessage(NULL,"USER",buf),BN_HIGH_PRIORITY);
}

void BN_RegisterService(BN_PInfo I,const char *Nick,const char *Distribution,const char *Type,const char *Info)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s * %s %s 0 :%s",Nick,Distribution,Type,Info);
  BN_SendMessage(I,BN_MakeMessage(NULL,"SERVICE",buf),BN_HIGH_PRIORITY);
}

void BN_SendNickMessage(BN_PInfo I,const char *NewNick)
{
  char *buf;
  if(I->Nick[0] != 0)
  {
    buf = BN_MakeMessage(I->Nick,"NICK",NewNick);
    BN_SendMessage(I,buf,BN_HIGH_PRIORITY);
  }
  else
  {
    strncpy(I->Nick,NewNick,sizeof(I->Nick)-1);
    I->Nick[sizeof(I->Nick)-1] = 0;
    buf = BN_MakeMessage(NULL,"NICK",NewNick);
    BN_SendMessage(I,buf,BN_HIGH_PRIORITY);
  }
}

void BN_SendChannelMessage(BN_PInfo I,const char *Channel,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%s",Channel,Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"PRIVMSG",buf),BN_LOW_PRIORITY);
}

void BN_SendActionMessage(BN_PInfo I,const char *Channel,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%cACTION %s%c",Channel,1,Message,1);
  BN_SendMessage(I,BN_MakeMessage(NULL,"PRIVMSG",buf),BN_LOW_PRIORITY);
}

void BN_SendPrivateMessage(BN_PInfo I,const char *Nick,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%s",Nick,Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"PRIVMSG",buf),BN_LOW_PRIORITY);
}

void BN_SendSQueryMessage(BN_PInfo I,const char *ServiceName,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),"%s :%s",ServiceName,Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"SQUERY",buf),BN_LOW_PRIORITY);
}

void BN_SendWallopMessage(BN_PInfo I,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),":%s",Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"WALLOPS",buf),BN_LOW_PRIORITY);
}

bool BN_RealSendMessage(BN_PInfo I,char *Message,int len)
{
  int res;

  res = send(I->Socket,Message,len,0);
  BN_PrintDebug(4,"(%s) : %d octets sent (for a length of %d) %s",GetTimeStamp(),res,len,Message);
  free(Message);
  if(len != res)
  {
    if(I->CB.OnError != NULL)
      I->CB.OnError(I,errno);
    BN_Disconnect(I);
    if(I->CB.OnDisconnected != NULL)
      I->CB.OnDisconnected(I,"Write error on socket");
    if((I->Flags & PROCESS_NEW_PROCESS) == PROCESS_NEW_PROCESS)
      EXIT_PROCESS_FUNC(1);
    else if((I->Flags & PROCESS_NEW_THREAD) == PROCESS_NEW_THREAD)
      EXIT_THREAD_FUNC 1);
    else
      return false;
  }
  return true;
}

bool BN_EmptyQueue(BN_PInfo I)
{
  int len;
  BN_PQueue Ptr;

  /* Send all message we can before flooding */
#ifdef HAVE_SEM_INIT
  sem_wait(&I->Flood->sem);
#endif
  while(I->Flood->Queue != NULL)
  {
    len = strlen(I->Flood->Queue->Message);
    if(BN_CheckForFlood(I->Flood,len))
      break;
    if(BN_RealSendMessage(I,I->Flood->Queue->Message,len) == false)
    {
#ifdef HAVE_SEM_INIT
      sem_post(&I->Flood->sem);
#endif
      return false;
    }
    Ptr = I->Flood->Queue->Next;
    free(I->Flood->Queue);
    I->Flood->Queue = Ptr;
  }
#ifdef HAVE_SEM_INIT
  sem_post(&I->Flood->sem);
#endif
  return true;
}

bool BN_SendMessage(BN_PInfo I,char *Message,int Priority)
{
  int len;
  void (*SafCB)(BN_PInfo,const char *);
  BN_PQueue Que,Ptr,Prec;

  if(I->Flood != NULL)
  {
    if(Message == NULL)
      return BN_EmptyQueue(I);
    else
    {
      len = strlen(Message);
      if((I->Flood->Queue != NULL) || BN_CheckForFlood(I->Flood,len))
      {
        if(BN_CheckForFlood(I->Flood,len))
        {
          /* Raise ExcessFlood callback, if set */
          if(I->CB.OnExcessFlood != NULL)
          {
            SafCB = I->CB.OnExcessFlood;
            I->CB.OnExcessFlood = NULL;
            SafCB(I,Message);
            I->CB.OnExcessFlood = SafCB;
          }
        }
#ifdef HAVE_SEM_INIT
        sem_wait(&I->Flood->sem);
#endif
        /* Add current message to the queue */
        if(Priority == BN_LOW_PRIORITY)
        {
          Que = (BN_PQueue) malloc(sizeof(BN_TQueue));
          Que->Message = Message;
          Que->Prio = Priority;
          Que->Next = NULL;
          if(I->Flood->Queue == NULL)
            I->Flood->Queue = Que;
          else
          {
            Ptr = I->Flood->Queue;
            while(Ptr->Next != NULL)
              Ptr = Ptr->Next;
            Ptr->Next = Que;
          }
        }
        else
        {
          Que = (BN_PQueue) malloc(sizeof(BN_TQueue));
          Que->Message = Message;
          Que->Prio = Priority;
          Que->Next = NULL;
          if(I->Flood->Queue == NULL)
            I->Flood->Queue = Que;
          else
          {
            Prec = NULL;
            Ptr = I->Flood->Queue;
            while(Ptr != NULL)
            {
              if(Ptr->Prio == BN_LOW_PRIORITY)
              {
                if(Prec == NULL)
                {
                  Que->Next = I->Flood->Queue;
                  I->Flood->Queue = Que;
                }
                else
                {
                  Que->Next = Prec->Next;
                  Prec->Next = Que;
                }
                Que = NULL;
                break;
              }
              Prec = Ptr;
              Ptr = Ptr->Next;
            }
            if(Que != NULL)
              Prec->Next = Que;
          }
        }
#ifdef HAVE_SEM_INIT
        sem_post(&I->Flood->sem);
#endif
        return BN_EmptyQueue(I);
      }
    }
  }
  else
  {
    if(Message == NULL)
      return false;
    len = strlen(Message);
  }
  return BN_RealSendMessage(I,Message,len);
}

void BN_SendJoinMessage(BN_PInfo I,const char *Channel,const char *Keys)
{
  char buf[1024];

  strncpy(buf,Channel,sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;
  if(Keys != NULL)
  {
    strncat(buf," ",sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    strncat(buf,Keys,sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
  }
  BN_SendMessage(I,BN_MakeMessage(NULL,"JOIN",buf),BN_HIGH_PRIORITY);
}

void BN_SendPartMessage(BN_PInfo I,const char *Channel,const char *Reason)
{
  char buf[1024];

  strncpy(buf,Channel,sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;
  if(Reason != NULL)
  {
    strncat(buf," :",sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    strncat(buf,Reason,sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
  }
  BN_SendMessage(I,BN_MakeMessage(NULL,"PART",buf),BN_HIGH_PRIORITY);
}

void BN_SendQuitMessage(BN_PInfo I,const char *Message)
{
  char buf[1024];

  snprintf(buf,sizeof(buf),":%s",Message);
  BN_SendMessage(I,BN_MakeMessage(NULL,"QUIT",buf),BN_HIGH_PRIORITY);
}


