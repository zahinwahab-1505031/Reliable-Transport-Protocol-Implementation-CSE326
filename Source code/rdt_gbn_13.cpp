#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<queue>
#include<string.h>
#include<stack>
using namespace std;

FILE *fp;

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

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
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

#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define INCREMENT 15

#define OFF 0
#define ON 1
#define A 0
#define B 1

#define WINDOWSIZE 7
#define BUFFERSIZE 50

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



int send_base_A;
int send_nextSeqNum_A;
queue<pkt> pkt_buffer_A;
queue<float> timer_buffer_A;
int num_sent_unacknowledged_packets_A = 0;
stack<pkt> cumulativeACK_B;
int expectedSeqNum_B;
struct pkt senderWindow_A[WINDOWSIZE+1];
int isTimerON_A = 0;
bool duplicateACK_A[WINDOWSIZE+1];

int send_base_B;
int send_nextSeqNum_B;
queue<pkt> pkt_buffer_B;
queue<float> timer_buffer_B;
int num_sent_unacknowledged_packets_B = 0;
stack<pkt> cumulativeACK_A;
int expectedSeqNum_A;
struct pkt senderWindow_B[WINDOWSIZE+1];
int isTimerON_B = 0;
bool duplicateACK_B[WINDOWSIZE+1];

struct pkt sendAgain_ACK_B;
struct pkt sendAgain_ACK_A;

void myStartTimer(int AorB,float increment){
    if(AorB==A) {
    fprintf(fp, "========IN MY START TIMER : CURRENT TIME IS: %f ====================\n",time );
    if(isTimerON_A==0){
        fprintf(fp,"========NO TIMER IS CURRENTLY RUNNING SO STARTING IT========\n");
        starttimer(AorB,increment);
        isTimerON_A = 1;
    }else {
        timer_buffer_A.push(time);

        fprintf(fp, "PUSHING IT INTO BUFFER: BUFFERSIZE BECOMES %d\n",(int)timer_buffer_A.size() );
    }
    }else {
        fprintf(fp, "========IN MY START TIMER : CURRENT TIME IS: %f ====================\n",time );
    if(isTimerON_B==0){
        fprintf(fp,"========NO TIMER IS CURRENTLY RUNNING SO STARTING IT========\n");
        starttimer(AorB,increment);
        isTimerON_B = 1;
    }else {
        timer_buffer_B.push(time);

        fprintf(fp, "PUSHING IT INTO BUFFER: BUFFERSIZE BECOMES %d\n",(int)timer_buffer_B.size() );
    }
    }
}

