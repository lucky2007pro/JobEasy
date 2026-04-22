cmake_minimum_required(VERSION 3.10) # 3.5 dan 3.10 ga ko'tardik (zamonaviy funksiyalar uchun)

project(JobEasy CXX)

# C++ Standartini aniqlash (Drogon uchun kamida C++17 shart)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. Kerakli paketlarni topamiz
# Visual Studio vcpkg ni avtomatik ulaydi, shuning uchun toolchain yozish shartmas
find_package(Drogon CONFIG REQUIRED)

# 2. Papkalarni skanerlash
# aux_source_directory yangi fayl qo'shganda CMake-ni qayta ishga tushirishni talab qilishi mumkin
aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(models MODEL_SRC)

# 3. Asosiy executable faylni yaratamiz
# Bu yerga faqat main.cc ni qo'yamiz, qolganlarini target_sources orqali qo'shamiz
add_executable(${PROJECT_NAME} main.cc)

# 4. CSP (Views) fayllarini C++ kodiga o'giramiz
# CSP fayllari o'zgarganda avtomatik qayta generatsiya bo'ladi
drogon_create_views(${PROJECT_NAME} 
                    ${CMAKE_CURRENT_SOURCE_DIR}/views 
                    ${CMAKE_CURRENT_BINARY_DIR} 
                    FALSE 
                    VIEW_SRC)

# 5. Barcha to'plangan manba fayllarni loyihaga qo'shamiz
target_sources(${PROJECT_NAME}
               PRIVATE
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}
               ${VIEW_SRC})

# 6. Kutubxonalarni bog'laymiz
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

if(MSVC)
    # Suppress external header warning promoted as error in Visual Studio.
    target_compile_options(${PROJECT_NAME} PRIVATE /wd26819)
endif()

# 7. Include yo'llarini ko'rsatamiz
target_include_directories(${PROJECT_NAME}
                           PRIVATE 
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/controllers
                           ${CMAKE_CURRENT_SOURCE_DIR}/filters
                           ${CMAKE_CURRENT_SOURCE_DIR}/models
                           ${CMAKE_CURRENT_BINARY_DIR}) # Generatsiya bo'lgan .h fayllar (views) uchun

# 8. config.json faylini build papkasiga nusxalash
# Dastur ishlaganda config.json-ni topishi uchun kerak
file(COPY config.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Test papkasi (agar mavjud bo'lsa)
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_subdirectory(test)
endif()