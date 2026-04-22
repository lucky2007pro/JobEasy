#include <drogon/HttpFilter.h>
#include "LoginFilter.h"
#include "AdminFilter.h"

using namespace drogon;

// These macros are handled by the Drogon reflection system
REGISTER_FILTER(LoginFilter);
REGISTER_FILTER(AdminFilter);
