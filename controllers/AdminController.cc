#include "AdminController.h"
#include <vector>
#include <map>

void AdminController::dashboard(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!req->session()->find("user_id") || req->session()->get<std::string>("user_role") != "admin") {
        callback(HttpResponse::newRedirectionResponse("/api/login"));
        return;
    }
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT id, title, price, stock FROM products ORDER BY id DESC",
        [callback](const drogon::orm::Result& r) {
            HttpViewData data;

            std::vector<std::map<std::string, std::string>> products_list;
            for (const auto& row : r) {
                std::map<std::string, std::string> product;
                product["id"] = row["id"].as<std::string>();
                product["title"] = row["title"].as<std::string>();
                product["price"] = row["price"].as<std::string>();
                product["stock"] = row["stock"].as<std::string>();
                products_list.push_back(std::move(product));
            }

            data.insert("products_list", std::move(products_list));

            auto resp = HttpResponse::newHttpViewResponse("AdminDashboard", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        }
    );
}

void AdminController::deleteProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    if (!req->session()->find("user_id") || req->session()->get<std::string>("user_role") != "admin") {
        callback(HttpResponse::newRedirectionResponse("/api/login"));
        return;
    }
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "DELETE FROM products WHERE id = $1",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newNotFoundResponse());
        },
        id
    );
}