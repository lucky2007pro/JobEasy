#include "ReviewController.h"
#include <drogon/drogon.h>

void ReviewController::addReview(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto session = req->getSession();
    if (!session->find("user_id")) {
        auto resp = HttpResponse::newRedirectionResponse("/api/login");
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
        db->execSqlSync("INSERT INTO reviews (product_id, user_id, rating, comment) VALUES ($1, $2, $3, $4)",
                        product_id, user_id, rating, comment);
                        
        auto resp = HttpResponse::newRedirectionResponse("/product/" + product_id_str);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newRedirectionResponse("/product/" + product_id_str);
        callback(resp);
    }
}
