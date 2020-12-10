#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net/netif.h"
#include "net/gnrc/netif.h"

#include "net/nanocoap.h"

//extern netif_t;
//extern netif_t *netif_get_by_name(const char *name);

typedef struct
{
	const char* name;		// String to be matched with request
	const netopt_t opt;		// Corresponding operation macro (see: https://riot-os.org/api/group__net__netopt.html#ga19e30424c1ab107c9c84dc0cb29d9906)
	const short size;		// Size in bytes of the requested data 1->8bits, 2->16bits, 3->32bits etc.
}netopt_unit;

const netopt_unit netopt_units[] = {
	{"channel", NETOPT_CHANNEL, 2},
	{"hop_limit", NETOPT_HOP_LIMIT, 1}
};
const unsigned short netopt_units_numof = ARRAY_SIZE(netopt_units);

int readArgs (char* source, int sourceLen, char** args, int maxArgs)
{
	if (maxArgs < 1 || sourceLen < 1) return -1;

	char* brk = source;
	args[0] = source;
	int i = 0, count = 1;
	while(i++ < sourceLen && count < maxArgs)      //cycle through the entire argument string
    {
		//printf("arg[%i]:\t%c\t(%i)\n", i, *brk, (int)*brk); //DEBUG
		if(*brk == ' ' || *brk == '/' || *brk == '\\')       //at every whitespace OR (back)slash
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

	    //*DEBUG
	    printf("\n---Arguments:\n");
	    for(i = 0; i < count; i++)
	      {printf("\targs[%i]:\t%s\n", i, args[i]);}
	    printf("\n");
	    //DEBUG*/

    return count;
}

static ssize_t netGetter (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    const int maxArgs = 2;
    //PARSE URI
    char* args[maxArgs];
    char source[CONFIG_NANOCOAP_URI_MAX];
	int sourceLen = coap_get_uri_path(pkt, (uint8_t *)source) - strlen("/netif/");
    //args[0] = source + strlen("/shell/");
    (void) context;
    //COLLECT ARGUMENTS
	int argCount = readArgs(source + strlen("/netif/"), sourceLen, args, maxArgs);
	if(argCount == -1)	//if error reading arguments (returns)
	{
		printf("\n");
	        char payload[] = "Failure:\t\tError reading arguments";
	        printf("%s\n", payload);
	        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}

    //HANDLE SECOND ARGUMENT /netif/X/...
    netif_t* iface = netif_get_by_name(args[0]);
	if (!iface)
	{
        char payload[] = "Failure: Invalid interface";
		printf("%s\n", payload);
        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}

    //HANDLE THRID ARGUMENT /netif/X/Y
	for(unsigned int i = 0; i < netopt_units_numof; i++)
		if(strcmp(args[1], netopt_units[i].name) == 0)	//if name matches
		{
				//WERK WERK
				printf("Hi, you requested to see the \"%s\" resource, of interface \"%s\".\nReading you load and clear!", args[1], args[0]);
			return 0;
		}
	//if(i == netopt_units_numof)	//if no argument matched (entire loop was run)
	return 0;
}

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    {"/netif/", COAP_GET | COAP_MATCH_SUBTREE, netGetter, NULL},
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
