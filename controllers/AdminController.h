#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AdminController : public drogon::HttpController<AdminController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AdminController::dashboard, "/admin/dashboard", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::deleteProduct, "/admin/delete/{1}", Get, "AdminFilter");
    METHOD_LIST_END

    void dashboard(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void deleteProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
};