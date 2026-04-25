#include "ProductController.h"
#include <drogon/HttpViewData.h>
#include <drogon/utils/Utilities.h>
#include <drogon/MultiPart.h>
#include <vector>
#include <map>
#include <filesystem>

namespace
{
std::string ensureCsrfToken(const HttpRequestPtr &request)
{
    auto session = request->session();
    if (!session->find("csrf_token"))
    {
        session->insert("csrf_token", drogon::utils::getUuid());
    }
    return session->get<std::string>("csrf_token");
}

bool validateCsrfToken(const HttpRequestPtr &request)
{
    auto session = request->session();
    if (!session->find("csrf_token"))
    {
        return false;
    }
    auto token = request->getParameter("csrf_token");
    if (token.empty())
    {
        return false;
    }
    return token == session->get<std::string>("csrf_token");
}
}  // namespace

void ProductController::showProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");

    std::string userName = "Mehmon";
    bool isLoggedIn = false;
    if (request->session()->find("user_name")) {
        userName = request->session()->get<std::string>("user_name");
        isLoggedIn = true;
    }
    const auto csrfToken = ensureCsrfToken(request);

    dbClient->execSqlAsync(
        "SELECT id, title, description, price, image_url, stock, discount_percentage FROM products WHERE id = $1",
        [callback, request, dbClient, id](const drogon::orm::Result& r) {
            if (r.empty()) {
                LOG_DEBUG << "ShowProduct: Product " << id << " not found in DB";
                callback(HttpResponse::newNotFoundResponse());
                return;
            }

            std::map<std::string, std::string> product;
            product["id"] = r[0]["id"].as<std::string>();
            product["title"] = r[0]["title"].as<std::string>();
            product["description"] = r[0]["description"].isNull() ? "" : r[0]["description"].as<std::string>();
            product["price"] = r[0]["price"].as<std::string>();
            product["stock"] = r[0]["stock"].as<std::string>();
            product["discount_percentage"] = r[0]["discount_percentage"].isNull() ? "0" : r[0]["discount_percentage"].as<std::string>();

            dbClient->execSqlAsync(
                "SELECT id, image_url, is_primary FROM product_images WHERE product_id = $1 ORDER BY is_primary DESC, id ASC",
                [callback, request, product, dbClient](const drogon::orm::Result& img_res) mutable {
                    std::vector<std::map<std::string, std::string>> images;
                    for (const auto& row : img_res) {
                        std::map<std::string, std::string> img;
                        img["id"] = row["id"].as<std::string>();
                        img["image_url"] = row["image_url"].as<std::string>();
                        img["is_primary"] = row["is_primary"].as<bool>() ? "1" : "0";
                        images.push_back(img);
                    }

                    dbClient->execSqlAsync(
                        "SELECT r.*, u.full_name FROM reviews r JOIN users u ON r.user_id = u.id WHERE r.product_id = $1",
                        [callback, request, product, images](const drogon::orm::Result& rev_res) {
                            HttpViewData data;
                            data.insert("product", product);
                            data.insert("product_images", images);
                            
                            std::vector<std::map<std::string, std::string>> reviews;
                            for (const auto& row : rev_res) {
                                std::map<std::string, std::string> rev;
                                rev["id"] = row["id"].as<std::string>();
                                rev["full_name"] = row["full_name"].as<std::string>();
                                rev["rating"] = row["rating"].as<std::string>();
                                rev["comment"] = row["comment"].as<std::string>();
                                rev["created_at"] = row["created_at"].as<std::string>();
                                reviews.push_back(rev);
                            }
                            data.insert("reviews", reviews);

                            auto session = request->session();
                            data.insert("is_logged_in", session->find("user_id"));
                            data.insert("user_name", session->find("user_name") ? session->get<std::string>("user_name") : std::string("Mehmon"));
                            data.insert("csrf_token", session->find("csrf_token") ? session->get<std::string>("csrf_token") : std::string(""));

                            auto resp = HttpResponse::newHttpViewResponse("ProductDetail", data);
                            callback(resp);
                        },
                        [callback](const drogon::orm::DrogonDbException& e) {
                            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                        },
                        product.at("id")
                    );
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                },
                id
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        id
    );
}

void ProductController::addProductForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT id, name FROM categories ORDER BY name ASC",
        [callback, request](const drogon::orm::Result& r) {
            HttpViewData data;
            data.insert("csrf_token", ensureCsrfToken(request));
            
            std::vector<std::map<std::string, std::string>> categories;
            for (const auto& row : r) {
                std::map<std::string, std::string> cat;
                cat["id"] = row["id"].as<std::string>();
                cat["name"] = row["name"].as<std::string>();
                categories.push_back(cat);
            }
            data.insert("categories_list", categories);

            auto resp = HttpResponse::newHttpViewResponse("AddProduct", data);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        }
    );
}