void myStopTimer(int AorB){
    if(AorB == A) {
    fprintf(fp, "========IN MY STOP TIMER : CURRENT TIME IS: %f ====================\n",time );
   
    if(isTimerON_A==1)  {
        fprintf(fp,"=====================STOPPING THE TIMER=============\n");
        stoptimer(AorB);
    }
    isTimerON_A = 0;
    if(!timer_buffer_A.empty()) {

        float timeItWantedToStart = timer_buffer_A.front();
        timer_buffer_A.pop();
        float timeSpentAlready =  time - timeItWantedToStart;
        float actualIncrement = INCREMENT - timeSpentAlready;
         fprintf(fp,"=====================TIMER BUFFER NOT EMPTY SO POPPING %f %f=============\n",timeItWantedToStart,timeSpentAlready);
       
        starttimer(AorB,actualIncrement);
        isTimerON_A = 1;
    }
}else {
      fprintf(fp, "========IN MY STOP TIMER : CURRENT TIME IS: %f ====================\n",time );
   
    if(isTimerON_B==1)  {
        fprintf(fp,"=====================STOPPING THE TIMER=============\n");
        stoptimer(AorB);
    }
    isTimerON_B = 0;
    if(!timer_buffer_B.empty()) {

        float timeItWantedToStart = timer_buffer_B.front();
        timer_buffer_B.pop();
        float timeSpentAlready =  time - timeItWantedToStart;
        float actualIncrement = INCREMENT - timeSpentAlready;
         fprintf(fp,"=====================TIMER BUFFER NOT EMPTY SO POPPING %f %f=============\n",timeItWantedToStart,timeSpentAlready);
       
        starttimer(AorB,actualIncrement);
        isTimerON_B = 1;
    }
}
}
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
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
int fallsWithinWindow(int AorB,int num) {
    if(AorB==A){
    for(int i=0;i<WINDOWSIZE;i++){
     if( num == ((send_base_A+i)%(WINDOWSIZE+1)) ){
        return 1;
     }
    }
    return 0;
}
else {
      for(int i=0;i<WINDOWSIZE;i++){
     if( num == ((send_base_B+i)%(WINDOWSIZE+1)) ){
        return 1;
     }
    }
    return 0;
}
}
void printWindow(int AorB){
    if(AorB==A){
    fprintf (fp,"A: PRINTING WINDOW------------>||");
}
    else {
           fprintf (fp,"B: PRINTING WINDOW------------>||");

    }
    for(int i=0;i<WINDOWSIZE;i++){
  if(AorB==A)  fprintf(fp,"%d ",  ( (send_base_A+i)%(WINDOWSIZE+1) ) );
  else fprintf(fp,"%d ",  ( (send_base_B+i)%(WINDOWSIZE+1) ) );
     
    }
   fprintf(fp,"||\n");
}
void printTimerBuffer(int  AorB){
        if(AorB==A){
    fprintf (fp,"A: PRINTING TIMER BUFFER------------>||");
}
    else {
           fprintf (fp,"B: PRINTING TIMER BUFFER------------>||");

    }
queue <float> temp;
    if(AorB == A) temp = timer_buffer_A;
    else temp = timer_buffer_B;

   // fprintf(fp, "@@@@@@@@@@@@@@@@@@@%d  %d\n", (int)timer_buffer_A.size(),(int)temp.size());
    while(!temp.empty()) {
        float t = temp.front();
       fprintf(fp, "%f ,",t );
       temp.pop();
    }
    fprintf(fp, "||\n" );
}
int packetsACKed (int num) {
    int count = 0;
    for(int i=0;i<WINDOWSIZE;i++){
        count++;
     if( num == ((send_base_A+i)%(WINDOWSIZE+1)) ){
        return count;
     }
    }
}

void printPacket(struct pkt packet){
   fprintf(fp,"MSG: %s ACKNUM: %d CHECKSUM: %d SEQNUM: %d\n",packet.payload,packet.acknum,packet.checksum,packet.seqnum);
}
void A_output(struct msg message)
{
    if(num_sent_unacknowledged_packets_A < WINDOWSIZE && fallsWithinWindow(A,send_nextSeqNum_A)){
       struct pkt sendPacket;
        sendPacket.seqnum = send_nextSeqNum_A;
        sendPacket.acknum  = -1;
         strcpy(sendPacket.payload,message.data);
        int checksum = createCheckSum(sendPacket);
        sendPacket.checksum = checksum;
        senderWindow_A[send_nextSeqNum_A] = sendPacket;
         send_nextSeqNum_A = (send_nextSeqNum_A+1)%(WINDOWSIZE+1);
        
        num_sent_unacknowledged_packets_A++;

       fprintf(fp,"===================A IS SENDING PACKET OF MSG: %s SEQNUM: %d CHECKSUM: %d =============\n",sendPacket.payload,sendPacket.seqnum,sendPacket.checksum);
       fprintf(fp,"======================#SENT BUT UNACKED PKTS: %d============\n",num_sent_unacknowledged_packets_A);
         
        tolayer3(A,sendPacket); 
        myStartTimer(A,INCREMENT);
        printTimerBuffer(A);
    }
    else if(num_sent_unacknowledged_packets_A == WINDOWSIZE) {
        if(pkt_buffer_A.size()<=BUFFERSIZE){
           
            struct pkt sendPacket;
            sendPacket.acknum = -1;
            sendPacket.seqnum = send_nextSeqNum_A;
          
            send_nextSeqNum_A = (send_nextSeqNum_A+1)%(WINDOWSIZE+1);
            strcpy(sendPacket.payload,message.data);
            int checksum = createCheckSum(sendPacket);
            sendPacket.checksum = checksum;
              pkt_buffer_A.push(sendPacket);
            fprintf(fp,"===================A IS STORING PACKET OF MSG: %s SEQNUM: %d CHECKSUM: %d =============\n",sendPacket.payload,sendPacket.seqnum,sendPacket.checksum);
       
        }
        else{
            //discard the message
        }
    }



}

