#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class CategoryController : public drogon::HttpController<CategoryController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CategoryController::addCategory, "/admin/category/add", Post);
    METHOD_LIST_END

    void addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
