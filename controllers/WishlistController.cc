#include "WishlistController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <map>

namespace
{
std::string ensureCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->session();
    if (!session->find("csrf_token"))
    {
        session->insert("csrf_token", drogon::utils::getUuid());
    }
    return session->get<std::string>("csrf_token");
}

bool validateCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->session();
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

void WishlistController::viewWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient("default");
    auto userId = req->session()->get<int>("user_id");
    auto userName = req->session()->get<std::string>("user_name");
    const auto csrfToken = ensureCsrfToken(req);

    dbClient->execSqlAsync(
        "SELECT p.id, p.title, p.price, pi.image_url FROM products p "
        "JOIN wishlist w ON p.id = w.product_id "
        "LEFT JOIN product_images pi ON (p.id = pi.product_id AND pi.is_primary = TRUE) "
        "WHERE w.user_id = $1",
        [callback, userName, csrfToken](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", true);
            data.insert("csrf_token", csrfToken);

            std::vector<std::map<std::string, std::string>> items;
            for (const auto& row : r) {
                std::map<std::string, std::string> item;
                item["id"] = row["id"].as<std::string>();
                item["title"] = row["title"].as<std::string>();
                item["price"] = row["price"].as<std::string>();
                item["image_url"] = row["image_url"].as<std::string>();
                items.push_back(item);
            }
            data.insert("wishlist_items", items);

            auto resp = HttpResponse::newHttpViewResponse("Wishlist", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        },
        userId
    );
}

void WishlistController::addToWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");
    auto userId = req->session()->get<int>("user_id");
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    dbClient->execSqlAsync(
        "INSERT INTO wishlist (user_id, product_id) VALUES ($1, $2) ON CONFLICT DO NOTHING",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/wishlist");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        },
        userId,
        id
    );
}

void WishlistController::removeFromWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");
    auto userId = req->session()->get<int>("user_id");
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    dbClient->execSqlAsync(
        "DELETE FROM wishlist WHERE user_id = $1 AND product_id = $2",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/wishlist");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        },
        userId,
        id
    );
}