/* need be completed only for extra credit */
void B_output(struct msg message)
{
    if(num_sent_unacknowledged_packets_B < WINDOWSIZE && fallsWithinWindow(B,send_nextSeqNum_B)){
       struct pkt sendPacket;
        sendPacket.seqnum = send_nextSeqNum_B;
        sendPacket.acknum  = -1;
         strcpy(sendPacket.payload,message.data);
        int checksum = createCheckSum(sendPacket);
        sendPacket.checksum = checksum;
        senderWindow_B[send_nextSeqNum_B] = sendPacket;
        
        send_nextSeqNum_B = (send_nextSeqNum_B+1)%(WINDOWSIZE+1);
        
        num_sent_unacknowledged_packets_B++;

       fprintf(fp,"===================B IS SENDING PACKET OF MSG: %s SEQNUM: %d CHECKSUM: %d =============\n",sendPacket.payload,sendPacket.seqnum,sendPacket.checksum);
       fprintf(fp,"======================#SENT BUT UNACKED PKTS FOR B: %d============\n",num_sent_unacknowledged_packets_B);
         
        tolayer3(B,sendPacket); 
        myStartTimer(B,INCREMENT);
        printTimerBuffer(B);
    }
    else if(num_sent_unacknowledged_packets_B == WINDOWSIZE) {
        if(pkt_buffer_B.size()<=BUFFERSIZE){
           
            struct pkt sendPacket;
            sendPacket.acknum = -1;
            sendPacket.seqnum = send_nextSeqNum_B;
          
            send_nextSeqNum_B = (send_nextSeqNum_B+1)%(WINDOWSIZE+1);
            strcpy(sendPacket.payload,message.data);
            int checksum = createCheckSum(sendPacket);
            sendPacket.checksum = checksum;
              pkt_buffer_B.push(sendPacket);
            fprintf(fp,"===================B IS STORING PACKET OF MSG: %s SEQNUM: %d CHECKSUM: %d =============\n",sendPacket.payload,sendPacket.seqnum,sendPacket.checksum);
       
        }
        else{
            //discard the message
        }
    }



}

/* called from layer 3, when a packet arrives for layer 4 */

int prev_sendbase_A;

int prev_sendbase_B;

