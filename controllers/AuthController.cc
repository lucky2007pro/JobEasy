#include "AuthController.h"
#include <drogon/utils/Utilities.h> // SHA256 uchun

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

void AuthController::loginForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    HttpViewData data;
    data.insert("csrf_token", ensureCsrfToken(req));
    auto resp = HttpResponse::newHttpViewResponse("Login", data);
    callback(resp);
}

void AuthController::handleLogin(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
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
                std::string role = result[0]["role"].as<std::string>();
                session->insert("user_id", result[0]["id"].as<int>());
                session->insert("user_name", result[0]["full_name"].as<std::string>());
                session->insert("user_role", role);

                if (role == "admin") {
                    auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
                    callback(resp);
                } else {
                    auto resp = HttpResponse::newRedirectionResponse("/");
                    callback(resp);
                }
            }
            else {
                // Xato ma'lumotlar
                HttpViewData data;
                data.insert("csrf_token", ensureCsrfToken(req));
                auto resp = HttpResponse::newHttpViewResponse("Login", data);
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
    auto resp = HttpResponse::newRedirectionResponse("/login");
    callback(resp);
}

// Formani ko'rsatish
void AuthController::registerForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    HttpViewData data;
    data.insert("csrf_token", ensureCsrfToken(req));
    auto resp = HttpResponse::newHttpViewResponse("Register", data);
    callback(resp);
}

// Ma'lumotni bazaga saqlash
void AuthController::handleRegister(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
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
            auto resp = HttpResponse::newRedirectionResponse("/login");
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

