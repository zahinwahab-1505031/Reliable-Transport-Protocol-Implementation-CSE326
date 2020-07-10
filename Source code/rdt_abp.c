#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
FILE *fp;
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);


#define WAIT_FOR_CALL_FROM_ABOVE0 0
#define WAIT_FOR_ACK0 1
#define WAIT_FOR_CALL_FROM_ABOVE1 1
#define WAIT_FOR_ACK1 3
//states for B

#define WAIT_FOR_PKT0 0
#define WAIT_FOR_PKT1 1

int A_STATE = 0;
int B_STATE = 0;

int seqnum = 0;
int acknum = 0;

#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2
#define INCREMENT 15
#define OFF 0
#define ON 1
#define A 0
#define B 1

struct pkt sendAgain;
struct pkt sendAgain_ACK;
//states for A
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
int createCheckSum(struct pkt packet) {
    int hash = 3;
    hash = hash + 57* packet.seqnum;
    hash = hash + 73* packet.acknum;
    for(int i=0;i< strlen(packet.payload);i++) {
        hash+= (int)packet.payload[i];
    }
    return hash;

}
int matchCheckSum(struct pkt packet) {
    int actual = createCheckSum(packet);
    if(actual==packet.checksum) return 1;
    else return 0;
}
void printPacket(struct pkt packet){
    fprintf(fp,"MSG: %s ACKNUM: %d CHECKSUM: %d SEQNUM: %d\n",packet.payload,packet.acknum,packet.checksum,packet.seqnum);
}
void A_output(struct msg message)
{
   if(A_STATE == WAIT_FOR_CALL_FROM_ABOVE0) {
    fprintf(fp,"===================================A IS WAITING FOR CALL FROM ABOVE0==============================\n");
        struct pkt sendPacket;
       /* sendPacket.acknum = acknum;
        if(acknum==0) acknum = 1;
        else acknum = 0;*/
        sendPacket.seqnum = 0;
        sendPacket.acknum = -1;
        
        strcpy(sendPacket.payload,message.data);
        int checksum = createCheckSum(sendPacket);
        sendPacket.checksum = checksum;

        tolayer3(A,sendPacket); 
        starttimer(A,INCREMENT);
        A_STATE = WAIT_FOR_ACK0;
        sendAgain = sendPacket;
         fprintf(fp,"===================================A SENDING seqnum %d msg: %s checksum: %d=============================\n",sendPacket.seqnum,sendPacket.payload,sendPacket.checksum);
  
   }
    else if(A_STATE == WAIT_FOR_CALL_FROM_ABOVE1) {
 fprintf(fp,"===================================A IS WAITING FOR CALL FROM ABOVE1==============================\n");

        struct pkt sendPacket;
        sendPacket.acknum = -1;
          /* sendPacket.acknum = acknum;
        if(acknum==0) acknum = 1;
        else acknum = 0;*/
        sendPacket.seqnum = 1;
       
        strcpy(sendPacket.payload,message.data);
        int checksum = createCheckSum(sendPacket);
        sendPacket.checksum = checksum;

        tolayer3(A,sendPacket); 
        starttimer(A,INCREMENT);
        A_STATE = WAIT_FOR_ACK1;
        sendAgain = sendPacket;
         fprintf(fp,"===================================A SENDING seqnum %d msg: %s checksum: %d=============================\n",sendPacket.seqnum,sendPacket.payload,sendPacket.checksum);
   }
   else {
    fprintf(fp,"==========================A IS WAITING FOR ACK %d=================",!acknum);
    //emon ek shomoye layer 5 theke call kora hoise jokhon packet ke ignore korbo
   }

}

/* need be completed only for extra credit */
void B_output(struct msg message)
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(A_STATE == WAIT_FOR_ACK0) {
        fprintf(fp,"============================A WAITING FOR ACK 0==========================\n");
        if(matchCheckSum(packet)==1 && packet.acknum == 0) {
            fprintf(fp,"===A GOT PACKET OF checksum %d and acknum %d not corrupted TIMER STOPPED=====\n",packet.checksum,packet.acknum);
            stoptimer(A);
            A_STATE = WAIT_FOR_CALL_FROM_ABOVE1;
        }
         if(matchCheckSum(packet)==0 || packet.acknum == 1) {
                 fprintf(fp,"===A GOT PACKET OF checksum %d and acknum %d =====\n",packet.checksum,packet.acknum);
          //  do nothing
        }

    }
    else if(A_STATE == WAIT_FOR_ACK1) {
        if(matchCheckSum(packet)==1 && packet.acknum == 1) {

            fprintf(fp,"===A GOT PACKET OF checksum %d and acknum %d not corrupted TIMER STOPPED=====\n",packet.checksum,packet.acknum);
            stoptimer(A);
            A_STATE = WAIT_FOR_CALL_FROM_ABOVE0;
        }
         if(matchCheckSum(packet)==0 || packet.acknum == 0) {

                 fprintf(fp,"===A GOT PACKET OF checksum %d and acknum %d =====\n",packet.checksum,packet.acknum);
          //  do nothing
        }

    }

}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{

    if(A_STATE == WAIT_FOR_ACK0 || A_STATE == WAIT_FOR_ACK1) {
        fprintf(fp,"====A HAD A TIMER INTERRUPT SO SENDING PACKET: MSG: %s, SEQNUM: %d,CHECKSUM: %d=====\n",sendAgain.payload,sendAgain.seqnum,sendAgain.checksum );
        tolayer3(A,sendAgain);
        starttimer(A,INCREMENT);
    }


}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    A_STATE = WAIT_FOR_CALL_FROM_ABOVE0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/

