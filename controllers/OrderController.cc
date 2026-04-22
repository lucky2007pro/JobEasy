#include "OrderController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <map>
#include <cctype>

namespace
{
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

std::string ensureCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->session();
    if (!session->find("csrf_token"))
    {
        auto token = drogon::utils::getUuid();
        session->insert("csrf_token", token);
        return token;
    }
    return session->get<std::string>("csrf_token");
}

bool isValidPhone(const std::string &phone)
{
    int digits = 0;
    for (char c : phone)
    {
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            ++digits;
        }
        else if (c != '+' && c != ' ' && c != '-' && c != '(' && c != ')')
        {
            return false;
        }
    }
    return digits >= 9;
}

double calculateDeliveryFee(double subtotal, const std::string &deliveryType)
{
    if (subtotal >= 500000.0)
    {
        return 0.0;
    }
    if (deliveryType == "express")
    {
        return 35000.0;
    }
    return 18000.0;
}
}  // namespace

void OrderController::checkoutPage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto session = req->session();
    auto userId = session->get<int>("user_id");
    auto userName = session->get<std::string>("user_name");
    auto db = app().getDbClient();

    db->execSqlAsync(
        "SELECT p.id, p.title, p.price, p.image_url, c.quantity "
        "FROM cart_items c JOIN products p ON c.product_id = p.id WHERE c.user_id = $1",
        [callback, userName, req](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", true);
            data.insert("csrf_token", ensureCsrfToken(req));

            std::vector<std::map<std::string, std::string>> items;
            double subtotal = 0.0;
            for (const auto& row : r) {
                std::map<std::string, std::string> item;
                item["id"] = row["id"].as<std::string>();
                item["title"] = row["title"].as<std::string>();
                item["price"] = row["price"].as<std::string>();
                item["image_url"] = row["image_url"].as<std::string>();
                item["quantity"] = row["quantity"].as<std::string>();
                items.push_back(item);
                subtotal += row["price"].as<double>() * row["quantity"].as<int>();
            }
            const double deliveryFee = calculateDeliveryFee(subtotal, "standard");
            data.insert("items", items);
            data.insert("subtotal", subtotal);
            data.insert("delivery_fee", deliveryFee);
            data.insert("grand_total", subtotal + deliveryFee);

            auto resp = HttpResponse::newHttpViewResponse("Checkout", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId
    );
}

void OrderController::processCheckout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto session = req->session();
    auto userId = session->get<int>("user_id");
    auto address = req->getParameter("address");
    auto phone = req->getParameter("phone");
    auto promoCode = req->getParameter("promo_code");
    auto deliveryType = req->getParameter("delivery_type");
    if (deliveryType.empty()) {
        deliveryType = "standard";
    }

    if (address.size() < 8) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Yetkazib berish manzili juda qisqa.")));
        return;
    }
    if (!isValidPhone(phone)) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Telefon raqam noto'g'ri.")));
        return;
    }

    auto db = app().getDbClient();

    try {
        auto cartItems = db->execSqlSync(
            "SELECT p.id, p.price, c.quantity FROM cart_items c "
            "JOIN products p ON c.product_id = p.id WHERE c.user_id = $1",
            userId
        );
        if (cartItems.empty()) {
            callback(HttpResponse::newRedirectionResponse("/cart"));
            return;
        }

        double subtotal = 0.0;
        for (const auto& row : cartItems) {
            subtotal += row["price"].as<double>() * row["quantity"].as<int>();
        }

        double discount = 0.0;
        if (!promoCode.empty()) {
            auto promoRes = db->execSqlSync(
                "SELECT discount_amount, discount_percentage, min_order_amount FROM promo_codes "
                "WHERE code = $1 AND is_active = TRUE "
                "AND (expiry_date IS NULL OR expiry_date > CURRENT_TIMESTAMP)",
                promoCode
            );
            if (!promoRes.empty() && subtotal >= promoRes[0]["min_order_amount"].as<double>()) {
                discount = promoRes[0]["discount_amount"].as<double>();
                if (promoRes[0]["discount_percentage"].as<int>() > 0) {
                    discount += subtotal * (promoRes[0]["discount_percentage"].as<int>() / 100.0);
                }
            }
        }

        double discountedSubtotal = subtotal - discount;
        if (discountedSubtotal < 0.0) {
            discountedSubtotal = 0.0;
        }
        const double deliveryFee = calculateDeliveryFee(discountedSubtotal, deliveryType);
        const double finalTotal = discountedSubtotal + deliveryFee;
        const std::string deliveryTag = (deliveryType == "express") ? "Express" : "Standart";
        const std::string shippingAddress = address + " [" + deliveryTag + "]";

        auto txn = db->newTransaction();
        auto orderRes = txn->execSqlSync(
            "INSERT INTO orders (user_id, total_price, status, shipping_address, phone_number, promo_code, final_total) "
            "VALUES ($1, $2, 'Pending', $3, $4, $5, $6) RETURNING id",
            userId, subtotal, shippingAddress, phone, promoCode, finalTotal
        );
        const int orderId = orderRes[0]["id"].as<int>();

        for (const auto& row : cartItems) {
            txn->execSqlSync(
                "INSERT INTO order_items (order_id, product_id, quantity, price) VALUES ($1, $2, $3, $4)",
                orderId, row["id"].as<int>(), row["quantity"].as<int>(), row["price"].as<double>()
            );
        }
        txn->execSqlSync("DELETE FROM cart_items WHERE user_id = $1", userId);
        callback(HttpResponse::newRedirectionResponse("/orders"));
    } catch (const drogon::orm::DrogonDbException& e) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
    }
}

void OrderController::myOrders(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!req->session()->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    auto userId = req->session()->get<int>("user_id");
    auto userName = req->session()->get<std::string>("user_name");
    
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT id, total_price, final_total, status, shipping_address, created_at FROM orders WHERE user_id = $1 ORDER BY created_at DESC",
        [callback, userName, req](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("user_name", userName);
            data.insert("is_logged_in", true);
            data.insert("csrf_token", ensureCsrfToken(req));
            
            std::vector<std::map<std::string, std::string>> orders;
            for (const auto& row : r) {
                std::map<std::string, std::string> order;
                order["id"] = row["id"].as<std::string>();
                order["total_price"] = row["total_price"].as<std::string>();
                order["final_total"] = row["final_total"].isNull() ? order["total_price"] : row["final_total"].as<std::string>();
                order["status"] = row["status"].as<std::string>();
                order["shipping_address"] = row["shipping_address"].isNull() ? "" : row["shipping_address"].as<std::string>();
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
