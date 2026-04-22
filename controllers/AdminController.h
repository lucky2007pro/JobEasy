#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AdminController : public drogon::HttpController<AdminController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AdminController::dashboard, "/admin/dashboard", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::analytics, "/admin/analytics", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::deleteProduct, "/admin/product/delete/{id}", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::updateStock, "/admin/product/update-stock", Post, "AdminFilter");
        ADD_METHOD_TO(AdminController::allOrders, "/admin/orders", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::categories, "/admin/categories", Get, "AdminFilter");
        ADD_METHOD_TO(AdminController::addCategory, "/admin/category/add", Post, "AdminFilter");
        ADD_METHOD_TO(AdminController::deleteCategoryAction, "/admin/category/delete/{id}", Get, "AdminFilter");
    METHOD_LIST_END

    void dashboard(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void analytics(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void deleteProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void updateStock(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void allOrders(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void categories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void deleteCategoryAction(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
};