void B_input(struct pkt packet)
{
    if(B_STATE == WAIT_FOR_PKT0) {
        if(packet.seqnum ==0 && matchCheckSum(packet)==1) {
            fprintf(fp,"===WAITING FOR 0: B RECEIVED PACKET OF MSG: %s SEQNUM %d and checksum %d matches so go !!!!! ===\n",packet.payload,packet.seqnum,packet.checksum );
            struct pkt ACKPacket;
            ACKPacket.acknum = 0;
            ACKPacket.seqnum = -1;
            ACKPacket.checksum = createCheckSum(ACKPacket);
            sendAgain_ACK = ACKPacket;
            struct msg message;
            strcpy(message.data,packet.payload);
            B_STATE = WAIT_FOR_PKT1;
            fprintf(fp,"====B SENDING ACK PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
            tolayer3(B,sendAgain_ACK);
            tolayer5(B,packet.payload);
       

        }
        else  if(packet.seqnum ==1 && matchCheckSum(packet)==1) {
             fprintf(fp,"WAITING FOR 0: B RECEIVED PKT OF MSG: %s SEQNUM %d AND CHECKSUM %d\n",packet.payload,packet.seqnum,packet.checksum );
           
            tolayer3(B,sendAgain_ACK);
             fprintf(fp,"====B HAD A DIFF SEQ PACKET SO SENDING PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
       

            // oi state ei thakbe
        }
        else if(matchCheckSum(packet)==0) {
            fprintf(fp,"WAITING FOR 0: B B RECEIVED CORRUPTED PACKET OF SEQNUM %d AND CHECKSUM %d\n",packet.seqnum,packet.checksum);
            tolayer3(B,sendAgain_ACK);
            fprintf(fp,"====GOT CORRUPTED PACKET SO SENDING PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
       


          //  oi state ei thakbe
        }
    }
    else if(B_STATE == WAIT_FOR_PKT1) {
        if(packet.seqnum ==1 && matchCheckSum(packet)==1) {
             fprintf(fp,"===WAITING FOR 1: B RECEIVED PACKET OF MSG: %s SEQNUM %d and checksum %d matches so go !!!!! ===\n",packet.payload,packet.seqnum,packet.checksum );
           struct pkt ACKPacket;
            ACKPacket.acknum = 1;
            ACKPacket.seqnum = -1;
            ACKPacket.checksum = createCheckSum(ACKPacket);
            sendAgain_ACK = ACKPacket;
            B_STATE = WAIT_FOR_PKT0;

             fprintf(fp,"====B SENDING ACK PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
            tolayer3(B,sendAgain_ACK);
            tolayer5(B,packet.payload);
       

        }
        else  if(packet.seqnum ==0 && matchCheckSum(packet)==1) {
             fprintf(fp,"WAITING FOR 1: B RECEIVED PKT OF MSG: %s SEQNUM %d AND CHECKSUM %d\n",packet.payload,packet.seqnum,packet.checksum );
           
               tolayer3(B,sendAgain_ACK);
                   fprintf(fp,"====B HAD A DIFF SEQ PACKET SO SENDING PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
       


            // oi state ei thakbe
        }
        else if(matchCheckSum(packet)==0) {

            fprintf(fp,"WAITING FOR 1: B RECEIVED CORRUPTED PACKET OF SEQNUM %d AND CHECKSUM %d\n",packet.seqnum,packet.checksum);
               tolayer3(B,sendAgain_ACK);
                    fprintf(fp,"====B GOT CORRUPTED PACKET SO SENDING PACKET: ACKNUM: %d,CHECKSUM: %d=====\n",sendAgain_ACK.acknum,sendAgain_ACK.checksum );
       


          //  B_STATE = WAIT_FOR_PKT1;
        }
    }

}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    fprintf(fp,"  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */

void B_init(void)
{
    sendAgain_ACK.acknum = 1;
    sendAgain_ACK.seqnum = -1;
    sendAgain_ACK.checksum = createCheckSum(sendAgain_ACK);
    B_STATE = WAIT_FOR_PKT0;

}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */



int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    fp = fopen("output_abp.txt", "w");//

    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();
//     freopen("output.txt","w",stdout);
    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            fprintf(fp,"\nEVENT time: %f,", eventptr->evtime);
            fprintf(fp,"  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                fprintf(fp,", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                fprintf(fp,", fromlayer5 ");
            else
                fprintf(fp,", fromlayer3 ");
            fprintf(fp," entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    fprintf(fp,"          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        fprintf(fp,"%c", msg2give.data[i]);
                    fprintf(fp,"\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            fprintf(fp,"INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    fprintf(fp,
        " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
        time, nsim);
  //  freopen("output.txt","w",stdout);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d",&nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f",&lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f",&corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        fprintf(fp,"It is likely that random number generation on your machine\n");
        fprintf(fp,"is different from what this emulator expects.  Please take\n");
        fprintf(fp,"a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        fprintf(fp,"          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        fprintf(fp,"            INSERTEVENT: time is %lf\n", time);
        fprintf(fp,"            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    fprintf(fp,"--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        fprintf(fp,"Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    fprintf(fp,"--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        fprintf(fp,"          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    fprintf(fp,"Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        fprintf(fp,"          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            fprintf(fp,"Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            fprintf(fp,"          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        fprintf(fp,"          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            fprintf(fp,"%c", mypktptr->payload[i]);
        fprintf(fp,"\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            fprintf(fp,"          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        fprintf(fp,"          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        fprintf(fp,"          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            fprintf(fp,"%c", datasent[i]);
        fprintf(fp,"\n");
    }
}
