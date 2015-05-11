#include "api.h"
#include "../util.h"

size_t
api_nint(enum api_property property)
{
	switch (property) {
	case API_PROPERTY_URGENT_WORKSPACE:
	case API_PROPERTY_URGENT_DESKTOP:
		return 0;
	case API_PROPERTY_MONITOR:
	case API_PROPERTY_WORKSPACE:
	case API_PROPERTY_DESKTOP:
	case API_PROPERTY_LAYOUT:
		return 1;
	case API_PROPERTY_NMASTER:
	case API_PROPERTY_SUBSCRIPTION:
		return 2;
	default:
		ERROR("Unknown property: %d", property);
		return 0;
	}
}

size_t
api_nstr(enum api_property property)
{
	switch (property) {
	case API_PROPERTY_MONITOR:
	case API_PROPERTY_NMASTER:
	case API_PROPERTY_SUBSCRIPTION:
		return 0;
	case API_PROPERTY_WORKSPACE:
	case API_PROPERTY_DESKTOP:
	case API_PROPERTY_URGENT_WORKSPACE:
	case API_PROPERTY_URGENT_DESKTOP:
		return 1;
	default:
		ERROR("Unknown property: %d", property);
		return 0;
	}
}
