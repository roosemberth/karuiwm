#ifndef _API
#define _API

#include <stdlib.h>

enum api_command {
	API_COMMAND_GET = 0x00,
	API_COMMAND_SET = 0x01,
};

enum api_property {
	API_PROPERTY_MONITOR          = 0x00,
	API_PROPERTY_WORKSPACE        = 0x01,
	API_PROPERTY_DESKTOP          = 0x02,
	API_PROPERTY_LAYOUT           = 0x03,
	API_PROPERTY_NMASTER          = 0x04,
	API_PROPERTY_URGENT_WORKSPACE = 0x05,
	API_PROPERTY_URGENT_DESKTOP   = 0x06,
	API_PROPERTY_SUBSCRIPTION     = 0x07,
};

enum api_subscription {
	API_SUBSCRIPTION_MONITOR      = 1<<0,
	API_SUBSCRIPTION_WORKSPACE    = 1<<1,
	API_SUBSCRIPTION_DESKTOP      = 1<<2,
	API_SUBSCRIPTION_LAYOUT       = 1<<3,
	API_SUBSCRIPTION_URGENT       = 1<<4,
};

size_t api_nint(enum api_property property);
size_t api_nstr(enum api_property property);

#endif /* _API */
