#include "CartController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <map>

namespace
{
std::string columnToStringSafe(const drogon::orm::Row &row,
                               const std::string &columnName,
                               const std::string &fallback = "")
{
    try
    {
        if (row[columnName].isNull())
        {
            return fallback;
        }
        return row[columnName].as<std::string>();
    }
    catch (const std::exception &)
    {
        return fallback;
    }
}

bool parseIntSafe(const std::string &value, int &out)
{
    try
    {
        out = std::stoi(value);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool parseDoubleSafe(const std::string &value, double &out)
{
    try
    {
        out = std::stod(value);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

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

void CartController::viewCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!req->session()->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    auto userId = req->session()->get<int>("user_id");
    auto userName = req->session()->get<std::string>("user_name");
    
    auto dbClient = drogon::app().getDbClient("default");
    const auto csrfToken = ensureCsrfToken(req);

    dbClient->execSqlAsync(
        "SELECT c.id as cart_id, p.id as product_id, p.title, p.price, pi.image_url, c.quantity "
        "FROM cart_items c JOIN products p ON c.product_id = p.id "
        "LEFT JOIN product_images pi ON (p.id = pi.product_id AND pi.is_primary = TRUE) "
        "WHERE c.user_id = $1",
        [callback, userName, csrfToken](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", true);
            data.insert("csrf_token", csrfToken);
            
            std::vector<std::map<std::string, std::string>> items;
            double total = 0.0;
            
            for (const auto& row : r) {
                std::map<std::string, std::string> item;
                item["cart_id"] = columnToStringSafe(row, "cart_id", "");
                item["product_id"] = columnToStringSafe(row, "product_id", "");
                item["title"] = columnToStringSafe(row, "title", "");
                item["price"] = columnToStringSafe(row, "price", "0");
                item["image_url"] = columnToStringSafe(row, "image_url", "");
                item["quantity"] = columnToStringSafe(row, "quantity", "1");
                
                double price = 0.0;
                int qty = 1;
                if (!parseDoubleSafe(item["price"], price))
                {
                    price = 0.0;
                }
                if (!parseIntSafe(item["quantity"], qty) || qty < 1)
                {
                    qty = 1;
                }
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
    if (!req->session()->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto userId = req->session()->get<int>("user_id");
    auto params = req->getParameters();
    int productId = 0;
    if (!parseIntSafe(params["product_id"], productId) || productId <= 0)
    {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: product_id noto'g'ri.")));
        return;
    }
    int quantity = 1;
    if (params.find("quantity") != params.end()) {
        if (!parseIntSafe(params["quantity"], quantity) || quantity <= 0)
        {
            quantity = 1;
        }
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
    if (!req->session()->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
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
