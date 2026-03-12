cmake_minimum_required(VERSION 3.22.1)
project("fclient")

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Импортируем spdlog как статическую библиотеку
add_library(spdlog STATIC IMPORTED)
# Используем полный путь к файлу для конкретной ABI (x86 в вашем случае)
set_target_properties(spdlog PROPERTIES IMPORTED_LOCATION
    "C:/Users/yan/AndroidStudioProjects/rpo/libs/spdlog/build/${ANDROID_ABI}/libspdlog.a")

# Создаём shared-библиотеку для приложения
add_library(${PROJECT_NAME} SHARED
    native-lib.cpp)

add_library(mbedcrypto SHARED IMPORTED)

set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION
    "C:/Users/yan/AndroidStudioProjects/rpo/libs/mbedtls/build/${ANDROID_ABI}/library/libmbedcrypto.so")


# Подключаем заголовки spdlog
target_include_directories(${PROJECT_NAME} PUBLIC
    "C:/Users/yan/AndroidStudioProjects/rpo/libs/spdlog/spdlog/include"
    "C:/Users/yan/AndroidStudioProjects/rpo/libs/mbedtls/mbedtls/include"
    )


# Линкуем с системными библиотеками и spdlog
target_link_libraries(${PROJECT_NAME}
    android
    log
    mbedcrypto
    spdlog)