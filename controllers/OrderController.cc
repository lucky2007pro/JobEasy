#include "OrderController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <map>
#include <cctype>
#include <sstream>
#include <iomanip>

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

// Professional HTML error page helper
HttpResponsePtr makeErrorPage(const std::string &title, const std::string &message, const std::string &backUrl = "/checkout", const std::string &backText = "Orqaga qaytish")
{
    std::string html =
        "<!DOCTYPE html><html lang=\"uz\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>CyberShop | Xatolik</title>"
        "<script src=\"https://cdn.tailwindcss.com\"></script>"
        "<link href=\"https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700;800&display=swap\" rel=\"stylesheet\">"
        "<style>body{font-family:'Outfit',sans-serif;background:#f8fafc;}</style></head>"
        "<body class=\"min-h-screen flex items-center justify-center p-4\">"
        "<div class=\"max-w-md w-full text-center space-y-6\">"
        "<div class=\"w-20 h-20 mx-auto rounded-2xl bg-red-50 border-2 border-red-100 flex items-center justify-center\">"
        "<svg class=\"w-10 h-10 text-red-500\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\">"
        "<path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L4.082 16.5c-.77.833.192 2.5 1.732 2.5z\"/>"
        "</svg></div>"
        "<div><h1 class=\"text-2xl font-extrabold text-slate-800 tracking-tight\">" + title + "</h1>"
        "<p class=\"text-sm text-slate-500 font-medium mt-2 leading-relaxed\">" + message + "</p></div>"
        "<a href=\"" + backUrl + "\" class=\"inline-block bg-amber-600 hover:bg-amber-700 text-white font-bold py-3 px-8 rounded-xl text-sm uppercase tracking-widest transition-all shadow-lg shadow-amber-600/20\">" + backText + "</a>"
        "</div></body></html>";
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(html);
    return resp;
}
}  // namespace

void OrderController::checkoutPage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto session = req->session();
    auto userId = session->get<int>("user_id");
    auto userName = session->get<std::string>("user_name");
    auto db = app().getDbClient();

    db->execSqlAsync(
        "SELECT p.id, p.title, p.price, pi.image_url, c.quantity "
        "FROM cart_items c JOIN products p ON c.product_id = p.id "
        "LEFT JOIN product_images pi ON (p.id = pi.product_id AND pi.is_primary = TRUE) "
        "WHERE c.user_id = $1",
        [callback, userName, req, db, userId](const drogon::orm::Result& r) {
            if (r.empty()) {
                callback(makeErrorPage("Savat bo'sh", "Buyurtma berish uchun avval savatga mahsulot qo'shing.", "/", "Do'konga qaytish"));
                return;
            }

            // Get user balance for display
            try {
                auto balRes = db->execSqlSync("SELECT COALESCE(balance, 0) AS balance FROM users WHERE id = $1", userId);
                double userBalance = balRes.empty() ? 0.0 : balRes[0]["balance"].as<double>();

                HttpViewData data;
                data.insert("user_name", userName);
                data.insert("is_logged_in", true);
                data.insert("csrf_token", ensureCsrfToken(req));
                data.insert("user_balance", userBalance);

                std::vector<std::map<std::string, std::string>> items;
                double subtotal = 0.0;
                for (const auto& row : r) {
                    std::map<std::string, std::string> item;
                    item["id"] = row["id"].as<std::string>();
                    item["title"] = row["title"].as<std::string>();
                    item["price"] = row["price"].as<std::string>();
                    item["image_url"] = row["image_url"].isNull() ? "" : row["image_url"].as<std::string>();
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
            } catch (const std::exception& e) {
                LOG_ERROR << "Checkout page error: " << e.what();
                callback(makeErrorPage("Xatolik", "Sahifani yuklashda muammo yuz berdi.", "/cart", "Savatga qaytish"));
            }
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "Checkout DB error: " << e.base().what();
            callback(makeErrorPage("Xatolik", "Ma'lumotlarni yuklashda xatolik yuz berdi.", "/cart", "Savatga qaytish"));
        },
        userId
    );
}

