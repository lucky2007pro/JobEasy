#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class SearchController : public drogon::HttpController<SearchController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SearchController::search, "/search", Get);
    METHOD_LIST_END

    void search(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
