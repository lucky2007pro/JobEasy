#ifdef _MSC_VER
#pragma warning(disable : 26819)
#endif
#include "AdminController.h"
#include <vector>
#include <map>
#include <iomanip>
#include <drogon/utils/Utilities.h>

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
    const auto token = req->getParameter("csrf_token");
    if (token.empty())
    {
        return false;
    }
    return token == session->get<std::string>("csrf_token");
}
}

void AdminController::dashboard(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT p.id, p.title, p.price, p.stock, pi.image_url "
        "FROM products p "
        "LEFT JOIN product_images pi ON (p.id = pi.product_id AND pi.is_primary = TRUE) "
        "ORDER BY p.id DESC",
        [callback, req](const drogon::orm::Result& r) {
            HttpViewData data;
            std::vector<std::map<std::string, std::string>> products_list;
            for (const auto& row : r) {
                std::map<std::string, std::string> product;
                product["id"] = row["id"].as<std::string>();
                product["title"] = row["title"].as<std::string>();
                product["price"] = row["price"].as<std::string>();
                product["stock"] = row["stock"].as<std::string>();
                product["image_url"] = row["image_url"].isNull() ? "" : row["image_url"].as<std::string>();
                products_list.push_back(std::move(product));
            }
            data.insert("products_list", std::move(products_list));
            data.insert("is_logged_in", true);
            data.insert("user_name", "Admin");
            data.insert("csrf_token", ensureCsrfToken(req));

            auto resp = HttpResponse::newHttpViewResponse("AdminDashboard", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        }
    );
}

void AdminController::analytics(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = app().getDbClient();
    
    // Revenue va jami buyurtmalar
    dbClient->execSqlAsync(
        "SELECT COUNT(*) as total_orders, SUM(final_total) as total_revenue FROM orders",
        [callback, dbClient](const drogon::orm::Result& r1) {
            Json::Value stats;
            stats["total_orders"] = r1[0]["total_orders"].as<std::string>();
            stats["total_revenue"] = r1[0]["total_revenue"].isNull() ? "0" : r1[0]["total_revenue"].as<std::string>();

            // Top sotilgan mahsulotlar
            dbClient->execSqlAsync(
                "SELECT p.title, SUM(oi.quantity) as sold FROM order_items oi "
                "JOIN products p ON oi.product_id = p.id "
                "GROUP BY p.id, p.title ORDER BY sold DESC LIMIT 5",
                [callback, dbClient, stats](const drogon::orm::Result& r2) mutable {
                    std::vector<std::map<std::string, std::string>> top_sellers;
                    for(const auto& row : r2) {
                        std::map<std::string, std::string> item;
                        item["title"] = row["title"].as<std::string>();
                        item["sold"] = row["sold"].as<std::string>();
                        top_sellers.push_back(item);
                    }

                    // Oxirgi buyurtmalar
                    dbClient->execSqlAsync(
                        "SELECT o.id, u.full_name, o.final_total, o.status, o.created_at FROM orders o "
                        "JOIN users u ON o.user_id = u.id ORDER BY o.created_at DESC LIMIT 10",
                        [callback, stats, top_sellers](const drogon::orm::Result& r3) mutable {
                            HttpViewData data;
                            data.insert("stats", stats);
                            data.insert("top_sellers", top_sellers);
                            
                            std::vector<std::map<std::string, std::string>> recent_orders;
                            for(const auto& row : r3) {
                                std::map<std::string, std::string> order;
                                order["id"] = row["id"].as<std::string>();
                                order["full_name"] = row["full_name"].as<std::string>();
                                order["total"] = row["final_total"].isNull() ? "0" : row["final_total"].as<std::string>();
                                order["status"] = row["status"].as<std::string>();
                                order["date"] = row["created_at"].as<std::string>();
                                recent_orders.push_back(order);
                            }
                            data.insert("recent_orders", recent_orders);
                            data.insert("user_name", "Admin");

                            auto resp = HttpResponse::newHttpViewResponse("AdminAnalytics", data);
                            callback(resp);
                        },
                        [callback](const drogon::orm::DrogonDbException& e){
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        }
                    );
                },
                [callback](const drogon::orm::DrogonDbException& e){
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        }
            );
        },
        [callback](const drogon::orm::DrogonDbException& e){
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        }
    );
}

