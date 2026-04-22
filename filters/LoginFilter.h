#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class LoginFilter : public drogon::HttpFilter<LoginFilter>
{
public:
    virtual void doFilter(const HttpRequestPtr& req, FilterCallback&& fcb, FilterChainCallback&& fccb) override
    {
        auto session = req->session();
        if (session->find("user_id"))
        {
            fccb();
            return;
        }
        auto resp = HttpResponse::newRedirectionResponse("/login");
        fcb(resp);
    }
};