void A_input(struct pkt packet)
{
    if(matchCheckSum(packet)==1 && packet.seqnum == -1) {
        if(fallsWithinWindow(A,packet.acknum)) {
            fprintf(fp,"==========A RECEIVED PACKET OF ACKNUM: %d CHECKSUM: %d FALLS WITHIN WINDOW======\n",packet.acknum,packet.checksum);
            //slide window to the next seq num after the acknum received
            int total_packets_acked = packetsACKed(packet.acknum);
            num_sent_unacknowledged_packets_A -= total_packets_acked;
            fprintf(fp,"======================#SENT BUT UNACKED PKTS: %d============\n",num_sent_unacknowledged_packets_A);

            
            
            fprintf(fp,"=============PRINTING WINDOW BEFORE SLIDING===========\n");
            printWindow(A);
            printTimerBuffer(A);
            prev_sendbase_A = send_base_A;
            send_base_A = (packet.acknum+1)%(WINDOWSIZE+1);
            for(int i=0;i<(num_sent_unacknowledged_packets_A-1);i++){
                if(!timer_buffer_A.empty()) {
                float timeOfPktsAlreadyAcked = timer_buffer_A.front();
                timer_buffer_A.pop();
                 }
            }
            for(int i=0;i<(WINDOWSIZE+1);i++) duplicateACK_A[i] = false;
            //send_nextSeqNum_A = send_base_A;
            //send_max = -1;
            fprintf(fp,"=============PRINTING WINDOW AFTER SLIDING===========\n");
            printWindow(A);
            printTimerBuffer(A);
            myStopTimer(A);
            while(!pkt_buffer_A.empty()) {
              struct  pkt packet = pkt_buffer_A.front();
                if(fallsWithinWindow(A,packet.seqnum)){
                   fprintf(fp,"====A IS SENDING FOLLOWING PACKET FROM BUFFER==================\n");
                    printPacket(packet);
                    pkt_buffer_A.pop();
                    senderWindow_A[packet.seqnum] = packet;
                    tolayer3(A,packet);
                    myStartTimer(A,INCREMENT);
                }
            }
        }

        else  if(!fallsWithinWindow(A,packet.acknum)){
               fprintf(fp,"==========A RECEIVED PACKET OF ACKNUM: %d CHECKSUM: %d DOESN'T FALL WITHIN WINDOW======\n",packet.acknum,packet.checksum);
                    if(duplicateACK_A[prev_sendbase_A]==false) {
                    stoptimer(A);
                    isTimerON_A  = 0;
                    timer_buffer_A = queue<float>();
                    for(int i=0;i<num_sent_unacknowledged_packets_A;i++) {
                        fprintf(fp,"====SENDING PACKETS OF SEQ NUM: %d AGAIN=====\n",(send_base_A+i)%(WINDOWSIZE+1) );

                        printPacket(senderWindow_A[((send_base_A+i)%(WINDOWSIZE+1))]);
                        tolayer3(A,senderWindow_A[((send_base_A+i)%(WINDOWSIZE+1))]);
                        //stoptimer(A);
                        myStartTimer(A,INCREMENT);
                    }
                     fprintf(fp, "======AFTER A GOT A DUPLICATE ACK WE HAVE SENT ALL PKTS AND RESTARTED TIMERS===\n" );
                    printTimerBuffer(A);
                    }
                    duplicateACK_A[prev_sendbase_A] = true;
    
           
        }
    }
    else {
        //packet is corrupted
    
    }


  //  fprintf(fp,"A RECEIVED A PACKET\n");
    if(matchCheckSum(packet)==1 && packet.acknum== -1) { //packet is not corrupted
       fprintf(fp,"==================A RECEIVED GOOD PACKET AND IT'S EXPECTING %d=================\n",expectedSeqNum_A);

        printPacket(packet);
        if(packet.seqnum == expectedSeqNum_A) {
            fprintf(fp,"==========IN A MATCHES EXPECTED SEQNUM SO GOOOOO!!!!=========\n");
            struct pkt ACKPacket;
            strcpy(ACKPacket.payload,"");
            ACKPacket.seqnum = -1;
            ACKPacket.acknum = expectedSeqNum_A;
            ACKPacket.checksum = createCheckSum(ACKPacket);
            tolayer5(A,packet.payload);
            sendAgain_ACK_A = ACKPacket;
            fprintf(fp,"=================A IS SENDING ACK PACKET============\n");

            printPacket(sendAgain_ACK_A);
            cumulativeACK_A.push(sendAgain_ACK_A);
            starttimer(A,INCREMENT);
            tolayer3(A,sendAgain_ACK_A);
            expectedSeqNum_A = (expectedSeqNum_A+1)%(WINDOWSIZE+1);
         
        }
        else {
           fprintf(fp,"==========IN A DOESNT MATCH EXPECTED SEQNUM SENDING DUPLICATE PACKET=========\n");
         
            printPacket(sendAgain_ACK_A);
            tolayer3(A,sendAgain_ACK_A);
            
        }

    }
    else { // packet is corrupted
        fprintf(fp,"==================A RECEIVED FOLLOWING CORRUPTED PACKET=================\n");
        printPacket(packet);

           fprintf(fp,"=================A IS SENDING DUPLICATE ACK PACKET============\n");
            printPacket(sendAgain_ACK_A);
          tolayer3(A,sendAgain_ACK_A);
    }

}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
       fprintf(fp,"========================A GOT A TIMER INTERRUPT================\n");
                    timer_buffer_A = queue<float>();
                    isTimerON_A = 0;

       fprintf(fp,"=========================CLEARING TIMER BUFFER : SIZE: %d",(int)timer_buffer_A.size());
     for(int i=0;i<num_sent_unacknowledged_packets_A;i++) {
               fprintf(fp,"====SENDING PACKETS OF SEQ NUM: %d AGAIN=====\n",(send_base_A+i)%(WINDOWSIZE+1) );
                printPacket(senderWindow_A[((send_base_A+i)%(WINDOWSIZE+1))]);
                tolayer3(A,senderWindow_A[((send_base_A+i)%(WINDOWSIZE+1))]);
                myStartTimer(A,INCREMENT);
            }
            fprintf(fp, "======AFTER A GOT A TIMER INTERRUPT WE HAVE SENT ALL PKTS AND RESTARTED TIMERS===\n" );
            printTimerBuffer(A);
    

}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    send_base_A = 0;
    send_nextSeqNum_A = 0;
    expectedSeqNum_A = 0;
    sendAgain_ACK_A.seqnum = -1;
    sendAgain_ACK_A.acknum = 7;

    sendAgain_ACK_A.checksum = createCheckSum(sendAgain_ACK_A);
     //  senderWindow_A = (pkt *) malloc(sizeof(pkt) * WINDOWSIZE);


}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/

