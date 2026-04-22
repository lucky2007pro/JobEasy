#include <drogon/HttpFilter.h>
#include "LoginFilter.h"

static drogon::FilterRegistrar<::LoginFilter> loginFilterRegistrar("LoginFilter");
