//CoAPclient.c
//code copied form github.com/obgm/libcoap-minimal/blob/master/client.cc

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include <address.h>
//#include <sys/epolĺ.h>

#include <coap2/coap.h>

#define MAXINPUT 100

//#include "resolveAdd.cpp"
int resolve_address(const char* host, const char* service, coap_address_t* dst);	//make proper header file if more functions are needed

void* nullptr = NULL;	//translator from c++ to c. Comment out when compiling as C++

void simple_resp_handler (struct coap_context_t *context, coap_session_t *session, coap_pdu_t *sent, coap_pdu_t *received, const coap_tid_t id)
{
	(void)context;
	(void)session;
	(void)sent;
	(void)id;
	char *type[4];
	switch(received->type)
    {
        case 0: *type = "CON"; break;
        case 1: *type = "NON"; break;
        case 2: *type = "ACK"; break;
        case 3: *type = "RST"; break;
        default: *type = "???";
    }

    int codeClass = (0xe0 & received->code) >> 5;
	int codeDetail = 0x1f & received->code;
	printf("received datagram: <ID: %i, Token(complicated): %i>\n"
        "\tType:\t%s\n"
            "\tCode:\t%i.%i\n", id, -1, *type, codeClass, codeDetail);
	if(received->data != 0) printf("\tPayload:\n\t\t%s\n", received->data);
	else printf("\tPayload: none\n");
	
	return;
}


int main (int argc, char* argv[])
{
	//parse arguments: "udpSender, IPv6 address, port, message"
	if( argc < 5 )
	{
		printf("Missing arguments, usage: %s <IPv6 address> <Port> <Message> <Code>\n", argv[0]);
		return 0;
	}
	else if(strcmp(argv[1], "help") == 0)
    {
	    printf("Usage: %s <IPv6 address> <Port> <Message> <Code>\n", argv[0]);
	    return 0;
    }
	char* addr = argv[1];
	char* port = argv[2];
	char* msg = argv[3];
	coap_request_t code;
	if(strcmp("put", argv[4]) == 0 || strcmp("PUT", argv[4]) == 0)
	    code = COAP_REQUEST_PUT;
    else if(strcmp("get", argv[4]) == 0 || strcmp("GET", argv[4]) == 0)
        code = COAP_REQUEST_GET;
    else code = COAP_REQUEST_GET; //temp fix
	
	
	coap_context_t* ctx = nullptr;
	coap_session_t* session = nullptr;
	coap_address_t dst;
	coap_pdu_t* pdu = nullptr;
	int result = EXIT_FAILURE;
	
	coap_startup();


    //printf("DEBUG: STARTUP");

	
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

    //register response handler
    coap_register_response_handler(ctx, simple_resp_handler);
	
	//Construct CoAP message	(pdu seems to represent the CoAP datagram)
	pdu = coap_pdu_init(COAP_MESSAGE_CON, code, 0 /*message ID (for ACK-matching)*/, coap_session_max_pdu_size(session));
	if(!pdu)
	{
		coap_log(LOG_EMERG, "cannot create PDU\n");
		goto finish;
	}
	
	//add a URI-path option
	if(coap_add_option(pdu, COAP_OPTION_URI_PATH, strlen(msg), /* reinterpret_cast<const uint8_t*>(msg) */ (const uint8_t*)msg) == COAP_INVALID_TID)
	{
		coap_log(LOG_EMERG, "failed to send message");
		goto finish;
	}
	

	coap_send(session, pdu); //send the pdu


	char input[MAXINPUT]; //100
	int id = 1;

	int res;
	while(1)
	{
        //printf("DEBUG: LOOPING");
		res = coap_io_process(ctx, 0);	//not sure what this is ("main message processing loop" according to documentation) (Replaced depricated function coap_run_once)	

		//internal error
		if(res < 0)
			break;

        fgets(input, MAXINPUT, stdin); //read up to 100 characters from standard input

        //Construct CoAP message	(pdu seems to represent the CoAP datagram)
        pdu = coap_pdu_init(COAP_MESSAGE_CON, code, id++ /*message ID (for ACK-matching)*/, coap_session_max_pdu_size(session));
        if(!pdu)
        {
            coap_log(LOG_EMERG, "cannot create PDU\n");
            goto finish;
        }

        //add a URI-path option
        if(coap_add_option(pdu, COAP_OPTION_URI_PATH, strlen(input), /* reinterpret_cast<const uint8_t*>(msg) */ (const uint8_t*)input) == COAP_INVALID_TID)
        {
            coap_log(LOG_EMERG, "failed to send message");
            goto finish;
        }

        coap_send(session, pdu); //send the pdu
	}
	
	result = EXIT_SUCCESS;
finish:
	 
	 coap_session_release(session);
	 coap_free_context(ctx);
	 coap_cleanup();
	 
	 return result;
}