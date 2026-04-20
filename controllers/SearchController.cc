#include "SearchController.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <map>

using namespace drogon;
using namespace drogon::orm;

void SearchController::search(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto query = req->getParameter("q");
    auto categoryStr = req->getParameter("category_id");
    
    auto db = app().getDbClient();
    std::string sql = "SELECT * FROM products WHERE is_active = TRUE ";
    
    if (!query.empty()) {
        sql += " AND (title ILIKE $1 OR description ILIKE $1)";
    }
    if (!categoryStr.empty()) {
        sql += " AND category_id = " + categoryStr; // In a robust app, use parameterized query for this too
    }
    
    sql += " ORDER BY id DESC";

    try {
        Result result;
        if (!query.empty()) {
            result = db->execSqlSync(sql, "%" + query + "%");
        } else {
            result = db->execSqlSync(sql);
        }

        HttpViewData data;
        std::vector<std::map<std::string, std::string>> products;
        
        for (auto row : result) {
            std::map<std::string, std::string> p;
            p["id"] = row["id"].as<std::string>();
            p["title"] = row["title"].as<std::string>();
            p["price"] = row["price"].as<std::string>();
            p["image_url"] = row["image_url"].as<std::string>();
            p["description"] = row["description"].as<std::string>();
            p["stock"] = row["stock"].as<std::string>();
            p["discount_percentage"] = row["discount_percentage"].as<std::string>();
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
