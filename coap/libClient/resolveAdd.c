//resolveAdd.c
//code copied form github.com/obgm/libcoap-minimal/blob/master/common.cc

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <coap2/coap.h>

int resolve_address(const char* host, const char* service, coap_address_t* dst)
{
	struct addrinfo *res, *ainfo, hints;
	int error, len = -1;
	
	memset(&hints, 0, sizeof(hints));
	memset(dst, 0, sizeof(*dst));	//WHAT? isn't this the same as dst = NULL?
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_UNSPEC;
	
	error = getaddrinfo(host, service, &hints, &res);
	
	if(error != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		return error;
	}
	
	for(ainfo = res; ainfo != NULL;  ainfo = ainfo->ai_next)
		switch (ainfo->ai_family)
		{
			case AF_INET6:
			case AF_INET:
				len = dst->size = ainfo->ai_addrlen;
				memcpy(&dst->addr.sin6, ainfo->ai_addr, dst->size);
				goto finish;
			default:
			;
		}
finish:
	freeaddrinfo(res);
	return len;
}