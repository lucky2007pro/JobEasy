#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class LoginFilter : public HttpFilter<LoginFilter>
{
public:
    virtual void doFilter(const HttpRequestPtr& req,
        FilterCallback&& fcb,
        FilterChainCallback&& fccb) override
    {
        // 1. Session'da user_id bormi?
        if (req->session()->find("user_id"))
        {
            // Bor bo'lsa, yo'lida davom etsin
            fccb();
            return;
        }

        // 2. Yo'q bo'lsa, Login sahifasiga haydaymiz
        auto resp = HttpResponse::newRedirectionResponse("/login");
        fcb(resp);
    }
};