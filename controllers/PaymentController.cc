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

        dbClient->execSqlAsync(
            "SELECT COALESCE(balance::text, '0') AS balance, "
            "COALESCE(full_name, '') AS full_name "
            "FROM users WHERE id = $1",
            [callback, request, dbClient, userId](const drogon::orm::Result& r) {
                if (r.empty()) {
                    LOG_DEBUG << "Wallet: User " << userId << " not found in DB";
                    callback(HttpResponse::newNotFoundResponse());
                    return;
                }

                HttpViewData data;
                std::string balance = columnToStringSafe(r[0], "balance", "0");
                data.insert("balance", balance);
                data.insert("user_name", columnToStringSafe(r[0], "full_name", ""));
                data.insert("user_id", std::to_string(userId));

                // Fetch linked cards
                dbClient->execSqlAsync(
                    "SELECT id::text AS id, "
                    "COALESCE(card_number_masked, '**** **** **** ****') AS card_number_masked, "
                    "COALESCE(card_holder, 'NOMA''LUM') AS card_holder, "
                    "COALESCE(provider, 'CARD') AS provider "
                    "FROM user_cards WHERE user_id = $1 ORDER BY created_at DESC",
                    [callback, data, dbClient, userId](const drogon::orm::Result& card_res) mutable {
                        std::vector<std::map<std::string, std::string>> cards;
                        for (const auto& row : card_res) {
                            std::map<std::string, std::string> card;
                            card["id"] = columnToStringSafe(row, "id", "");
                            card["number"] = columnToStringSafe(row, "card_number_masked", "**** **** **** ****");
                            card["holder"] = columnToStringSafe(row, "card_holder", "NOMA'LUM");
                            card["provider"] = columnToStringSafe(row, "provider", "CARD");
                            cards.push_back(card);
                        }
                        data.insert("cards", cards);

                        // Fetch transactions
                        dbClient->execSqlAsync(
                            "SELECT COALESCE(amount::text, '0') AS amount, "
                            "COALESCE(type, 'payment') AS type, "
                            "COALESCE(status, '') AS status, "
                            "COALESCE(description, 'Tranzaksiya') AS description, "
                            "COALESCE(created_at::text, '') AS created_at "
                            "FROM transactions WHERE user_id = $1 ORDER BY created_at DESC LIMIT 10",
                            [callback, data, userId, cards](const drogon::orm::Result& trans_res) mutable {
                                std::vector<std::map<std::string, std::string>> transactions;
                                for (const auto& row : trans_res) {
                                    std::map<std::string, std::string> trans;
                                    trans["amount"] = columnToStringSafe(row, "amount", "0");
                                    trans["type"] = columnToStringSafe(row, "type", "payment");
                                    trans["status"] = columnToStringSafe(row, "status", "");
                                    trans["description"] = columnToStringSafe(row, "description", "Tranzaksiya");
                                    trans["created_at"] = columnToStringSafe(row, "created_at", "");
                                    transactions.push_back(trans);
                                }
                                data.insert("transactions", transactions);

                                auto resp = HttpResponse::newHttpViewResponse("Wallet", data);
                                LOG_INFO << "Wallet page rendered, user_id=" << userId
                                         << ", cards=" << cards.size()
                                         << ", tx=" << transactions.size();
                                callback(resp);
                            },
                            [callback](const drogon::orm::DrogonDbException& e) {
                                callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                            },
                            userId
                        );
                    },
                    [callback, userId](const drogon::orm::DrogonDbException& e) {
                        LOG_ERROR << "Wallet Card Error for user " << userId << ": " << e.base().what();
                        callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                    },
                    userId
                );
            },
            [callback, userId](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Wallet User Error for user " << userId << ": " << e.base().what();
                callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
            },
            userId
        );
    } catch (const std::exception& e) {
        LOG_ERROR << "Captured exception in walletPage: " << e.what();
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Ichki xatolik")));
    }
}

void PaymentController::addCardForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    HttpViewData data;
    auto resp = HttpResponse::newHttpViewResponse("AddCard", data);
    callback(resp);
}

void PaymentController::saveCard(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
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
            auto resp = HttpResponse::newRedirectionResponse("/wallet");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId, masked, holder, expiry, "UZCARD"
    );
}

void PaymentController::topupBalance(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
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
