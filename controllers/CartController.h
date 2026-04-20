#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class CartController : public drogon::HttpController<CartController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CartController::viewCart, "/cart", Get, "LoginFilter");
        ADD_METHOD_TO(CartController::addToCart, "/cart/add", Post, "LoginFilter");
        ADD_METHOD_TO(CartController::removeFromCart, "/cart/remove/{1}", Get, "LoginFilter");
    METHOD_LIST_END

    void viewCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void addToCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void removeFromCart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
};
