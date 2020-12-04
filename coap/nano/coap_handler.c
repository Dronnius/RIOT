/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
//#include <sys/types.h>
#include <thread.h>

#include "shell.h"
//#include "../../sys/shell/commands/shell_commands.c"
#include "fmt.h"
#include "net/nanocoap.h"
//#include "hashes/sha256.h"
#include "extra.h"
#include "../../sys/include/net/netif.h"	//ugly, hopefully temporary solution
#include "../../sys/include/net/gnrc/netif.h"	//ugly, hopefully temporary solution
#include "../../sys/include/net/gnrc/netapi.h"	//ugly, hopefully temporary solution

#define ENABLE_DEBUG (1)

/* internal value that can be read/written via CoAP */
static uint8_t internal_value = 0;

//debugging stuff:
//static const shell_command_t * extra_shell_commands = NULL;
//int dint = 3;

/*void print_dint(void)
{
    printf("checking threading (thread: %i):", thread_getpid());
    printf("\nDINT\n\t<**> <DEBUG> dint = %i,\t &dint = %lx,\t thread ID = %i <DEBUG> <**>\n", dint, &dint, thread_getpid);
    printf("\nEXTRAS\n\t <**> <DEBUG> extra = %lx,\t &extra = %lx,\t thread ID = %i <DEBUG> <**>\n", extra_shell_commands, &extra_shell_commands, thread_getpid());

    return;
}*/


/*DEBUG        //FOR SOME REASON (PROBABLY DUE TO THREADING) THE SHELL AND THE SERVER HAVE DIFFERENT ADDRESSES FOR THE EXTRA COMMANDS
void check_extras(void)
{
    printf("[-][-][-] Extras memory position: %lx [-][-][-]\n", extra_shell_commands);    //debug
    printf("\t***Extra commands***\n");
    const shell_command_t* peek = shell_commands;
    if(peek->name == NULL) printf("\t\tNO EXTRA COMMANDS\n");
    while(peek->name != NULL)
    {
        printf("\t\t%s, %s\n", peek->name, peek->desc);
        peek = peek + 1;
    }
    printf("\t***End of Extras***\n");
}
//DEBUG*/

/*void set_extra_commands(const shell_command_t* extras)
{
    if( extras != NULL)
        extra_shell_commands = extras;
    check_extras(); //debug
}*/
/*DEBUG
void wait_until_issue(void)
{
    int i = 0;
    while(extra_shell_commands->name != NULL)
    {
        i++;
        thread_yield();
    }
    printf("\t!!!\tDEBUG: Extra shell commands \"nulled\" after %i iterations.", i);
    return;
}
//DEBUG*/

static const uint8_t block2_intro[] = "This is RIOT (Version: ";
static const uint8_t block2_board[] = " running on a ";
static const uint8_t block2_mcu[] = " board with a ";

static ssize_t _echo_handler(coap_pkt_t * pkt, uint8_t * buf, size_t len, void * context)
{
#ifdef ENABLE_DEBUG
	printf("DID AN ECHO THING!\n");		//DEBUG!!!
#endif
    (void)context;
    char uri[CONFIG_NANOCOAP_URI_MAX];

    if (coap_get_uri_path(pkt, (uint8_t *)uri) <= 0) {
        return coap_reply_simple(pkt, COAP_CODE_INTERNAL_SERVER_ERROR, buf,
                                 len, COAP_FORMAT_TEXT, NULL, 0);
    }
    char *sub_uri = uri + strlen("/echo/");
    size_t sub_uri_len = strlen(sub_uri);
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len, COAP_FORMAT_TEXT,
                             (uint8_t *)sub_uri, sub_uri_len);
}

static ssize_t _riot_board_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
#ifdef ENABLE_DEBUG
	printf("DID A BOARD THING!");		//DEBUG!!!
#endif
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
}

static ssize_t _riot_block2_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
#ifdef ENABLE_DEBUG
	printf("DID A VER THING\n");		//DEBUG!!!
