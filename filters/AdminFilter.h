#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class AdminFilter : public HttpFilter<AdminFilter>
{
public:
    virtual void doFilter(const HttpRequestPtr& req, FilterCallback&& fcb, FilterChainCallback&& fccb) override
    {
        auto session = req->session();

        // 1. Tizimga kirganmi? 
        // 2. Role 'admin'mi?
        if (session->find("user_id") && session->get<std::string>("user_role") == "admin")
        {
            fccb(); // Hammasi joyida, o'taversin
            return;
        }

        // Aks holda login sahifasiga
        auto resp = HttpResponse::newRedirectionResponse("/login");
        fcb(resp);
    }
};