//CoAPclient.c
//code copied form github.com/obgm/libcoap-minimal/blob/master/client.cc

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include <address.h>

#include <coap2/coap.h>

//#include "resolveAdd.cpp"
int resolve_address(const char* host, const char* service, coap_address_t* dst);	//make proper header file if more functions are needed

void* nullptr = NULL;	//translator from c++ to c. Comment out when compiling as C++

int main (int argc, char* argv[])
{
	//parse arguments: "udpSender, IPv6 address, port, message"
	if( argc < 4 )
	{
		printf("missing arguments: %s <IPv6 address> <Port> <Message>\n", argv[0]);
		return 0;
	}
	char* addr = argv[1];
	char* port = argv[2];
	char* msg = argv[3];
	
	
	coap_context_t* ctx = nullptr;
	coap_session_t* session = nullptr;
	coap_address_t dst;
	coap_pdu_t* pdu = nullptr;
	int result = EXIT_FAILURE;
	
	coap_startup();
	
	if(resolve_address(addr, port, &dst) < 0)			//call function in resolveAdd.cpp to resolve address and port into a coap_address struct
	{
		coap_log(LOG_CRIT, "failed to resolve address\n");
		goto finish;
	}
		
	ctx = coap_new_context(nullptr);									//create CoAP context
	session = coap_new_client_session(ctx, nullptr, &dst, COAP_PROTO_UDP);	//create CoAP session
	if(!ctx || !session)
	{
		coap_log(LOG_EMERG, "cannot create client session\n");
		goto finish;		
	}
	
	//Construct CoAP message	(pdu seems to represent the CoAP datagram)
	pdu = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_GET, 0 /*message ID (for ACK-matching)*/, coap_session_max_pdu_size(session));
	if(!pdu)
	{
		coap_log(LOG_EMERG, "cannot create PDU\n");
		goto finish;
	}
	
	//add a URI-path option
	if(coap_add_option(pdu, COAP_OPTION_URI_PATH, 5, /* reinterpret_cast<const uint8_t*>(msg) */ (const uint8_t*)msg) == COAP_INVALID_TID)
	{
		coap_log(LOG_EMERG, "failed to send message");
		goto finish;
	}
	
	coap_send(session, pdu); //send the pdu
	
	coap_io_process(ctx, 0);	//not sure what this is ("main message processing loop" according to documentation) (Replaced depricated function coap_run_once)	
	
	result = EXIT_SUCCESS;
finish:
	 
	 coap_session_release(session);
	 coap_free_context(ctx);
	 coap_cleanup();
	 
	 return result;
}