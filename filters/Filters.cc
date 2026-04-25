#include <drogon/drogon.h>
#include "LoginFilter.h"
#include "AdminFilter.h"

using namespace drogon;

namespace {
    // Explicitly instantiate to force registration with the framework
    static const auto _admin_filter = AdminFilter();
    static const auto _login_filter = LoginFilter();
}

// Drogon 1.9+ might require explicit instantiation if filters are header-only
