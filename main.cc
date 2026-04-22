#include <drogon/drogon.h>
#include <iostream>
#include <filesystem>
#include <optional>

namespace
{
std::optional<std::filesystem::path> findConfigPath()
{
    namespace fs = std::filesystem;
    fs::path current = fs::current_path();

    for (int depth = 0; depth < 8; ++depth)
    {
        const auto direct = current / "config.json";
        if (fs::exists(direct))
        {
            return fs::weakly_canonical(direct);
        }

        const auto inJobEasyDir = current / "JobEasy" / "config.json";
        if (fs::exists(inJobEasyDir))
        {
            return fs::weakly_canonical(inJobEasyDir);
        }

        if (!current.has_parent_path() || current.parent_path() == current)
        {
            break;
        }
        current = current.parent_path();
    }

    return std::nullopt;
}
}  // namespace

int main() {
    try {
        // 1. Loglash darajasini o'rnatish (xatolarni ko'rish uchun yordam beradi)
        trantor::Logger::setLogLevel(trantor::Logger::kDebug);

        // 2. config.json ni ishga tushirish papkasidan mustaqil topib yuklash
        const auto configPath = findConfigPath();
        if (!configPath)
        {
            throw std::runtime_error("config.json topilmadi");
        }
        drogon::app().loadConfigFile(configPath->string());
        LOG_INFO << "Config yuklandi: " << configPath->string();

        // 3. Portni manual qo'shish o'rniga config.json dan yuklanadi (Production uchun mos)
        LOG_INFO << "JobEasy Server ishga tushmoqda...";

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