bool validateMultipartCsrf(const MultiPartParser& parser, const HttpRequestPtr& request) {
    auto session = request->session();
    if (!session->find("csrf_token")) return false;
    auto params = parser.getParameters();
    if (params.find("csrf_token") == params.end()) return false;
    return params.at("csrf_token") == session->get<std::string>("csrf_token");
}

void ProductController::createProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback) {
    MultiPartParser fileUpload;
    if (fileUpload.parse(request) != 0) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: Ma'lumotlarni o'qib bo'lmadi")));
        return;
    }

    if (!validateMultipartCsrf(fileUpload, request)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    if (fileUpload.getFiles().empty()) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Rasm tanlanmagan")));
        return;
    }

    auto &files = fileUpload.getFiles();
    auto params = fileUpload.getParameters();
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "INSERT INTO products (title, description, price, stock, category_id, is_active) VALUES ($1, $2, $3, $4, $5, TRUE) RETURNING id",
        [callback, files, dbClient](const drogon::orm::Result& r) {
            if (r.empty()) {
                callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: Mahsulot yaratilmadi")));
                return;
            }
            int productId = r[0]["id"].as<int>();

            for (size_t i = 0; i < files.size(); ++i) {
                auto &file = files[i];
                std::string fileName = file.getFileName();
                std::string fileExtension = std::filesystem::path(fileName).extension().string();
                std::string newFileName = drogon::utils::getUuid() + fileExtension;
                file.saveAs(newFileName);
                
                std::string dbImageUrl = "/uploads/" + newFileName;
                bool isPrimary = (i == 0);

                dbClient->execSqlAsync(
                    "INSERT INTO product_images (product_id, image_url, is_primary) VALUES ($1, $2, $3)",
                    [](const drogon::orm::Result& ir) {},
                    [](const drogon::orm::DrogonDbException& e) {
                        LOG_ERROR << "Rasm saqlashda xato: " << e.base().what();
                    },
                    productId, dbImageUrl, isPrimary
                );
            }

            auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        params["title"], params["description"], params["price"], params["stock"], params["category_id"]
    );
}

