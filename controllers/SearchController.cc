#include "SearchController.h"
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <string>
#include <map>

using namespace drogon;
using namespace drogon::orm;

namespace
{
std::string ensureCsrfToken(const HttpRequestPtr &req)
{
    auto session = req->session();
    if (!session->find("csrf_token"))
    {
        session->insert("csrf_token", drogon::utils::getUuid());
    }
    return session->get<std::string>("csrf_token");
}
}  // namespace

void SearchController::search(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto query = req->getParameter("q");
    auto categoryStr = req->getParameter("category_id");
    
    std::string userName = "Mehmon";
    bool isLoggedIn = false;
    if (req->session()->find("user_name"))
    {
        userName = req->session()->get<std::string>("user_name");
        isLoggedIn = true;
    }

    int categoryId = 0;
    bool hasCategory = false;
    if (!categoryStr.empty())
    {
        try
        {
            categoryId = std::stoi(categoryStr);
            hasCategory = true;
        }
        catch (...)
        {
            hasCategory = false;
        }
    }

    auto db = app().getDbClient("default");
    std::string baseSql =
        "SELECT id, title, price, image_url, description, stock, discount_percentage "
        "FROM products WHERE is_active = TRUE";

    try {
        Result result;
        if (!query.empty() && hasCategory)
        {
            result = db->execSqlSync(baseSql + " AND (title ILIKE $1 OR description ILIKE $1) AND category_id = $2 ORDER BY id DESC",
                                     "%" + query + "%",
                                     categoryId);
        }
        else if (!query.empty())
        {
            result = db->execSqlSync(baseSql + " AND (title ILIKE $1 OR description ILIKE $1) ORDER BY id DESC",
                                     "%" + query + "%");
        }
        else if (hasCategory)
        {
            result = db->execSqlSync(baseSql + " AND category_id = $1 ORDER BY id DESC",
                                     categoryId);
        }
        else
        {
            result = db->execSqlSync(baseSql + " ORDER BY id DESC");
        }

        HttpViewData data;
        data.insert("user_name", userName);
        data.insert("is_logged_in", isLoggedIn);
        data.insert("csrf_token", ensureCsrfToken(req));
        std::vector<std::map<std::string, std::string>> products;
        
        for (auto row : result) {
            std::map<std::string, std::string> p;
            p["id"] = row["id"].as<std::string>();
            p["title"] = row["title"].as<std::string>();
            p["price"] = row["price"].as<std::string>();
            p["image_url"] = row["image_url"].isNull() ? "" : row["image_url"].as<std::string>();
            p["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
            p["stock"] = row["stock"].isNull() ? "0" : row["stock"].as<std::string>();
            p["discount_percentage"] = row["discount_percentage"].isNull() ? "0" : row["discount_percentage"].as<std::string>();
            products.push_back(p);
        }

        data.insert("search_query", query);
        data.insert("products_list", products);
        
        auto resp = HttpResponse::newHttpViewResponse("Search", data);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "Search failed: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
