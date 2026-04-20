#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ProductController : public drogon::HttpController<ProductController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ProductController::showProduct, "/product/{1}", Get);
        ADD_METHOD_TO(ProductController::addProductForm, "/admin/product/add", Get, "AdminFilter");
        ADD_METHOD_TO(ProductController::createProduct, "/admin/product/add", Post, "AdminFilter");
    METHOD_LIST_END

    void showProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void addProductForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void createProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
