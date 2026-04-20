#include "ProductController.h"
#include <drogon/HttpViewData.h>
#include <vector>
#include <map>

void ProductController::showProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");
    
    std::string userName = "Mehmon";
    bool isLoggedIn = false;
    if (req->session()->find("user_name")) {
        userName = req->session()->get<std::string>("user_name");
        isLoggedIn = true;
    }

    dbClient->execSqlAsync(
        "SELECT id, title, description, price, image_url, stock FROM products WHERE id = $1",
        [callback, userName, isLoggedIn](const drogon::orm::Result& r) {
            if (r.empty()) {
                callback(HttpResponse::newNotFoundResponse());
                return;
            }
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", isLoggedIn);
            
            std::map<std::string, std::string> product;
            product["id"] = r[0]["id"].as<std::string>();
            product["title"] = r[0]["title"].as<std::string>();
            product["description"] = r[0]["description"].as<std::string>();
            product["price"] = r[0]["price"].as<std::string>();
            product["image_url"] = r[0]["image_url"].as<std::string>();
            product["stock"] = r[0]["stock"].as<std::string>();
            
            data.insert("product", product);
            auto resp = HttpResponse::newHttpViewResponse("ProductDetail", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        },
        id
    );
}

void ProductController::addProductForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto resp = HttpResponse::newHttpViewResponse("AddProduct");
    callback(resp);
}

void ProductController::createProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto params = req->getParameters();
    
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "INSERT INTO products (title, description, price, image_url, stock) VALUES ($1, $2, $3, $4, $5)",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        },
        params["title"],
        params["description"],
        params["price"],
        params["image_url"],
        std::stoi(params["stock"])
    );
}
