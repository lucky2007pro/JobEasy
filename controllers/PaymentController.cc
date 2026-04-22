#include "PaymentController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <drogon/orm/Result.h>
#include <vector>
#include <map>

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

        dbClient->execSqlAsync(
            "SELECT balance, full_name FROM users WHERE id = $1",
            [callback, request, dbClient, userId](const drogon::orm::Result& r) {
                if (r.empty()) {
                    LOG_DEBUG << "Wallet: User " << userId << " not found in DB";
                    callback(HttpResponse::newNotFoundResponse());
                    return;
                }

                HttpViewData data;
                std::string balance = "0";
                if (!r[0]["balance"].isNull()) {
                    balance = r[0]["balance"].as<std::string>();
                }
                data.insert("balance", balance);
                data.insert("user_name", r[0]["full_name"].as<std::string>());
                data.insert("user_id", std::to_string(userId));

                // Fetch linked cards
                dbClient->execSqlAsync(
                    "SELECT id, card_number_masked, card_holder, provider FROM user_cards WHERE user_id = $1 ORDER BY created_at DESC",
                    [callback, data, dbClient, userId](const drogon::orm::Result& card_res) mutable {
                        std::vector<std::map<std::string, std::string>> cards;
                        for (const auto& row : card_res) {
                            std::map<std::string, std::string> card;
                            card["id"] = row["id"].as<std::string>();
                            card["number"] = row["card_number_masked"].as<std::string>();
                            card["holder"] = row["card_holder"].as<std::string>();
                            card["provider"] = row["provider"].as<std::string>();
                            cards.push_back(card);
                        }
                        data.insert("cards", cards);

                        // Fetch transactions
                        dbClient->execSqlAsync(
                            "SELECT amount, type, status, description, created_at FROM transactions WHERE user_id = $1 ORDER BY created_at DESC LIMIT 10",
                            [callback, data](const drogon::orm::Result& trans_res) mutable {
                                std::vector<std::map<std::string, std::string>> transactions;
                                for (const auto& row : trans_res) {
                                    std::map<std::string, std::string> trans;
                                    trans["amount"] = row["amount"].as<std::string>();
                                    trans["type"] = row["type"].as<std::string>();
                                    trans["status"] = row["status"].as<std::string>();
                                    trans["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
                                    trans["created_at"] = row["created_at"].as<std::string>();
                                    transactions.push_back(trans);
                                }
                                data.insert("transactions", transactions);

                                auto resp = HttpResponse::newHttpViewResponse("Wallet", data);
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
        "INSERT INTO user_cards (user_id, card_number_masked, card_holder, expiry_date) VALUES ($1, $2, $3, $4)",
        [callback](const drogon::orm::Result& r) {
            auto resp = HttpResponse::newRedirectionResponse("/wallet");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        userId, masked, holder, expiry
    );
}

void PaymentController::topupBalance(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto params = request->getParameters();
    auto session = request->session();
    int userId = session->get<int>("user_id");

    double amount = std::stod(params["amount"]);
    int cardId = std::stoi(params["card_id"]);

    auto dbClient = drogon::app().getDbClient("default");
    
    // Start transaction (or just execute sequence)
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
}
