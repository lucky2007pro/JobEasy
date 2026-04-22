#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class AdminFilter : public drogon::HttpFilter<AdminFilter>
{
public:
    virtual void doFilter(const HttpRequestPtr& req, FilterCallback&& fcb, FilterChainCallback&& fccb) override
    {
        auto session = req->session();
        if (session->find("user_id") && session->get<std::string>("user_role") == "admin")
        {
            fccb();
            return;
        }
        auto resp = HttpResponse::newRedirectionResponse("/login");
        fcb(resp);
    }
};