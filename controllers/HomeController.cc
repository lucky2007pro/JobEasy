#include "HomeController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <vector>
#include <map>

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

void HomeController::index(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto dbClient = drogon::app().getDbClient("default");

    std::string userName = "Mehmon";
    bool isLoggedIn = false;
    if (req->session()->find("user_name")) {
        userName = req->session()->get<std::string>("user_name");
        isLoggedIn = true;
    }

    const auto csrfToken = ensureCsrfToken(req);

    std::string pageStr = req->getParameter("page");
    int page = 1;
    if (!pageStr.empty()) {
        try { page = std::stoi(pageStr); } catch (...) { page = 1; }
    }
    int pageSize = 12;
    int offset = (page - 1) * pageSize;

    std::string sortBy = req->getParameter("sort");
    std::string sql = "SELECT p.id, p.title, p.price, pi.image_url, p.discount_percentage "
        "FROM products p "
        "LEFT JOIN product_images pi ON (p.id = pi.product_id AND pi.is_primary = TRUE) "
        "WHERE p.is_active = TRUE";
    
    if (sortBy == "price_asc") {
        sql += " ORDER BY p.price ASC";
    } else if (sortBy == "price_desc") {
        sql += " ORDER BY p.price DESC";
    } else if (sortBy == "oldest") {
        sql += " ORDER BY id ASC";
    } else {
        sql += " ORDER BY id DESC"; // default is newest
    }

    sql += " LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);

    dbClient->execSqlAsync(
        sql,
        [callback, userName, isLoggedIn, csrfToken, dbClient, page](const drogon::orm::Result& r) {
            std::vector<std::map<std::string, std::string>> products;
            for (const auto& row : r) {
                std::map<std::string, std::string> product;
                product["id"] = row["id"].as<std::string>();
                product["title"] = row["title"].as<std::string>();
                product["price"] = row["price"].as<std::string>();
                product["image_url"] = row["image_url"].isNull() ? "" : row["image_url"].as<std::string>();
                product["discount_percentage"] = row["discount_percentage"].isNull() ? "0" : row["discount_percentage"].as<std::string>();
                products.push_back(product);
            }

            // Endi kategoriyalarni olamiz
            dbClient->execSqlAsync(
                "SELECT id, name FROM categories ORDER BY name ASC",
                [=, products = std::move(products)](const drogon::orm::Result& cat_res) {
                    HttpViewData data;
                    data.insert("user_name", userName);
                    data.insert("is_logged_in", isLoggedIn);
                    data.insert("csrf_token", csrfToken);
                    data.insert("products", products);
                    data.insert("current_page", page);

                    std::vector<std::map<std::string, std::string>> categories;
                    for (const auto& cat_row : cat_res) {
                        std::map<std::string, std::string> cat;
                        cat["id"] = cat_row["id"].as<std::string>();
                        cat["name"] = cat_row["name"].as<std::string>();
                        categories.push_back(cat);
                    }
                    data.insert("categories", categories);

                    auto resp = HttpResponse::newHttpViewResponse("HomePage", data);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
                    callback(resp);
                }
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = HttpResponse::newHttpJsonResponse(Json::Value(e.base().what()));
            callback(resp);
        }
    );
}
