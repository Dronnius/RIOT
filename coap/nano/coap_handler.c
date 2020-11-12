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

#include "fmt.h"
#include "net/nanocoap.h"
#include "hashes/sha256.h"

#define ENABLE_DEBUG (1)

/* internal value that can be read/written via CoAP */
static uint8_t internal_value = 0;

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

ssize_t _sha256_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;

    /* using a shared sha256 context *will* break if two requests are handled
     * at the same time.  doing it anyways, as this is meant to showcase block1
     * support, not proper synchronisation. */
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

// ===== WORK IN PROGRESS ===== //

//TODO:
// parse arguments          X
// move to thread           X
// execute with arguments   X
// NEW - remove threading   O
// pipe processes (I/O)
// return result
//
//NOTES:
// maybe multithreading with limited memory isn't a good idea?
// what happens to the caller's stack when the handler returns?
// nothing good, probably, freezing the handler after spawning the caller fixed the 9 character agument issue
extern int _gnrc_netif_config(int argc, char **argv);

void* caller(void* arg) //arg is cut to 9 characters (inlucding terminating null)!!! WHY???
{
    printf("Starting process #%i\t(%s)\n\n", thread_getpid(), thread_getname(thread_getpid()));

    /*char meep[1024];
    meep[1023] = 'k';
    (void)meep;*/
    /*int k = 0;
    while(((char*)arg)[k] != '\0')
        ((char*)arg)[k++] = '!';            //DEBUG "!" */
    printf("incoming argument: \"%s\"\n", (char*)arg);      //DEBUG

    //translate arguments to desired form
    char* temp;
    char* args[10];
    char command[9] = "ifconfig";
    args[0] = command; //set first argument
    args[1] = arg;  //set second argument
    int i = 1;
        printf("DEBUG A\n");
    while((temp = strchr((char*)args[i++], (int)' ')) != NULL && i < 10) //loop until last occurence
    {
        printf("LOOP\n");
        //printf("\tAssigning \"%c\"'s position to args[%i]...", *temp, i);
        args[i] = temp;
        //printf("\t")
        *args[i] = '\0';    //exchange spaces with NULL characters
        args[i]++; //shift to next character (instead of the space)
        //i++;    //increment index
    }
        printf("DEBUG B\n");
    if(i >= 10 && strchr((char*)args[9], (int)' ') != NULL)
    { printf("ERROR: too many arguments for command handler"); return NULL;}

    //print arguments (debugging reasons)
    for(int j = 0; j < i; j++)
        printf("%s\n", args[j]);

    //temp = &args;
        printf("DEBUG C\n");
    _gnrc_netif_config(i, args);

    printf("Killing process #%i\t(%s)\n\n", thread_getpid(), thread_getname(thread_getpid()));
    return NULL;
}

static ssize_t _ifconfig_handler (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    // parse argument
    char uri[CONFIG_NANOCOAP_URI_MAX];
    int uri_len;
    uri_len = coap_get_uri_path(pkt, (uint8_t *)uri);
        printf("received uri: \"%s\"\n", uri);       //DEBUG
    char* arg = uri + strlen("/ifconfig/");
    //int newVal = atoi(arg);

    //printf("DEBUG\treceived argument: %i\n", newVal);
    (void)uri_len;
    (void)context;

    printf("sending argument: \"%s\"\n", arg);

    pid_t child;
    char child_stack[THREAD_STACKSIZE_SMALL + 2];   //try increasing by 1 (+1 on stack and size below)
    child = thread_create(child_stack, THREAD_STACKSIZE_SMALL + 2, THREAD_PRIORITY_MAIN - 1,
                          THREAD_CREATE_STACKTEST, caller, arg, "ifconfig agent");

    (void)child;

    while(1)
        thread_yield();

    return coap_reply_simple(pkt, COAP_CODE_204, buf, len, COAP_FORMAT_TEXT, NULL, 0);
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
    { "/sha256", COAP_POST, _sha256_handler, NULL },
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
