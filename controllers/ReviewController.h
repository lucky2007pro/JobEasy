#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class ReviewController : public drogon::HttpController<ReviewController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewController::addReview, "/api/review", Post);
    METHOD_LIST_END

    void addReview(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
