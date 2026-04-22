#include "PaymentController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <drogon/orm/Result.h>
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

std::string escapeHtml(const std::string &input)
{
    std::string out;
    out.reserve(input.size());
    for (char c : input)
    {
        if (c == '&')
        {
            out += "&amp;";
        }
        else if (c == '<')
        {
            out += "&lt;";
        }
        else if (c == '>')
        {
            out += "&gt;";
        }
        else if (c == '"')
        {
            out += "&quot;";
        }
        else if (c == '\'')
        {
            out += "&#39;";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

std::string mapValueOrDefault(const std::map<std::string, std::string> &m,
                              const std::string &key,
                              const std::string &fallback = "")
{
    const auto it = m.find(key);
    if (it == m.end())
    {
        return fallback;
    }
    return it->second;
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
    const auto token = req->getParameter("csrf_token");
    if (token.empty())
    {
        return false;
    }
    return token == session->get<std::string>("csrf_token");
}
}  // namespace

void PaymentController::walletPage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient("default");
    try {
        auto session = request->session();
        int userId = 0;
        if (session->find("user_id")) {
            userId = session->get<int>("user_id");
        } else {
            callback(HttpResponse::newRedirectionResponse("/login"));
            return;
        }
        LOG_INFO << "Wallet page request, user_id=" << userId;
        LOG_INFO << "Wallet step: querying user row";
        auto userRes = dbClient->execSqlSync(
            "SELECT COALESCE(balance::text, '0') AS balance, "
            "COALESCE(full_name, '') AS full_name "
            "FROM users WHERE id = $1",
            userId
        );
        LOG_INFO << "Wallet step: user row query done";
        if (userRes.empty()) {
            LOG_DEBUG << "Wallet: User " << userId << " not found in DB";
            callback(HttpResponse::newNotFoundResponse());
            return;
        }

        std::string balance = columnToStringSafe(userRes[0], "balance", "0");
        std::string userName = columnToStringSafe(userRes[0], "full_name", "");

        LOG_INFO << "Wallet step: querying cards";
        auto cardRes = dbClient->execSqlSync(
            "SELECT id::text AS id, "
            "COALESCE(card_number_masked, '**** **** **** ****') AS card_number_masked, "
            "COALESCE(card_holder, 'NOMA''LUM') AS card_holder, "
            "COALESCE(provider, 'CARD') AS provider "
            "FROM user_cards WHERE user_id = $1 ORDER BY created_at DESC",
            userId
        );
        LOG_INFO << "Wallet step: cards query done";
        std::vector<std::map<std::string, std::string>> cards;
        for (const auto& row : cardRes) {
            std::map<std::string, std::string> card;
            card["id"] = columnToStringSafe(row, "id", "");
            card["number"] = columnToStringSafe(row, "card_number_masked", "**** **** **** ****");
            card["holder"] = columnToStringSafe(row, "card_holder", "NOMA'LUM");
            card["provider"] = columnToStringSafe(row, "provider", "CARD");
            cards.push_back(card);
        }
        LOG_INFO << "Wallet step: querying transactions";
        auto transRes = dbClient->execSqlSync(
            "SELECT COALESCE(amount::text, '0') AS amount, "
            "COALESCE(type, 'payment') AS type, "
            "COALESCE(status, '') AS status, "
            "COALESCE(description, 'Tranzaksiya') AS description, "
            "COALESCE(created_at::text, '') AS created_at "
            "FROM transactions WHERE user_id = $1 ORDER BY created_at DESC LIMIT 10",
            userId
        );
        LOG_INFO << "Wallet step: transactions query done";
        std::vector<std::map<std::string, std::string>> transactions;
        for (const auto& row : transRes) {
            std::map<std::string, std::string> trans;
            trans["amount"] = columnToStringSafe(row, "amount", "0");
            trans["type"] = columnToStringSafe(row, "type", "payment");
            trans["status"] = columnToStringSafe(row, "status", "");
            trans["description"] = columnToStringSafe(row, "description", "Tranzaksiya");
            trans["created_at"] = columnToStringSafe(row, "created_at", "");
            transactions.push_back(trans);
        }
        HttpViewData data;
        data.insert("balance", balance);
        data.insert("user_name", userName);
        data.insert("cards", cards);
        data.insert("transactions", transactions);
        data.insert("csrf_token", ensureCsrfToken(request));
        auto resp = HttpResponse::newHttpViewResponse("Wallet", data);
        LOG_INFO << "Wallet page rendered, user_id=" << userId
                 << ", cards=" << cards.size()
                 << ", tx=" << transactions.size();
        callback(resp);
    } catch (const drogon::orm::DrogonDbException& e) {
        LOG_ERROR << "Wallet DB exception: " << e.base().what();
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Wallet DB xatolik")));
    } catch (const std::exception& e) {
        LOG_ERROR << "Captured exception in walletPage: " << e.what();
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Ichki xatolik")));
    }
}

void PaymentController::addCardForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        auto session = request->session();
        int userId = session->find("user_id") ? session->get<int>("user_id") : 0;
        LOG_INFO << "AddCard page request, user_id=" << userId;
        const auto csrf = ensureCsrfToken(request);

        // NOTE: Wallet view had render-related crash before. Keep add-card page
        // response explicit to isolate CSP rendering issues safely.
        std::string body =
            "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Karta qo'shish</title>"
            "<style>body{font-family:Arial,sans-serif;background:#f8fafc;margin:0;padding:24px;}"
            ".card{max-width:520px;margin:40px auto;background:#fff;border:1px solid #e2e8f0;border-radius:14px;padding:20px;}"
            "input{width:100%;padding:10px 12px;border:1px solid #dbe3ef;border-radius:10px;margin-top:6px;margin-bottom:12px;}"
            "button{background:#4f46e5;color:#fff;border:none;border-radius:10px;padding:10px 14px;font-weight:700;cursor:pointer;}"
            "a{color:#4f46e5;text-decoration:none;}</style></head><body>"
            "<div class=\"card\"><h2>Karta qo'shish</h2>"
            "<form action=\"/wallet/add-card\" method=\"POST\">"
            "<input type=\"hidden\" name=\"csrf_token\" value=\"" + escapeHtml(csrf) + "\">"
            "<label>Karta raqami</label><input type=\"text\" name=\"card_number\" maxlength=\"19\" required>"
            "<label>Karta egasi</label><input type=\"text\" name=\"card_holder\" required>"
            "<label>Muddati (MM/YY)</label><input type=\"text\" name=\"expiry_date\" maxlength=\"5\" required>"
            "<button type=\"submit\">Saqlash</button></form>"
            "<p style=\"margin-top:12px;\"><a href=\"/wallet\">Hamyonga qaytish</a></p></div></body></html>";

        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(body);
        LOG_INFO << "AddCard page rendered, user_id=" << userId;
        callback(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "Captured exception in addCardForm: " << e.what();
        callback(HttpResponse::newHttpJsonResponse(Json::Value("AddCard ichki xatolik")));
    }
}