void B_input(struct pkt packet)
{

    fprintf(fp,"B RECEIVED A PACKET\n");
    if(matchCheckSum(packet)==1 && packet.acknum== -1) { //packet is not corrupted
       fprintf(fp,"==================B RECEIVED GOOD PACKET AND IT'S EXPECTING %d=================\n",expectedSeqNum_B);

        printPacket(packet);
        if(packet.seqnum == expectedSeqNum_B) {
            fprintf(fp,"==========IN B MATCHES EXPECTED SEQNUM SO GOOOOO!!!!=========\n");
            struct pkt ACKPacket;
            strcpy(ACKPacket.payload,"");
            ACKPacket.seqnum = -1;
            ACKPacket.acknum = expectedSeqNum_B;
            ACKPacket.checksum = createCheckSum(ACKPacket);
            tolayer5(B,packet.payload);
            sendAgain_ACK_B = ACKPacket;
            fprintf(fp,"=================B IS SENDING ACK PACKET============\n");

            printPacket(sendAgain_ACK_B);
            cumulativeACK_B.push(sendAgain_ACK_B);
            starttimer(B,INCREMENT);
            tolayer3(B,sendAgain_ACK_B);
            expectedSeqNum_B = (expectedSeqNum_B+1)%(WINDOWSIZE+1);
         
        }
        else {
           fprintf(fp,"==========IN B DOESNT MATCH EXPECTED SEQNUM SENDING DUPLICATE PACKET=========\n");
         
            printPacket(sendAgain_ACK_B);
            tolayer3(B,sendAgain_ACK_B);
            
        }

    }
    else { // packet is corrupted
        fprintf(fp,"==================B RECEIVED FOLLOWING CORRUPTED PACKET=================\n");
        printPacket(packet);

           fprintf(fp,"=================B IS SENDING DUPLICATE ACK PACKET============\n");
            printPacket(sendAgain_ACK_B);
          tolayer3(B,sendAgain_ACK_B);
    }




    if(matchCheckSum(packet)==1 && packet.seqnum == -1) {
        if(fallsWithinWindow(B,packet.acknum)) {
            fprintf(fp,"==========B RECEIVED PACKET OF ACKNUM: %d CHECKSUM: %d FALLS WITHIN WINDOW======\n",packet.acknum,packet.checksum);
            //slide window to the next seq num after the acknum received
            int total_packets_acked = packetsACKed(packet.acknum);
            num_sent_unacknowledged_packets_B -= total_packets_acked;
            fprintf(fp,"======================B SENT BUT UNACKED PKTS: %d============\n",num_sent_unacknowledged_packets_B);

            
            
            fprintf(fp,"=============PRINTING WINDOW BEFORE SLIDING===========\n");
            printWindow(B);
            printTimerBuffer(B);
            prev_sendbase_B = send_base_B;
            send_base_B = (packet.acknum+1)%(WINDOWSIZE+1);
            for(int i=0;i<(num_sent_unacknowledged_packets_B-1);i++){
                if(!timer_buffer_B.empty()) {
                float timeOfPktsAlreadyAcked = timer_buffer_B.front();
                timer_buffer_B.pop();
                 }
            }
            for(int i=0;i<(WINDOWSIZE+1);i++) duplicateACK_B[i] = false;
            //send_nextSeqNum_B = send_base_B;
            //send_max = -1;
            fprintf(fp,"=============PRINTING WINDOW AFTER SLIDING===========\n");
            printWindow(B);
            printTimerBuffer(B);
            myStopTimer(B);
            while(!pkt_buffer_B.empty()) {
              struct  pkt packet = pkt_buffer_B.front();
                if(fallsWithinWindow(B,packet.seqnum)){
                   fprintf(fp,"====B IS SENDING FOLLOWING PACKET FROM BUFFER==================\n");
                    printPacket(packet);
                    pkt_buffer_B.pop();
                    senderWindow_B[packet.seqnum] = packet;
                    tolayer3(B,packet);
                    myStartTimer(B,INCREMENT);
                }
            }
        }

        else  if(!fallsWithinWindow(B,packet.acknum)){
               fprintf(fp,"==========B RECEIVED PACKET OF ACKNUM: %d CHECKSUM: %d DOESN'T FALL WITHIN WINDOW======\n",packet.acknum,packet.checksum);
                    if(duplicateACK_B[prev_sendbase_B]==false) {
                    stoptimer(A);
                    isTimerON_B  = 0;
                    timer_buffer_B = queue<float>();
                    for(int i=0;i<num_sent_unacknowledged_packets_B;i++) {
                        fprintf(fp,"====SENDING PACKETS OF SEQ NUM: %d AGAIN=====\n",(send_base_B+i)%(WINDOWSIZE+1) );

                        printPacket(senderWindow_B[((send_base_B+i)%(WINDOWSIZE+1))]);
                        tolayer3(B,senderWindow_B[((send_base_B+i)%(WINDOWSIZE+1))]);
                        //stoptimer(A);
                        myStartTimer(B,INCREMENT);
                    }
                     fprintf(fp, "======AFTER B GOT A DUPLICATE ACK WE HAVE SENT ALL PKTS AND RESTARTED TIMERS===\n" );
                    printTimerBuffer(B);
                    }
                    duplicateACK_B[prev_sendbase_B] = true;
    
           
        }
    }
    else {
        //packet is corrupted
    
    }

}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
  /*  if(!cumulativeACK_B.empty()) {
        struct pkt lastACK;
        lastACK = cumulativeACK_B.top();
        cumulativeACK_B.pop();
        tolayer3(B,lastACK);
        sendAgain_ACK_B = lastACK;
        fprintf(fp, "=======B GOT A TIMER INTERRUPT SO SENDING CUMULATIVE ACK===============\n");
        printPacket(lastACK);

    }
    else {
        tolayer3(B,sendAgain_ACK_B);
        fprintf(fp, "=======B GOT A TIMER INTERRUPT BUT BUFFER IS EMPTY SO SENDING THE LAST ACK AGAIN===============\n");
        printPacket(sendAgain_ACK_B);

    }
    cumulativeACK_B = stack<pkt>(); //clearing the queue*/
   
       fprintf(fp,"========================B GOT A TIMER INTERRUPT================\n");
                    timer_buffer_B = queue<float>();
                    isTimerON_B = 0;

       fprintf(fp,"=========================CLEARING TIMER BUFFER : SIZE: %d",(int)timer_buffer_B.size());
       for(int i=0;i<num_sent_unacknowledged_packets_B;i++) {
               fprintf(fp,"====SENDING PACKETS OF SEQ NUM: %d AGAIN=====\n",(send_base_B+i)%(WINDOWSIZE+1) );
                printPacket(senderWindow_B[((send_base_B+i)%(WINDOWSIZE+1))]);
                tolayer3(B,senderWindow_B[((send_base_B+i)%(WINDOWSIZE+1))]);
                myStartTimer(B,INCREMENT);
            }
            fprintf(fp, "=======AFTER B GOT A TIMER INTERRUPT WE HAVE SENT ALL PKTS AND RESTARTED TIMERS===\n" );
            printTimerBuffer(B);
    


  //  printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    expectedSeqNum_B = 0;
    sendAgain_ACK_B.seqnum = -1;
    sendAgain_ACK_B.acknum = 7;

    sendAgain_ACK_B.checksum = createCheckSum(sendAgain_ACK_B);
    send_base_B= 0;
    send_nextSeqNum_B = 0;


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



void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    fp = fopen("output_gbn_13.txt", "w");//
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

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
            printf("\nEVENT time: %f,", eventptr->evtime);
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
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
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
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
        time, nsim);
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