#endif
    (void)context;
    coap_block_slicer_t slicer;
    coap_block2_init(pkt, &slicer);
    uint8_t *payload = buf + coap_get_total_hdr_len(pkt);

    uint8_t *bufpos = payload;

    bufpos += coap_put_option_ct(bufpos, 0, COAP_FORMAT_TEXT);
    bufpos += coap_opt_put_block2(bufpos, COAP_OPT_CONTENT_FORMAT, &slicer, 1);
    *bufpos++ = 0xff;

    // Add actual content
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_intro, sizeof(block2_intro)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_VERSION, strlen(RIOT_VERSION));
    bufpos += coap_blockwise_put_char(&slicer, bufpos, ')');
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_board, sizeof(block2_board)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_mcu, sizeof(block2_mcu)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_MCU, strlen(RIOT_MCU));
    // To demonstrate individual chars
    bufpos += coap_blockwise_put_char(&slicer, bufpos, ' ');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'M');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'C');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'U');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, '.');


    unsigned payload_len = bufpos - payload;
    return coap_block2_build_reply(pkt, COAP_CODE_205,
                                   buf, len, payload_len, &slicer);
}

static ssize_t _riot_value_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
#ifdef ENABLE_DEBUG
	printf("DID A VALUE THING\n");		//DEBUG!!!
#endif
    (void) context;

    ssize_t p = 0;
    char rsp[16];
    unsigned code = COAP_CODE_EMPTY;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pkt));

    switch(method_flag) {
    case COAP_GET:
        /* write the response buffer with the internal value */
        p += fmt_u32_dec(rsp, internal_value);
        code = COAP_CODE_205;
        break;
    case COAP_PUT:
    case COAP_POST:
    {
        /* convert the payload to an integer and update the internal value */
        char payload[16] = { 0 };
        memcpy(payload, (char*)pkt->payload, pkt->payload_len);
        internal_value = strtol(payload, NULL, 10);
        code = COAP_CODE_CHANGED;
    }
    }

    return coap_reply_simple(pkt, code, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)rsp, p);
}
/*
ssize_t _sha256_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;

    // using a shared sha256 context *will* break if two requests are handled
    // at the same time.  doing it anyways, as this is meant to showcase block1
    // support, not proper synchronisation.
    static sha256_context_t sha256;

    uint8_t digest[SHA256_DIGEST_LENGTH];

    uint32_t result = COAP_CODE_204;

    coap_block1_t block1;
    int blockwise = coap_get_block1(pkt, &block1);

    printf("_sha256_handler(): received data: offset=%u len=%u blockwise=%i more=%i\n", \
            (unsigned)block1.offset, pkt->payload_len, blockwise, block1.more);

    if (block1.offset == 0) {
        puts("_sha256_handler(): init");
        sha256_init(&sha256);
    }

    sha256_update(&sha256, pkt->payload, pkt->payload_len);

    if (block1.more == 1) {
        result = COAP_CODE_CONTINUE;
    }

    size_t result_len = 0;
    if (!blockwise || !block1.more) {
        puts("_sha256_handler(): finish");
        sha256_final(&sha256, digest);
        result_len = SHA256_DIGEST_LENGTH * 2;
    }

    ssize_t reply_len = coap_build_reply(pkt, result, buf, len, 0);
    uint8_t *pkt_pos = (uint8_t*)pkt->hdr + reply_len;
    if (blockwise) {
        pkt_pos += coap_opt_put_block1_control(pkt_pos, 0, &block1);
    }
    if (result_len) {
        *pkt_pos++ = 0xFF;
        pkt_pos += fmt_bytes_hex((char *)pkt_pos, digest, sizeof(digest));
    }

    return pkt_pos - (uint8_t*)pkt->hdr;
}
*/

