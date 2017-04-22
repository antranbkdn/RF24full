#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include "./RF24.h"

#define MSGID_BROADCAST_NODE_QUE_US 0x0001
#define MSGID_UNICAST_RES_US 0x0002
#define MSGID_UNICAST_NODE_accept_US 0x0003
#define MSGID_FULL_CHILD_US 0x0004

using namespace std;

RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes_gw[6] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0D3LL, 0xF0F0F0F0D4LL, 0xF0F0F0F0D5LL, 0xF0F0F0F0D6LL};
const uint64_t pipes_dv[6] = { 0xF0F0F0F0E1LL, 0xF0F0F0AAD2LL, 0xF0F0F0AAD3LL, 0xF0F0F0AAD4LL, 0xF0F0F0AAD5LL, 0xF0F0F0AAD6LL};



const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increments_by = 1;
int next_payload_size = min_payload_size;

char receive_payload[max_payload_size+1]; // +1 to allow room for a terminating NULL char

/*********** Struct define ***************/
typedef struct NodeQuery {
	uint16_t MSGID;
	uint32_t SourceID;
	uint64_t RXAddress;
	unsigned long time;
} NodeQuery;

typedef struct NodeResponse {
	uint16_t MSGID;
	uint32_t SourceID;
	uint64_t RXAddress;
} NodeResponse;

typedef struct NodeFinish {
	uint16_t MSGID;
	uint32_t SourceID;
	bool accept;
} NodeFinish;

typedef struct NodeFull {
	uint16_t MSGID;
	uint32_t SourceID;
	uint16_t numchild;
} NodeFull;


int main(int argc, char** argv){

  bool device = 1, gateway = 0;
  bool role = 0;

  // Print preamble:
  cout << "RF24/examples/pingpair_dyn/\n";

  // Setup and configure rf radio
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(5,15);
  radio.printDetails();


/********* Role chooser ***********/

  printf("\n ************ Role Setup ***********\n");
  string input = "";
  char myChar = {0};
  cout << "Choose a role: Enter 0 for gateway, 1 for device (CTRL+C to exit) \n>";
  getline(cin,input);

  if(input.length() == 1) {
	myChar = input[0];
	if(myChar == '0'){
		cout << "Role: Gateway, Broadcast query" << endl << endl;
	}else{  cout << "Role: Device, waiting" << endl << endl;
		role = device;
	}
  }
/***********************************/

    if ( role == gateway )    {
      radio.openReadingPipe(0,pipes_gw[0]);
      radio.openReadingPipe(1,pipes_gw[1]);
      radio.openReadingPipe(2,pipes_gw[2]);
      radio.openReadingPipe(3,pipes_gw[3]);
      radio.openReadingPipe(4,pipes_gw[4]);
      radio.openReadingPipe(5,pipes_gw[5]);
    } else {
      radio.openReadingPipe(0,pipes_dv[0]);
      radio.openReadingPipe(1,pipes_dv[1]);
      radio.openReadingPipe(2,pipes_dv[2]);
      radio.openReadingPipe(3,pipes_dv[3]);
      radio.openReadingPipe(4,pipes_dv[4]);
      radio.openReadingPipe(5,pipes_dv[5]);
      radio.startListening();
    }


// forever loop
	while (1)
	{

if (role == gateway)
  {
     static NodeQuery* nodequery = new NodeQuery;
     nodequery->MSGID = MSGID_BROADCAST_NODE_QUE_US;
     nodequery->SourceID = 'A';
     nodequery->RXAddress = pipes_gw[1];
     nodequery->time = 100;

     NodeResponse noderesponse;
    radio.stopListening();
    radio.openWritingPipe (pipes_gw[0]);
    radio.write( nodequery, sizeof (NodeQuery) );

    radio.openReadingPipe(0,pipes_gw[0]);
    radio.startListening();

    unsigned long started_waiting_at = millis();
    bool timeout = false;
    uint8_t in_pipe = 1;
    while ( ! radio.available(&in_pipe) && ! timeout )
      if (millis() - started_waiting_at > 1000 )
        timeout = true;

    if ( timeout ){
      printf("Failed, response timed out.\n\r");
    }
    else {
      uint8_t len = radio.getDynamicPayloadSize();
      radio.read( &noderesponse, len );
      printf("Got response from %d\n\r", noderesponse.SourceID);
    }
/*** FINISH ***/
     static NodeFinish* nodefinish = new NodeFinish;
     nodefinish->MSGID = MSGID_UNICAST_NODE_accept_US;
     nodefinish->SourceID = 'A';
     nodefinish->accept = true;
    radio.stopListening();
    radio.openWritingPipe (noderesponse.RXAddress);
    radio.write( nodefinish, sizeof (NodeFinish) );

    radio.openReadingPipe(0,pipes_gw[0]);
    radio.startListening();   

return  0;
  }


  if ( role == device )
  {
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    uint8_t in_pipe = 0;
    NodeQuery nodequery;
    while ( ! radio.available(&in_pipe) && ! timeout )
      if (millis() - started_waiting_at > 1000 )
        timeout = true;

    if ( timeout ){
      printf("Failed, response timed out.\n\r");
    }
    else {
      uint8_t len = radio.getDynamicPayloadSize();
      radio.read( &nodequery, len );
      printf("Got response from %d\n\r", nodequery.SourceID);
      radio.stopListening();
      static NodeResponse* noderesponse = new NodeResponse;
      noderesponse->MSGID = MSGID_UNICAST_RES_US;
      noderesponse->SourceID  = 'B';
      noderesponse->RXAddress = pipes_dv[1];
      radio.stopListening();
      radio.openWritingPipe (nodequery.RXAddress);
      radio.write( noderesponse, sizeof (NodeResponse) );
      printf("Sent response.\n\r");

      radio.openReadingPipe(0,pipes_dv[0]);
      radio.startListening();

/*** FINISH ***/

    NodeFinish nodefinish;
    in_pipe = 1;
    while ( ! radio.available(&in_pipe) && ! timeout )
      if (millis() - started_waiting_at > 1000 )
        timeout = true;

    if ( timeout ){
      printf("Failed, response timed out.\n\r");
    }
    else {
      uint8_t len = radio.getDynamicPayloadSize();
      radio.read( &nodefinish, len );
      printf("Got payload from %d - Gateway accept: %d\n\r", nodefinish.SourceID, nodefinish.accept);
    }
    radio.openReadingPipe(0,pipes_dv[0]);
    radio.startListening();
    }
  }
}
}


