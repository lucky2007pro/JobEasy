#pragma once
#include <drogon/HttpController.h>
#include "filters/LoginFilter.h"

using namespace drogon;

class OrderController : public drogon::HttpController<OrderController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(OrderController::checkoutPage, "/checkout", Get, "LoginFilter");
        ADD_METHOD_TO(OrderController::processCheckout, "/checkout", Post, "LoginFilter");
        ADD_METHOD_TO(OrderController::myOrders, "/orders", Get, "LoginFilter");
        ADD_METHOD_TO(OrderController::cancelOrder, "/orders/cancel/{id}", Post, "LoginFilter");
    METHOD_LIST_END

    void checkoutPage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void processCheckout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void myOrders(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void cancelOrder(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int orderId);
};