void ProductController::editProductForm(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT * FROM products WHERE id = $1",
        [callback, request, dbClient, id](const drogon::orm::Result& r) {
            if (r.empty()) {
                LOG_DEBUG << "EditProductForm: Product " << id << " not found in DB";
                callback(HttpResponse::newNotFoundResponse());
                return;
            }

            std::map<std::string, std::string> product;
            product["id"] = r[0]["id"].as<std::string>();
            product["title"] = r[0]["title"].as<std::string>();
            product["description"] = r[0]["description"].isNull() ? "" : r[0]["description"].as<std::string>();
            product["price"] = r[0]["price"].as<std::string>();
            product["image_url"] = r[0]["image_url"].isNull() ? "" : r[0]["image_url"].as<std::string>();
            product["stock"] = r[0]["stock"].as<std::string>();
            product["category_id"] = r[0]["category_id"].isNull() ? "0" : r[0]["category_id"].as<std::string>();

            dbClient->execSqlAsync(
                "SELECT id, name FROM categories ORDER BY name ASC",
                [callback, request, product = std::move(product), dbClient](const drogon::orm::Result& cat_res) {
                    
                    dbClient->execSqlAsync(
                        "SELECT id, image_url, is_primary FROM product_images WHERE product_id = $1 ORDER BY is_primary DESC, id ASC",
                        [callback, request, product, cat_res](const drogon::orm::Result& img_res) mutable {
                            HttpViewData data;
                            data.insert("csrf_token", request->session()->get<std::string>("csrf_token"));
                            data.insert("product", product);

                            std::vector<std::map<std::string, std::string>> categories;
                            for (const auto& row : cat_res) {
                                std::map<std::string, std::string> cat;
                                cat["id"] = row["id"].as<std::string>();
                                cat["name"] = row["name"].as<std::string>();
                                categories.push_back(cat);
                            }
                            data.insert("categories_list", categories);

                            std::vector<std::map<std::string, std::string>> images;
                            for (const auto& row : img_res) {
                                std::map<std::string, std::string> img;
                                img["id"] = row["id"].as<std::string>();
                                img["image_url"] = row["image_url"].as<std::string>();
                                img["is_primary"] = row["is_primary"].as<bool>() ? "1" : "0";
                                images.push_back(img);
                            }
                            data.insert("product_images", images);

                            auto resp = HttpResponse::newHttpViewResponse("EditProduct", data);
                            callback(resp);
                        },
                        [callback](const drogon::orm::DrogonDbException& e) {
                            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                        },
                        product.at("id")
                    );
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                }
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        id
    );
}

void ProductController::updateProduct(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    int productId = id;
    MultiPartParser fileUpload;
    if (fileUpload.parse(request) != 0) {
        callback(HttpResponse::newHttpJsonResponse(Json::Value("Xato: Ma'lumotlarni o'qib bo'lmadi")));
        return;
    }

    if (!validateMultipartCsrf(fileUpload, request)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto params = fileUpload.getParameters();
    auto &files = fileUpload.getFiles();
    auto dbClient = drogon::app().getDbClient("default");
    
    dbClient->execSqlAsync(
        "UPDATE products SET title=$1, description=$2, price=$3, stock=$4, category_id=$5 WHERE id=$6",
        [callback, files, dbClient, productId](const drogon::orm::Result& r) {
            for (size_t i = 0; i < files.size(); ++i) {
                auto &file = files[i];
                std::string fileName = file.getFileName();
                std::string fileExtension = std::filesystem::path(fileName).extension().string();
                std::string newFileName = drogon::utils::getUuid() + fileExtension;
                file.saveAs(newFileName);
                
                std::string dbImageUrl = "/uploads/" + newFileName;
                dbClient->execSqlAsync(
                    "INSERT INTO product_images (product_id, image_url, is_primary) VALUES ($1, $2, FALSE)",
                    [](const drogon::orm::Result& ir) {},
                    [](const drogon::orm::DrogonDbException& e) {},
                    productId, dbImageUrl
                );
            }
            auto resp = HttpResponse::newRedirectionResponse("/admin/dashboard");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        params["title"], params["description"], params["price"], params["stock"], params["category_id"], productId
    );
}

void ProductController::deleteImage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    int imageId = id;
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT product_id FROM product_images WHERE id = $1",
        [callback, dbClient, imageId](const drogon::orm::Result& r) {
            if (r.empty()) {
                LOG_DEBUG << "DeleteImage: Image " << imageId << " not found";
                callback(HttpResponse::newNotFoundResponse());
                return;
            }
            int productId = r[0]["product_id"].as<int>();
            dbClient->execSqlAsync(
                "DELETE FROM product_images WHERE id = $1",
                [callback, productId](const drogon::orm::Result& dr) {
                    auto resp = HttpResponse::newRedirectionResponse("/admin/product/edit/" + std::to_string(productId));
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                },
                imageId
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        imageId
    );
}

void ProductController::setPrimaryImage(const HttpRequestPtr& request, std::function<void(const HttpResponsePtr&)>&& callback, int id) {
    int imageId = id;
    auto dbClient = drogon::app().getDbClient("default");
    dbClient->execSqlAsync(
        "SELECT product_id FROM product_images WHERE id = $1",
        [callback, dbClient, imageId](const drogon::orm::Result& r) {
            if (r.empty()) {
                LOG_DEBUG << "SetPrimaryImage: Image " << imageId << " not found";
                callback(HttpResponse::newNotFoundResponse());
                return;
            }
            int productId = r[0]["product_id"].as<int>();
            
            dbClient->execSqlAsync(
                "UPDATE product_images SET is_primary = FALSE WHERE product_id = $1",
                [callback, dbClient, imageId, productId](const drogon::orm::Result& ur) {
                    dbClient->execSqlAsync(
                        "UPDATE product_images SET is_primary = TRUE WHERE id = $1",
                        [callback, productId](const drogon::orm::Result& ur2) {
                            auto resp = HttpResponse::newRedirectionResponse("/admin/product/edit/" + std::to_string(productId));
                            callback(resp);
                        },
                        [callback](const drogon::orm::DrogonDbException& e) {
                            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                        },
                        imageId
                    );
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
                },
                productId
            );
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            callback(HttpResponse::newHttpJsonResponse(Json::Value(e.base().what())));
        },
        imageId
    );
}