// ===== WORK IN PROGRESS ===== //
int readArgs (char* source, int sourceLen, char** args, int maxArgs)
{
	//printf("debuggy: A\n");		//DEBUG
	if (maxArgs < 1 || sourceLen < 1) return -1;

	char* brk = source;
	args[0] = source;
	int i = 0, count = 1;
		//printf("debuggy: B\n");		//DEBUG
	while(i++ < sourceLen && count < maxArgs)      //cycle through the entire argument string
    {
		//printf("debuggy: C.%i\n", i);		//DEBUG
		//printf("arg[%i]:\t%c\t(%i)\n", i, *brk, (int)*brk); //DEBUG
		if(*brk == ' ')       //at every whitespace
		{
			*brk = '\0';        //insert a string-termination character
			if(*(brk + 1) == '\0' || i >= sourceLen)	//in case of trailing whitespace/slash
				break;
			args[count++] = brk + 1; //add next character as beginning of next argument
			if(count >= maxArgs)	//if max arguments reached, cut of remainder
			{
				do {
					brk++;
					if(*brk == ' ')
						*brk = '\0';
				} while(*brk != '\0');
				break;
			}
		}
		brk++;  //increment pointer
    }
		//printf("debuggy: D\n");		//DEBUG

	    //*DEBUG
	    printf("\n---Arguments:\n");
	    for(i = 0; i < count; i++)
	      {printf("\targs[%i]:\t%s\n", i, args[i]);}
	    printf("\n");
	    //DEBUG*/

    return count;  //reverse 0-indexing offset
}

