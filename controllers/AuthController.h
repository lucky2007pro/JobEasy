#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::loginForm, "/login", Get);
        ADD_METHOD_TO(AuthController::loginForm, "/login/", Get);
        ADD_METHOD_TO(AuthController::loginForm, "/api/login", Get);
        ADD_METHOD_TO(AuthController::loginForm, "/api/login/", Get);
        ADD_METHOD_TO(AuthController::handleLogin, "/login", Post);
        ADD_METHOD_TO(AuthController::handleLogin, "/login/", Post);
        ADD_METHOD_TO(AuthController::handleLogin, "/api/login", Post);
        ADD_METHOD_TO(AuthController::handleLogin, "/api/login/", Post);
        ADD_METHOD_TO(AuthController::registerForm, "/register", Get);
        ADD_METHOD_TO(AuthController::registerForm, "/register/", Get);
        ADD_METHOD_TO(AuthController::registerForm, "/api/register", Get);
        ADD_METHOD_TO(AuthController::registerForm, "/api/register/", Get);
        ADD_METHOD_TO(AuthController::logout, "/logout", Get);
        ADD_METHOD_TO(AuthController::logout, "/logout/", Get);
        ADD_METHOD_TO(AuthController::logout, "/api/logout", Get);
        ADD_METHOD_TO(AuthController::logout, "/api/logout/", Get);
        ADD_METHOD_TO(AuthController::handleRegister, "/register", Post);
        ADD_METHOD_TO(AuthController::handleRegister, "/register/", Post);
        ADD_METHOD_TO(AuthController::handleRegister, "/api/register", Post);
        ADD_METHOD_TO(AuthController::handleRegister, "/api/register/", Post);
    METHOD_LIST_END

        void loginForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void handleLogin(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void logout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void registerForm(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void handleRegister(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};