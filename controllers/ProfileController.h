#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ProfileController : public drogon::HttpController<ProfileController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProfileController::viewProfile, "/profile", Get);
    ADD_METHOD_TO(ProfileController::updateProfile, "/api/profile/update", Post);
    METHOD_LIST_END

    void viewProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void updateProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
