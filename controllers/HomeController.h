#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class HomeController : public drogon::HttpController<HomeController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HomeController::index, "/", Get);
    METHOD_LIST_END

    void index(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};
