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

#define ENABLE_DEBUG (1)

/* internal value that can be read/written via CoAP */
static uint8_t internal_value = 0;
static const shell_command_t * extra_shell_commands = NULL;
int dint = 3;

void print_dint(void)
{
    printf("checking threading (thread: %i):", thread_getpid());
    printf("\nDINT\n\t<**> <DEBUG> dint = %i,\t &dint = %lx,\t thread ID = %i <DEBUG> <**>\n", dint, &dint, thread_getpid);
    printf("\nEXTRAS\n\t <**> <DEBUG> extra = %lx,\t &extra = %lx,\t thread ID = %i <DEBUG> <**>\n", extra_shell_commands, &extra_shell_commands, thread_getpid());

    return;
}


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

void set_extra_commands(const shell_command_t* extras)
{
    if( extras != NULL)
        extra_shell_commands = extras;
    check_extras(); //debug
}
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

    /* Add actual content */
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_intro, sizeof(block2_intro)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_VERSION, strlen(RIOT_VERSION));
    bufpos += coap_blockwise_put_char(&slicer, bufpos, ')');
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_board, sizeof(block2_board)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_mcu, sizeof(block2_mcu)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_MCU, strlen(RIOT_MCU));
    /* To demonstrate individual chars */
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

//TODO:
// parse arguments          X
// move to thread           X
// execute with arguments   X
// NEW - remove threading   X
// return result            /
// figure out command list  X
// generalize caller        X
// retrieve command list    X
// finalize                 O
//
//NOTES:
// How do I change stdout during ifconfig call?
//
extern int _gnrc_netif_config(int argc, char **argv);
// full list of commands (excluding programmer-defined?) can be found in variable _shell_command_list
// in directory RIOT/sys/shell/commands/shell_commands.c

static ssize_t _ifconfig_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    // parse argument
    char uri[CONFIG_NANOCOAP_URI_MAX];
    int uri_len;
    uri_len = coap_get_uri_path(pkt, (uint8_t *)uri);
    char* arg = uri + strlen("/ifconfig/");

    //printf("DEBUG\treceived argument: %i\n", newVal);
    (void)uri_len;
    (void)context;

    // TESTING SHELL ACCESSIBILITY>>>>>>>>>>>>>>>>>>>>>>>>>
        //print_dint();       //DINT DEBUG
            printf("%-20s %s\n", "Command", "Description");
            puts("---------------------------------------");
            const shell_command_t *command_lists[] = {
                    shell_commands,                             //TODO: Defend against undefined shell_commands
        #ifdef MODULE_SHELL_COMMANDS
                    _shell_command_list,
        #endif
            };

            // iterating over command_lists
            for (unsigned int i = 0; i < ARRAY_SIZE(command_lists); i++) {

                const shell_command_t *entry;

                if ((entry = command_lists[i])) {
                    // iterating over commands in command_lists entry
                    while (entry->name != NULL) {
                        printf("%-20s %s\n", entry->name, entry->desc);
                        entry++;
                    }printf("---End of list---\n");   //debug
                } else {printf("ERROR: empty command list!\n");}    //debug
            }
    // END OF SHELL ACCESS TEST<<<<<<<<<<<<<<<<<<<<<<<

    char* temp;
    char* args[10];
    char command[9] = "ifconfig";
    args[0] = command; //set first argument
    args[1] = arg;  //set second argument
    int i = 1;
    if (*arg != '\0')
    {
        while ((temp = strchr((char *) args[i++], (int) ' ')) != NULL && i < 10) //loop until last occurence
        {
            args[i] = temp;
            *args[i] = '\0';    //exchange spaces with NULL characters
            args[i]++; //shift to next character (instead of the space)
        }
        if (i >= 10 && strchr((char *) args[9], (int) ' ') != NULL) {
            printf("ERROR: too many arguments for command handler");
            char payload[29] = "Failure (too many arguments)";
            return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t *) payload,
                                     sizeof(payload));
        }

        //print arguments (debugging reasons)
        printf("Arguments (%i):\n", i);
        for (int j = 0; j < i; j++)
            printf("\t%i) \t%s\n", j, args[j]);
    }
    else
    {
        printf("<*> Special case: no arguments <*>\n"); //debug
        i = 1;
    }
    int result = _gnrc_netif_config(i, args);

    if(result == 0)
    {
        char payload[8] = "Success";
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
    else
    {
        char payload[8] = "Failure";
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }

}

static ssize_t _shell_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    #define MAX_ARGUMENTS 10
    //PARSE URI
    char* args[MAX_ARGUMENTS];
    char arg[CONFIG_NANOCOAP_URI_MAX];
    int uri_len;
    uri_len = coap_get_uri_path(pkt, (uint8_t *)arg);
    args[0] = arg + strlen("/shell/");
    (void) uri_len;
    (void) context;
    //COLLECT ARGUMENTS
    int count = 0;
    char* brk = args[0];
    /*while((brk = strtok(args[i], ' ')) != NULL)   //get pointer to first WHITESPACE ' '
    {
      if((brk + 1)* == '\0')
        break;

    }*/
    while(brk != '\0' && count < MAX_ARGUMENTS - 1)      //cycle through the entire argument string
    {
      if(*brk == ' ')       //at every whitespace...
      {
        *brk = '\0';        //insert a string-termination character
        args[++count] = brk + 1; //add next character as beginning of next argument
      }
      brk++;  //increment pointer
    }
    count++;  //reverse 0-indexing offset

    //*DEBUG
    printf("\n---Arguments:\n");
    for(int i = 0; i < count; i++)
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
            result = entry->handler(count, args); // Call function
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
    else
    {
        char payload[8] = "Failure";
        return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
    }
}


// ===== END OF WORK AREA ===== //

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    COAP_WELL_KNOWN_CORE_DEFAULT_HANDLER,
    { "/echo/", COAP_GET | COAP_MATCH_SUBTREE, _echo_handler, NULL },
    { "/ifconfig/", COAP_PUT | COAP_MATCH_SUBTREE, _ifconfig_handler, NULL},    //mine
    { "/riot/board", COAP_GET, _riot_board_handler, NULL },
    { "/riot/value", COAP_GET | COAP_PUT | COAP_POST, _riot_value_handler, NULL },
    { "/riot/ver", COAP_GET, _riot_block2_handler, NULL },
    { "/shell/", COAP_PUT | COAP_GET | COAP_MATCH_SUBTREE, _shell_handler, NULL},    //mine
    //{ "/sha256", COAP_POST, _sha256_handler, NULL },
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
