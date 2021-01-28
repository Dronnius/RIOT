#define NETOPT_UNITS_MAX 10

typedef struct
{
	const char* name;		// String to be matched with request
	const netopt_t op;		// Corresponding operation macro (see: https://riot-os.org/api/group__net__netopt.html#ga19e30424c1ab107c9c84dc0cb29d9906)
	const uint32_t size;	// Size in bytes of the requested data 1->8bits, 2->16bits, 3->32bits etc.
	//const if_setget setget;
}netopt_unit;

void start_coap_server(const netopt_unit* units, int count);
