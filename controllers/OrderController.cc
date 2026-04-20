#include "OrderController.h"
#include <drogon/HttpViewData.h>
#include <vector>
#include <map>

void OrderController::checkout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = req->session()->get<int>("user_id");
    auto dbClient = drogon::app().getDbClient("default");
    
    auto txn = dbClient->newTransaction();
    
    txn->execSqlAsync(
        "SELECT p.id, p.price, c.quantity FROM cart_items c JOIN products p ON c.product_id = p.id WHERE c.user_id = $1",
        [callback, txn, userId](const drogon::orm::Result& r) {
            if (r.empty()) {
                auto resp = HttpResponse::newRedirectionResponse("/cart");
                callback(resp);
                return;
            }
            
            double total = 0.0;
            for (const auto& row : r) {
                total += row["price"].as<double>() * row["quantity"].as<int>();
            }
            
            txn->execSqlAsync(
                "INSERT INTO orders (user_id, total_price) VALUES ($1, $2) RETURNING id",
                [callback, txn, userId, r](const drogon::orm::Result& order_res) {
                    int orderId = order_res[0]["id"].as<int>();
                    
                    for (const auto& row : r) {
                        txn->execSqlAsync(
                            "INSERT INTO order_items (order_id, product_id, quantity, price) VALUES ($1, $2, $3, $4)",
                            [](const drogon::orm::Result&){},
                            [](const drogon::orm::DrogonDbException&){},
                            orderId, row["id"].as<int>(), row["quantity"].as<int>(), row["price"].as<double>()
                        );
                    }
                    
                    txn->execSqlAsync(
                        "DELETE FROM cart_items WHERE user_id = $1",
                        [callback](const drogon::orm::Result&) {
                            auto resp = HttpResponse::newRedirectionResponse("/orders");
                            callback(resp);
                        },
                        [callback](const drogon::orm::DrogonDbException& e) {
                            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                        },
                        userId
                    );
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                },
                userId, total
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId
    );
}

void OrderController::myOrders(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto userId = req->session()->get<int>("user_id");
    auto userName = req->session()->get<std::string>("user_name");
    
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT id, total_price, status, created_at FROM orders WHERE user_id = $1 ORDER BY created_at DESC",
        [callback, userName](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            
            std::vector<std::map<std::string, std::string>> orders;
            for (const auto& row : r) {
                std::map<std::string, std::string> order;
                order["id"] = row["id"].as<std::string>();
                order["total_price"] = row["total_price"].as<std::string>();
                order["status"] = row["status"].as<std::string>();
                order["created_at"] = row["created_at"].as<std::string>();
                orders.push_back(order);
            }
            
            data.insert("orders", orders);
            auto resp = HttpResponse::newHttpViewResponse("Orders", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId
    );
}
