cmake_minimum_required(VERSION 3.16)
project(PasswordManager)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

add_executable(PasswordManager
    src/main.cpp
    src/PasswordGenerator.cpp
    src/utilities.cpp
    src/HashManager.cpp
    src/EncryptionManager.cpp
)

target_include_directories(PasswordManager PRIVATE include)
target_link_libraries(PasswordManager OpenSSL::SSL OpenSSL::Crypto)
