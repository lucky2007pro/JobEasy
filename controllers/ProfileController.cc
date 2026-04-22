#include "ProfileController.h"
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>
#include <map>

using namespace drogon;
using namespace drogon::orm;

namespace
{
std::string ensureCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->getSession();
    if (!session->find("csrf_token"))
    {
        session->insert("csrf_token", drogon::utils::getUuid());
    }
    return session->get<std::string>("csrf_token");
}

bool validateCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->getSession();
    if (!session->find("csrf_token"))
    {
        return false;
    }
    auto token = req->getParameter("csrf_token");
    if (token.empty())
    {
        return false;
    }
    return token == session->get<std::string>("csrf_token");
}
}  // namespace

void ProfileController::viewProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto session = req->getSession();
    
    int user_id = session->get<int>("user_id");
    
    try {
        auto db = app().getDbClient();
        
        // Ensure profile exists for the user; if not, create empty one dynamically
        auto check = db->execSqlSync("SELECT * FROM user_profiles WHERE user_id = $1", user_id);
        if (check.empty()) {
            db->execSqlSync("INSERT INTO user_profiles (user_id) VALUES ($1)", user_id);
            check = db->execSqlSync("SELECT * FROM user_profiles WHERE user_id = $1", user_id);
        }
        
        auto user_row = db->execSqlSync("SELECT full_name, email FROM users WHERE id = $1", user_id);
        
        HttpViewData data;
        std::map<std::string, std::string> profile;
        
        profile["full_name"] = user_row[0]["full_name"].as<std::string>();
        profile["email"] = user_row[0]["email"].as<std::string>();
        
        auto row = check[0];
        profile["phone_number"] = row["phone_number"].isNull() ? "" : row["phone_number"].as<std::string>();
        profile["address_line"] = row["address_line"].isNull() ? "" : row["address_line"].as<std::string>();
        profile["city"] = row["city"].isNull() ? "" : row["city"].as<std::string>();
        
        data.insert("profile", profile);
        data.insert("csrf_token", ensureCsrfToken(req));
        
        auto resp = HttpResponse::newHttpViewResponse("Profile", data);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void ProfileController::updateProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto session = req->getSession();
    
    int user_id = session->get<int>("user_id");
    if (!validateCsrfToken(req)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto phone = req->getParameter("phone_number");
    auto address = req->getParameter("address_line");
    auto city = req->getParameter("city");

    try {
        auto db = app().getDbClient();
        db->execSqlSync("UPDATE user_profiles SET phone_number = $1, address_line = $2, city = $3 WHERE user_id = $4",
                        phone, address, city, user_id);
                        
        auto resp = HttpResponse::newRedirectionResponse("/profile");
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newRedirectionResponse("/profile");
        callback(resp);
    }
}
