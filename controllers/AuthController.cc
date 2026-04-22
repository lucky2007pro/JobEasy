#include "AuthController.h"
#include <drogon/utils/Utilities.h> // SHA256 uchun

void AuthController::loginForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto resp = HttpResponse::newHttpViewResponse("Login");
    callback(resp);
}

void AuthController::handleLogin(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto params = req->getParameters();
    std::string email = params["email"];
    std::string password = params["password"];

    // Parolni xavfsizlik uchun SHA256 bilan shifrlaymiz
    std::string hashed_pw = drogon::utils::getSha256(password);

    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT id, full_name, role FROM users WHERE email = $1 AND password_hash = $2",
        [callback, req](const drogon::orm::Result& result) {
            if (result.size() > 0) {
                // Foydalanuvchi topildi! Session'ga saqlaymiz
                auto session = req->session();
                session->insert("user_id", result[0]["id"].as<int>());
                session->insert("user_name", result[0]["full_name"].as<std::string>());
                session->insert("user_role", result[0]["role"].as<std::string>());

                auto resp = HttpResponse::newRedirectionResponse("/");
                callback(resp);
            }
            else {
                // Xato ma'lumotlar
                auto resp = HttpResponse::newHttpViewResponse("Login");
                // Bu yerda xatolik xabarini yuborish mumkin
                callback(resp);
            }
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        email, hashed_pw
    );
}

void AuthController::logout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    req->session()->erase("user_id");
    req->session()->erase("user_name");
    req->session()->erase("user_role");
    auto resp = HttpResponse::newRedirectionResponse("/api/login");
    callback(resp);
}

// Formani ko'rsatish
void AuthController::registerForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto resp = HttpResponse::newHttpViewResponse("Register");
    callback(resp);
}

// Ma'lumotni bazaga saqlash
void AuthController::handleRegister(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto params = req->getParameters();
    std::string name = params["full_name"];
    std::string email = params["email"];
    std::string password = params["password"];

    // 1. Parolni xavfsiz xeshga o'girish
    std::string hashed_pw = drogon::utils::getSha256(password);

    auto dbClient = drogon::app().getDbClient("default");

    // 2. Bazaga yozish
    dbClient->execSqlAsync(
        "INSERT INTO users (full_name, email, password_hash, role) VALUES ($1, $2, $3, 'user')",
        [callback](const drogon::orm::Result& r) {
            // Ro'yxatdan o'tgach, to'g'ri login sahifasiga yuboramiz
            auto resp = HttpResponse::newRedirectionResponse("/api/login");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            // Agar email band bo'lsa yoki boshqa xato bo'lsa
            Json::Value ret;
            ret["error"] = "Xatolik: Email band bo'lishi mumkin.";
            callback(HttpResponse::newHttpJsonResponse(ret));
        },
        name, email, hashed_pw
    );
}
