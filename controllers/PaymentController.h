#pragma once
#include <drogon/HttpController.h>
#include "filters/LoginFilter.h"

using namespace drogon;

class PaymentController : public drogon::HttpController<PaymentController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PaymentController::walletPage, "/wallet", Get, "LoginFilter");
        ADD_METHOD_TO(PaymentController::addCardForm, "/wallet/add-card", Get, "LoginFilter");
        ADD_METHOD_TO(PaymentController::saveCard, "/wallet/add-card", Post, "LoginFilter");
        ADD_METHOD_TO(PaymentController::topupBalance, "/wallet/topup", Post, "LoginFilter");
    METHOD_LIST_END

    void walletPage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
    void addCardForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
    void saveCard(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
    void topupBalance(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback);
};