void OrderController::processCheckout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        callback(makeErrorPage("Xavfsizlik xatosi", "Sessiya muddati tugagan. Iltimos, sahifani yangilang.", "/checkout"));
        return;
    }

    auto session = req->session();
    auto userId = session->get<int>("user_id");
    auto address = req->getParameter("address");
    auto phone = req->getParameter("phone");
    auto promoCode = req->getParameter("promo_code");
    auto paymentMethod = req->getParameter("payment_method");
    auto deliveryType = req->getParameter("delivery_type");
    if (deliveryType.empty()) {
        deliveryType = "standard";
    }
    if (paymentMethod.empty()) {
        paymentMethod = "wallet";
    }

    if (address.size() < 8) {
        callback(makeErrorPage("Manzil juda qisqa", "Yetkazib berish manzilini kamida 8 belgidan iborat kiriting. Masalan: Toshkent shahri, Chilonzor tumani, 10-mavze, 5-uy.", "/checkout"));
        return;
    }
    if (!isValidPhone(phone)) {
        callback(makeErrorPage("Telefon raqam noto'g'ri", "Telefon raqamingizni to'g'ri formatda kiriting. Masalan: +998 90 123 45 67.", "/checkout"));
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
            callback(makeErrorPage("Savat bo'sh", "Buyurtma berish uchun avval savatga mahsulot qo'shing.", "/", "Do'konga qaytish"));
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

        // Wallet payment: check balance and deduct
        if (paymentMethod == "wallet") {
            auto balRes = db->execSqlSync("SELECT COALESCE(balance, 0) AS balance FROM users WHERE id = $1", userId);
            double balance = balRes.empty() ? 0.0 : balRes[0]["balance"].as<double>();
            if (balance < finalTotal) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(0);
                oss << "Hamyonda yetarli mablag' yo'q. Sizda " << balance << " so'm bor, lekin buyurtma uchun " << finalTotal << " so'm kerak.";
                callback(makeErrorPage("Mablag' yetarli emas", oss.str(), "/wallet", "Hamyonga o'tish"));
                return;
            }
        }

        auto txn = db->newTransaction();

        // Deduct from wallet
        if (paymentMethod == "wallet") {
            txn->execSqlSync("UPDATE users SET balance = balance - $1 WHERE id = $2", finalTotal, userId);
            txn->execSqlSync(
                "INSERT INTO transactions (user_id, amount, type, status, description) "
                "VALUES ($1, $2, 'payment', 'completed', 'Buyurtma uchun to''lov')",
                userId, finalTotal
            );
        }

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
        LOG_ERROR << "Checkout error: " << e.base().what();
        callback(makeErrorPage("Buyurtma xatosi", "Buyurtmani qayta ishlashda muammo yuz berdi. Iltimos, qaytadan urinib ko'ring.", "/checkout"));
    }
}

void OrderController::cancelOrder(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int orderId) {
    if (!validateCsrfToken(req)) {
        callback(makeErrorPage("Xavfsizlik xatosi", "Sessiya muddati tugagan.", "/orders", "Buyurtmalarga qaytish"));
        return;
    }
    auto session = req->session();
    if (!session->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    auto userId = session->get<int>("user_id");
    auto db = app().getDbClient();

    try {
        auto orderRes = db->execSqlSync(
            "SELECT id, status, final_total FROM orders WHERE id = $1 AND user_id = $2",
            orderId, userId
        );
        if (orderRes.empty()) {
            callback(makeErrorPage("Buyurtma topilmadi", "Bunday buyurtma mavjud emas yoki sizga tegishli emas.", "/orders", "Buyurtmalarga qaytish"));
            return;
        }
        auto status = orderRes[0]["status"].as<std::string>();
        if (status != "Pending") {
            callback(makeErrorPage("Bekor qilib bo'lmaydi", "Faqat 'Kutilmoqda' holatidagi buyurtmalarni bekor qilish mumkin.", "/orders", "Buyurtmalarga qaytish"));
            return;
        }

        double finalTotal = orderRes[0]["final_total"].isNull() ? 0.0 : orderRes[0]["final_total"].as<double>();

        auto txn = db->newTransaction();
        txn->execSqlSync("UPDATE orders SET status = 'Cancelled' WHERE id = $1", orderId);

        // Refund to wallet
        if (finalTotal > 0.0) {
            txn->execSqlSync("UPDATE users SET balance = balance + $1 WHERE id = $2", finalTotal, userId);
            txn->execSqlSync(
                "INSERT INTO transactions (user_id, amount, type, status, description) "
                "VALUES ($1, $2, 'refund', 'completed', 'Buyurtma #" + std::to_string(orderId) + " bekor qilish qaytarimi')",
                userId, finalTotal
            );
        }

        callback(HttpResponse::newRedirectionResponse("/orders"));
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Cancel order error: " << e.base().what();
        callback(makeErrorPage("Xatolik", "Buyurtmani bekor qilishda muammo yuz berdi.", "/orders", "Buyurtmalarga qaytish"));
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
            LOG_ERROR << "MyOrders error: " << e.base().what();
            callback(makeErrorPage("Xatolik", "Buyurtmalar ro'yxatini yuklashda muammo.", "/", "Asosiy sahifa"));
        },
        userId
    );
}