static ssize_t _channel_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void* context)
{
	//PARSE URI
	char* args[] =  {NULL, NULL};
    char source[CONFIG_NANOCOAP_URI_MAX];
    int sourceLen = coap_get_uri_path(pkt, (uint8_t *)source) - strlen("/config/channel/");
    (void) context;
	(void) len;
	(void) buf;

	int count = readArgs(source + strlen("/config/channel/"), sourceLen, args, 2);
	//DIVIDE INTO ARGUMENTS
	/*args[0] = arg + strlen("/config/channel/");
    //int count = 0;
    char* brk = args[0];
    int i = 0;
	short done = false;
    while(i++ < uri_len)      //cycle through the entire argument string
    {
		//printf("arg[%i]:\t%c\t(%i)\n", i, *brk, (int)*brk); //DEBUG
		if(*brk == ' ' || *brk == '/')       //at every whitespace or slash...
		{
			*brk = '\0';     //insert a string-termination character
			if(done)
				break;
			args[1] = brk + 1; //add next character as beginning of next argument
			done = true;
		}
		brk++;  //increment pointer
    }
	//count++;	//offset 0-index

	//DEBUG
    printf("\n---Arguments:\n");
    for(i = 0; i < 2; i++)
      {printf("\targs[%i]:\t%s\n", i, args[i]);}
    printf("\n");
    //DEBUG*/

	//Validate arguments
	if(count < 2)	//respond with format
	{
        char payload[] = "PUT usage: config/channel/[interface] [new value]";
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
		//note: check interface matches, and value is... a value
	netif_t* iface = netif_get_by_name(args[0]);
	if (!iface)
	{
        char payload[] = "Failure: Invalid interface";
		printf("%s\n", payload);
        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
	if (!fmt_is_number(args[1])) //negative values shouldd fail here also
	{
	        char payload[] = "Failure: Invalid value, must be a 16-bit unsigned integer";
			printf("%s\n", payload);
	        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
	printf("testy\n");
	uint16_t value = (uint16_t)(0x0ffff & strtoul(args[1], NULL, 10));
	if(value == 0 && *args[1] != '0')	//if
	{
	        char payload[] = "Failure: Value failed to convert into 16-bit unsigned integer";
			printf("%s\n", payload);
	        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
    //gnrc_netif_t *iface = (gnrc_netif_t*) netif;

	//set new value
    if(_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, NETOPT_CHANNEL, 0, &value, sizeof(uint16_t), GNRC_NETAPI_MSG_TYPE_SET) < 0)	//fail
	{
		char payload[] = "Failure: New channel value could not be set";
		printf("%s\n", payload);
		return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
	else //success
	{
		char payload[] = "Success: New channel value set";
	  printf("%s\n", payload);
	  return coap_reply_simple(pkt, coap_code(2, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
}

static ssize_t _shell_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    const int maxArgs = 10;
    //PARSE URI
    char* args[maxArgs];
    char source[CONFIG_NANOCOAP_URI_MAX];
	int sourceLen = coap_get_uri_path(pkt, (uint8_t *)source) - strlen("/shell/");
    //args[0] = source + strlen("/shell/");
    (void) context;
    //COLLECT ARGUMENTS
	int argCount = readArgs(source + strlen("/shell/"), sourceLen, args, maxArgs);
	if(argCount == -1)	//if error reading arguments
	{
		printf("\n");
	        char payload[] = "Failure:\t\tError reading arguments";
	        printf("%s\n", payload);
	        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}

    /*int count = 0;
    char* brk = args[0];
    int i = 0;
    while(i++ < uri_len && count < MAX_ARGUMENTS - 1)      //cycle through the entire argument string
    {
      //printf("arg[%i]:\t%c\t(%i)\n", i, *brk, (int)*brk); //DEBUG
      if(*brk == ' ')       //at every whitespace...
      {
        *brk = '\0';        //insert a string-termination character
        args[++count] = brk + 1; //add next character as beginning of next argument
      }
      brk++;  //increment pointer
    }
    count++;  //reverse 0-indexing offset
    //printf("\n#-#-#: # of arguments : %i #-#-#\n", count);  //DEBUG
*/
    /*DEBUG
    printf("\n---Arguments:\n");
    for(i = 0; i < count; i++)
      {printf("\targs[%i]:\t%s\n", i, args[i]);}
    printf("\n");
    //DEBUG*/

    //CALL FUNCTION
    const shell_command_t *command_lists[] = {
            shell_commands,                             //TODO: Defend against undefined shell_commands
#ifdef MODULE_SHELL_COMMANDS
            _shell_command_list,
#endif
    };

    int result = -123;
    /* iterating over command_lists */
    for (unsigned int i = 0; i < ARRAY_SIZE(command_lists) && result == -123; i++) {

        const shell_command_t *entry;

        if (!(entry = command_lists[i])) { continue; }
            /* iterating over commands in command_lists entry */
        while (entry->name != NULL) {
          if(strcmp(entry->name, args[0]) == 0)
          {
            printf("FOUND FUNCTION: %s,\t%s\n", entry->name, entry->desc);
            result = entry->handler(argCount, args); // Call function
            break;
          }
          entry++;
        }
    }

    //RETURN DEMI-RESULT
    if(result == 0)
    {
        char payload[8] = "Success";
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
    else if (result == -123)
    {
        char payload[] = "Failure: Function not found.";
        printf("%s\n", payload);
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
    else
    {
        char payload[8] = "Failure";
        printf("%s\n", payload);
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
}

//const coap_resource_t coap_resources[]; //declaration

static ssize_t _help_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
	#define RESPONSE_BUFFER_SIZE 500
	char response[RESPONSE_BUFFER_SIZE];
	(void) pkt;
	(void) buf;
	(void) len;
	(void) context;

	//loop through coap resources
		//@ "/shell/", also loop through all shell commands
	unsigned int respIndex = 0;
	unsigned int s_len;
	//int respSize = 0;
	//coap_resource_t resource = coap_resources[0];
	for(unsigned int i = 0; i < coap_resources_numof; i++)
	{
		s_len = strlen(coap_resources[i].path) - 1; // get length of resource path, minus one to remove preceeding '/' character (answer does NOT include terminating null-character)

		//handle excess data
		if( respIndex + s_len + 1 >= sizeof(response))	// if end of response buffer is reached (+1 for \n that shall be inserted)	(NO TERMINATING NULL CHARACTER INCLUDED, NOT SURE IF NEEDED)
		{
			/*response[respIndex - 1] = '\0';	// -1 to overwrite \n (hopefully)
			printf("response #%i:\n%s\n", i + 1, response);
			respSize += coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)response, respIndex);
			respIndex = 0;*/
			printf(">-- HELP ERROR: Response buffer of %i bytes exceeded!\n", RESPONSE_BUFFER_SIZE);
			char payload[] = "Server error: too much data";
			return coap_reply_simple(pkt, coap_code(5, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
		}

		//add string to response buffer (remove terminating null-character and preceeding '/'-character(that's the +1) )
		strcpy( (response + respIndex), (coap_resources[i].path + 1) );
		respIndex += s_len;		// increment respIndex by length of path (excluding preceeding '/')

		//add a newline!
		*(response + respIndex) = '\n';
		respIndex++;

		//if it was "/shell/"...
		if(strcmp(coap_resources[i].path, "/shell/") == 0)
		{
			    const shell_command_t *command_lists[] = {
				    shell_commands,                             //TODO: Defend against undefined shell_commands
			#ifdef MODULE_SHELL_COMMANDS
				    _shell_command_list,
			#endif
			    };

			    // iterating over command_lists
			    for (unsigned int j = 0; j < ARRAY_SIZE(command_lists); j++) {
					//printf("value of j: %i\n", j);

					const shell_command_t *entry;
					//unsigned int d_len;

					if ((entry = command_lists[j])) {
						    // iterating over commands in command_lists entry
						    while (entry->name != NULL) {
								//printf("%-20s %s\n", entry->name, entry->desc);

								s_len = strlen(entry->name); // get length of command name (answer does NOT include terminating null-character)
								//d_len = strlen(entry->desc); // get length of command description (answer does NOT include terminating null-character)

								//handle excess data
								if( respIndex + s_len + /*d_len + 4 +*/ 2 + strlen("shell/") >= sizeof(response))	// if end of response buffer is reached (+1 for \n that shall be inserted, +2 for 3 * \t to be inserted)
								{
									printf(">-- HELP ERROR: Response buffer of %i bytes exceeded!\n", RESPONSE_BUFFER_SIZE);
									char payload[] = "Server error: too much data";
									return coap_reply_simple(pkt, coap_code(5, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
								}


								//add prefix to response buffer
								strcpy( (response + respIndex), "\tshell/" );
								respIndex += strlen("\tshell/");
								//add name to response buffer
								strcpy( (response + respIndex), (entry->name) );
								respIndex += s_len;		// increment respIndex by length of path
								//add two tab-characters!
								//strcpy( (response + respIndex), "\t\t" );
								//respIndex += 2;
								//add description to response buffer
								//strcpy( (response + respIndex), (entry->desc) );
								//respIndex += d_len;		// increment respIndex by length of path
								//add a newline!
								*(response + respIndex) = '\n';
								respIndex++;

								entry++;
						    }//printf("---End of list---\n");   //debug
					}// else {printf("ERROR: empty command list!\n");}    //debug
			    }
		}


	}

	response[respIndex - 1] = '\0';
	printf("Response:\n%s\n", response);
	return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)response, respIndex);
}


// ===== END OF WORK AREA ===== //

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    COAP_WELL_KNOWN_CORE_DEFAULT_HANDLER,
    { "/echo/", COAP_GET | COAP_MATCH_SUBTREE, _echo_handler, NULL },
    { "/help", COAP_GET, _help_handler, NULL },         //mine
    { "/config/channel/", COAP_GET | COAP_PUT | COAP_MATCH_SUBTREE, _channel_handler, NULL},    //mine
    { "/riot/board", COAP_GET, _riot_board_handler, NULL },
    { "/riot/value", COAP_GET | COAP_PUT | COAP_POST, _riot_value_handler, NULL },
    { "/riot/ver", COAP_GET, _riot_block2_handler, NULL },
    { "/shell/", COAP_PUT | COAP_GET | COAP_MATCH_SUBTREE, _shell_handler, NULL},    //mine
    //{ "/sha256", COAP_POST, _sha256_handler, NULL },
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
