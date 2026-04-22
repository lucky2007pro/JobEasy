#pragma once
#include <drogon/HttpController.h>
#include "filters/AdminFilter.h"

using namespace drogon;

class ProductController : public drogon::HttpController<ProductController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ProductController::showProduct, "/product/{id}", Get);
        ADD_METHOD_TO(ProductController::addProductForm, "/admin/product/add", Get, "AdminFilter");
        ADD_METHOD_TO(ProductController::createProduct, "/admin/product/add", Post, "AdminFilter");
        ADD_METHOD_TO(ProductController::editProductForm, "/admin/product/edit/{id}", Get, "AdminFilter");
        ADD_METHOD_TO(ProductController::updateProduct, "/admin/product/edit/{id}", Post, "AdminFilter");
        ADD_METHOD_TO(ProductController::deleteImage, "/admin/product/image/delete/{id}", Get, "AdminFilter");
        ADD_METHOD_TO(ProductController::setPrimaryImage, "/admin/product/image/primary/{id}", Get, "AdminFilter");
    METHOD_LIST_END

    void showProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void addProductForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
    void createProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
    void editProductForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void updateProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void deleteImage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void setPrimaryImage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id);
};
