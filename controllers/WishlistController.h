#pragma once
#include <drogon/HttpController.h>
#include "filters/LoginFilter.h"
#include "filters/AdminFilter.h"

using namespace drogon;

class WishlistController : public drogon::HttpController<WishlistController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(WishlistController::viewWishlist, "/wishlist", Get, "LoginFilter");
        ADD_METHOD_TO(WishlistController::addToWishlist, "/wishlist/add/{id}", Post, "LoginFilter");
        ADD_METHOD_TO(WishlistController::removeFromWishlist, "/wishlist/remove/{id}", Post, "LoginFilter");
    METHOD_LIST_END

    void viewWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void addToWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
    void removeFromWishlist(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, int id);
};
