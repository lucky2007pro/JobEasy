#include "HomeController.h"
#include <drogon/HttpViewData.h>
#include <vector>
#include <map>

void HomeController::index(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto dbClient = drogon::app().getDbClient("default");

    std::string userName = "Mehmon";
    bool isLoggedIn = false;
    if (req->session()->find("user_name")) {
        userName = req->session()->get<std::string>("user_name");
        isLoggedIn = true;
    }

    dbClient->execSqlAsync(
        "SELECT id, title, price, image_url FROM products ORDER BY id DESC",
        [callback, userName, isLoggedIn](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", isLoggedIn);
            
            std::vector<std::map<std::string, std::string>> products;
            for (const auto& row : r) {
                std::map<std::string, std::string> product;
                product["id"] = row["id"].as<std::string>();
                product["title"] = row["title"].as<std::string>();
                product["price"] = row["price"].as<std::string>();
                product["image_url"] = row["image_url"].as<std::string>();
                products.push_back(product);
            }
            data.insert("products", products);
            
            auto resp = HttpResponse::newHttpViewResponse("index.csp", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        }
    );
}
