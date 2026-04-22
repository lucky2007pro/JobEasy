#include "ReviewController.h"
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>

namespace
{
bool validateCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->getSession();
    if (!session->find("csrf_token"))
    {
        return false;
    }
    auto token = req->getParameter("csrf_token");
    if (token.empty())
    {
        return false;
    }
    return token == session->get<std::string>("csrf_token");
}
}  // namespace

void ReviewController::addReview(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto session = req->session();
    if (!session->find("user_id")) {
        auto resp = HttpResponse::newRedirectionResponse("/login");
        callback(resp);
        return;
    }
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    
    int user_id = session->get<int>("user_id");
    auto product_id_str = req->getParameter("product_id");
    auto rating_str = req->getParameter("rating");
    auto comment = req->getParameter("comment");

    if(product_id_str.empty() || rating_str.empty()) {
        auto resp = HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }

    try {
        int product_id = std::stoi(product_id_str);
        int rating = std::stoi(rating_str);
        
        auto db = app().getDbClient();
        db->execSqlAsync("INSERT INTO reviews (product_id, user_id, rating, comment) VALUES ($1, $2, $3, $4)",
            [callback, product_id_str](const drogon::orm::Result& r) {
                auto resp = HttpResponse::newRedirectionResponse("/product/" + product_id_str);
                callback(resp);
            },
            [callback, product_id_str](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << e.base().what();
                auto resp = HttpResponse::newRedirectionResponse("/product/" + product_id_str);
                callback(resp);
            },
            product_id, user_id, rating, comment
        );
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newRedirectionResponse("/product/" + product_id_str);
        callback(resp);
    }
}