void PaymentController::saveCard(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    LOG_INFO << "AddCard save request started";
    if (!validateCsrfToken(request)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto params = request->getParameters();
    auto session = request->session();
    if (!session->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    int userId = session->get<int>("user_id");

    std::string cardNum = params["card_number"];
    std::string holder = params["card_holder"];
    std::string expiry = params["expiry_date"];
    LOG_INFO << "AddCard payload received, card_length=" << cardNum.length();

    if (cardNum.length() < 16) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: Karta raqami noto'g'ri")));
        return;
    }

    // Mask the card number (e.g., 8600 **** **** 1234)
    std::string masked = cardNum.substr(0, 4) + " **** **** " + cardNum.substr(cardNum.length() - 4);

    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "INSERT INTO user_cards (user_id, card_number_masked, card_holder, expiry_date, provider) VALUES ($1, $2, $3, $4, $5)",
        [callback](const drogon::orm::Result& r) {
            LOG_INFO << "AddCard save success";
            auto resp = HttpResponse::newRedirectionResponse("/wallet");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "AddCard save db error: " << e.base().what();
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId, masked, holder, expiry, "UZCARD"
    );
}

void PaymentController::topupBalance(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    if (!validateCsrfToken(request)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto params = request->getParameters();
    auto session = request->session();
    if (!session->find("user_id")) {
        callback(HttpResponse::newRedirectionResponse("/login"));
        return;
    }
    int userId = session->get<int>("user_id");
    double amount = 0.0;
    int cardId = 0;
    try {
        amount = std::stod(params["amount"]);
        cardId = std::stoi(params["card_id"]);
    } catch (const std::exception&) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: noto'g'ri summa yoki karta.")));
        return;
    }
    if (amount <= 0 || cardId <= 0) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: summa yoki karta noto'g'ri.")));
        return;
    }

    auto dbClient = drogon::app().getDbClient("default");

    // Ensure the selected card belongs to this user before balance update.
    dbClient->execSqlAsync(
        "SELECT id FROM user_cards WHERE id = $1 AND user_id = $2",
        [callback, dbClient, userId, amount](const drogon::orm::Result& cardCheck) {
            if (cardCheck.empty()) {
                callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: karta topilmadi.")));
                return;
            }
    dbClient->execSqlAsync(
        "UPDATE users SET balance = balance + $1 WHERE id = $2",
        [callback, dbClient, userId, amount](const drogon::orm::Result& r) {
            dbClient->execSqlAsync(
                "INSERT INTO transactions (user_id, amount, type, description) VALUES ($1, $2, 'top-up', 'Karta orqali balansni to''ldirish')",
                [callback](const drogon::orm::Result& tr) {
                    auto resp = HttpResponse::newRedirectionResponse("/wallet");
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                },
                userId, amount
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        amount, userId
    );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        cardId, userId
    );
}
