#include "CartController.h"
#include <drogon/HttpViewData.h>
#include <vector>
#include <map>

void CartController::viewCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = req->session()->get<int>("user_id");
    auto userName = req->session()->get<std::string>("user_name");
    
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT c.id as cart_id, p.id as product_id, p.title, p.price, p.image_url, c.quantity "
        "FROM cart_items c JOIN products p ON c.product_id = p.id "
        "WHERE c.user_id = $1",
        [callback, userName](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            
            std::vector<std::map<std::string, std::string>> items;
            double total = 0.0;
            
            for (const auto& row : r) {
                std::map<std::string, std::string> item;
                item["cart_id"] = row["cart_id"].as<std::string>();
                item["product_id"] = row["product_id"].as<std::string>();
                item["title"] = row["title"].as<std::string>();
                item["price"] = row["price"].as<std::string>();
                item["image_url"] = row["image_url"].as<std::string>();
                item["quantity"] = row["quantity"].as<std::string>();
                
                double price = std::stod(item["price"]);
                int qty = std::stoi(item["quantity"]);
                total += (price * qty);
                item["subtotal"] = std::to_string(price * qty);
                
                items.push_back(item);
            }
            
            data.insert("cart_items", items);
            data.insert("total_price", std::to_string(total));
            
            auto resp = HttpResponse::newHttpViewResponse("Cart", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId
    );
}

void CartController::addToCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = req->session()->get<int>("user_id");
    auto params = req->getParameters();
    int productId = std::stoi(params["product_id"]);
    int quantity = 1;
    if (params.find("quantity") != params.end()) {
        quantity = std::stoi(params["quantity"]);
    }
    
    auto dbClient = drogon::app().getDbClient("default");
    
    dbClient->execSqlAsync(
        "INSERT INTO cart_items (user_id, product_id, quantity) VALUES ($1, $2, $3) "
        "ON CONFLICT(user_id, product_id) DO UPDATE SET quantity = cart_items.quantity + $3",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/cart");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId, productId, quantity
    );
}

void CartController::removeFromCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int cartId) {
    auto userId = req->session()->get<int>("user_id");
    
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "DELETE FROM cart_items WHERE id = $1 AND user_id = $2",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/cart");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        cartId, userId
    );
}
