#include <drogon/drogon.h>
#include <iostream>

int main() {
    try {
        // 1. Loglash darajasini o'rnatish (xatolarni ko'rish uchun yordam beradi)
        trantor::Logger::setLogLevel(trantor::Logger::kDebug);

        // 2. config.json yuklash
        // Maslahat: Fayl yo'li to'g'riligini tekshiring
        drogon::app().loadConfigFile("config.json");

        // 3. Portni manual qo'shish (agar config.json da bo'lmasa)
        drogon::app().addListener("127.0.0.1", 8848);

        LOG_INFO << "JobEasy Server 8848-portda ishga tushmoqda...";

        // 4. Serverni yurgizish
        drogon::app().run();
    }
    catch (const std::exception& e) {
        std::cerr << "\n--- JIDDIY XATOLIK ---" << std::endl;
        std::cerr << "Xabar: " << e.what() << std::endl;
        std::cerr << "----------------------" << std::endl;
        return 1;
    }

    return 0;
}