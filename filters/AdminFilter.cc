#include <drogon/HttpFilter.h>
#include "AdminFilter.h"

static drogon::FilterRegistrar<::AdminFilter> adminFilterRegistrar("AdminFilter");
