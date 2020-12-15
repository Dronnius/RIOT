#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net/netif.h"
#include "net/gnrc/netif.h"
#include "fmt.h"

#include "net/nanocoap.h"

//extern netif_t;
//extern netif_t *netif_get_by_name(const char *name);

//typedef uint32_t (*if_setget) (netif_t* iface, netopt_t op, char* val);	//val == NULL for get calls

typedef struct
{
	const char* name;		// String to be matched with request
	const netopt_t op;		// Corresponding operation macro (see: https://riot-os.org/api/group__net__netopt.html#ga19e30424c1ab107c9c84dc0cb29d9906)
	const uint32_t size;	// Size in bytes of the requested data 1->8bits, 2->16bits, 3->32bits etc.
	//const if_setget setget;
}netopt_unit;

//uint32_t u8_setget (netif_t* iface, netopt_t op, char* val);
//uint32_t u16_setget (netif_t* iface, netopt_t op, char* val);

const netopt_unit netopt_units[] = {
	{"channel", NETOPT_CHANNEL, 2},//u16_setget},
	{"frequency", NETOPT_CHANNEL_FREQUENCY, 4},
	{"hop_limit", NETOPT_HOP_LIMIT, 1}//u8_setget}
};
const unsigned short netopt_units_numof = ARRAY_SIZE(netopt_units);


/*uint32_t u8_setget (netif_t* iface, netopt_t op, char* val)
{
	if(val == NULL)	//if GET request
	{
		uint8_t result;
		_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, op, 0, (void*)&result, 1, GNRC_NETAPI_MSG_TYPE_GET);
		return result;
	}
	return 0;
}
uint32_t u16_setget (netif_t* iface, netopt_t op, char* val)
{
	if(val == NULL)	//if GET request
	{
		uint16_t result;
		_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, op, 0, (void*)&result, 1, GNRC_NETAPI_MSG_TYPE_GET);
		return result;
	}
	return 0;
}*/


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

    //HANDLE THIRD ARGUMENT /netif/X/Y
	for(unsigned int i = 0; i < netopt_units_numof; i++)	//loop through net defined options
		if(strcmp(args[1], netopt_units[i].name) == 0)	//if name matches
		{
			//printf("Hi, you requested to see the \"%s\" resource, of interface \"%s\".\nReading you load and clear!\n", args[1], args[0]);
			uint32_t result = 0;	//memory space reserved for answer
			//void* result = (void*)&space;//((char*)&space + (sizeof(space) - netopt_units[i].size));	//offset depending on size
			_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, netopt_units[i].op, 0, &result, netopt_units[i].size, GNRC_NETAPI_MSG_TYPE_GET);

			//DEBUG: print byte-values byte, by byte
			printf("===== ===== ===== ===== =====\nMEMORY IMAGE:\nbyte:\t\t\t\tvalue\n");
			for(uint8_t* m = (uint8_t*)&result; (uint8_t*)m < (uint8_t*)&result + sizeof(result); m++)
				printf("result + [%i]\t\t\t0x%x\n", m - (uint8_t*)&result, *m);
			printf("END OF IMAGE\n===== ===== ===== =====\n");
			//DEBUG END

			printf("Result retrieved\n");
			char payload[20];
			memcpy(payload, 0, sizeof(payload));
			sprintf(payload, "%u", result);
			//payload[19] = '\0';	//just in case
			printf("%s\n",payload);
	        return coap_reply_simple(pkt, coap_code(2, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, strlen(payload) + 1);	// +1 for '\0' character
		}
	//if(i == netopt_units_numof)	//if no argument matched (entire loop was run)
	char payload[] = "Failure: property not recongnised";
	printf("%s\n", payload);
	return coap_reply_simple(pkt, coap_code(4, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
}

static ssize_t netSetter (coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
	const int maxArgs = 3;
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

    //HANDLE SECOND ARGUMENT X: /netif/X/...
    netif_t* iface = netif_get_by_name(args[0]);
	if (!iface)
	{
        char payload[] = "Failure: Invalid interface";
		printf("%s\n", payload);
        return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}

    //HANDLE THIRD ARGUMENT Y: /netif/X/Y
	unsigned int i = 0;
	for(; i < netopt_units_numof; i++)	//loop through net defined options
		if(strcmp(args[1], netopt_units[i].name) == 0)	//if name matches
			{printf("property match\n"); break;}
	if(i == netopt_units_numof)	//if property is not matched
	{
		char payload[] = "Failure: property not recongnised";
		printf("%s\n", payload);
		return coap_reply_simple(pkt, coap_code(4, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}
	// At this point, netopt_units[i] should refer to the correct unit
	//HANDLE FOURTH ARGUMENT Z: /netif/X/Y/Z
	uint32_t value;
	if(args[2][0] == 'x' || args[2][1] == 'x')
		value = scn_u32_hex(args[2], strlen(args[2]));
	else value = scn_u32_dec(args[2], strlen(args[2]));

	//ERROR HERE: 5, IS SOMEHOW NOT CONSISDERED REPRESENTABLE BY 4 BYTES	(ALSO, SE IF YOU CAN RESET STACKSIZE IN MAIN.C)
	if(value >= 1ul << (8 * netopt_units[i].size))	//if value exceeds what the property's (byte) size could hold
	{
		char payload[50];
		sprintf(payload, "Failure: value too high, max %u bytes", netopt_units[i].size);
		printf("%s\n", payload);
		return coap_reply_simple(pkt, coap_code(4, 0), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}

	//Arguments OK, make the change!
	_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, netopt_units[i].op, 0, &value, netopt_units[i].size, GNRC_NETAPI_MSG_TYPE_SET);
	uint32_t new = 0;
	_gnrc_netapi_get_set(((gnrc_netif_t*)iface)->pid, netopt_units[i].op, 0, &new, netopt_units[i].size, GNRC_NETAPI_MSG_TYPE_GET);
	if(new != value)	//Failed to set value
	{
		char payload[] = "Failure: new value not accepted";
		printf("%s\n", payload);
		printf("\tReceived\t=\t%i\n\tNew\t\t=\t%i\n", value, new);
		return coap_reply_simple(pkt, coap_code(4, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, sizeof(payload));
	}


	char payload[] = "Success: new value set";
	printf("%s\n", payload);
	//payload[19] = '\0';	//just in case
	printf("\tReceived\t=\t%i\n\tNew\t\t=\t%i\n", value, new);
	return coap_reply_simple(pkt, coap_code(2, 4), buf, len, COAP_FORMAT_TEXT, (uint8_t*)payload, strlen(payload) + 1); //+1 for '\0' character
}

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    {"/netif/", COAP_GET | COAP_MATCH_SUBTREE, netGetter, NULL},
	{"/netif/", COAP_PUT | COAP_MATCH_SUBTREE, netSetter, NULL},
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
