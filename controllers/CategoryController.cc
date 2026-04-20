#include "CategoryController.h"
#include <drogon/drogon.h>

void CategoryController::addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto name = req->getParameter("name");
    auto desc = req->getParameter("description");

    if(name.empty()) {
        auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
        callback(resp);
        return;
    }

    try {
        auto db = app().getDbClient();
        db->execSqlSync("INSERT INTO categories (name, description) VALUES ($1, $2)", name, desc);
        auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