void AdminController::deleteProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "DELETE FROM products WHERE id = $1",
        [callback, req](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newNotFoundResponse());
        },
        id
    );
}

void AdminController::updateStock(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto productId = req->getParameter("product_id");
    auto stockValue = req->getParameter("stock");

    if (productId.empty() || stockValue.empty()) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Missing arguments")));
        return;
    }

    auto dbClient = app().getDbClient();
    dbClient->execSqlAsync(
        "UPDATE products SET stock = $1 WHERE id = $2",
        [callback, req](const drogon::orm::Result& r) {
            Json::Value ret;
            ret["status"] = "success";
            callback(HttpResponse::newHttpJsonResponse(ret));
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        std::stoi(stockValue), std::stoi(productId)
    );
}

void AdminController::allOrders(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT o.id, u.full_name, o.final_total, o.status, o.created_at, o.shipping_address FROM orders o "
        "JOIN users u ON o.user_id = u.id ORDER BY o.created_at DESC",
        [callback, req](const drogon::orm::Result& r) {
            HttpViewData data;
            std::vector<std::map<std::string, std::string>> orders;
            for (const auto& row : r) {
                std::map<std::string, std::string> order;
                order["id"] = row["id"].as<std::string>();
                order["full_name"] = row["full_name"].as<std::string>();
                order["total"] = row["final_total"].isNull() ? "0" : row["final_total"].as<std::string>();
                order["total_price"] = row["final_total"].isNull() ? "0" : row["final_total"].as<std::string>();
                order["status"] = row["status"].as<std::string>();
                order["date"] = row["created_at"].as<std::string>();
                order["shipping_address"] = row["shipping_address"].isNull() ? "" : row["shipping_address"].as<std::string>();
                order["created_at"] = row["created_at"].as<std::string>();
                orders.push_back(order);
            }
            data.insert("orders", orders);
            data.insert("orders_list", orders);
            data.insert("user_name", "Admin");
            auto resp = HttpResponse::newHttpViewResponse("AdminOrders", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e){ 
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()))); 
        }
    );
}

void AdminController::categories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT * FROM categories ORDER BY name ASC",
        [callback, req](const drogon::orm::Result& r) {
            HttpViewData data;
            std::vector<std::map<std::string, std::string>> categories_list;
            for (const auto& row : r) {
                std::map<std::string, std::string> cat;
                cat["id"] = row["id"].as<std::string>();
                cat["name"] = row["name"].as<std::string>();
                cat["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
                categories_list.push_back(cat);
            }
            data.insert("categories_list", categories_list);
            data.insert("user_name", "Admin");
            data.insert("csrf_token", ensureCsrfToken(req));
            auto resp = HttpResponse::newHttpViewResponse("AdminCategories", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e){ 
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()))); 
        }
    );
}

void AdminController::addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto name = req->getParameter("name");
    auto desc = req->getParameter("description");
    
    if(name.empty()) {
        callback(HttpResponse::newRedirectionResponse("/admin/categories"));
        return;
    }

    auto dbClient = app().getDbClient();
    dbClient->execSqlAsync(
        "INSERT INTO categories (name, description) VALUES ($1, $2)",
        [callback](const drogon::orm::Result& r) {
            callback(HttpResponse::newRedirectionResponse("/admin/categories"));
        },
        [callback](const drogon::orm::DrogonDbException& e){ 
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()))); 
        },
        name, desc
    );
}

void AdminController::deleteCategoryAction(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = app().getDbClient();
    dbClient->execSqlAsync(
        "DELETE FROM categories WHERE id = $1",
        [callback](const drogon::orm::Result& r) {
            callback(HttpResponse::newRedirectionResponse("/admin/categories"));
        },
        [callback](const drogon::orm::DrogonDbException& e){ 
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()))); 
        },
        id
    );
}
