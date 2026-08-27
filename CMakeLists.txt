# CMakeLists.txt — native_banks.dll (wariant na NH3API), Win32/x86 plugin HD Moda.
#
# Build cross na Linuxie (mingw-w64 i686 — GCC):
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake
#   cmake --build build
#
# NH3API wspiera te C++17: MSVC 19.14+, MinGW GCC 9+, llvm-mingw (rekomendowane
# przez autora). Tu domyslnie GCC mingw jak w repo bazowym.
cmake_minimum_required(VERSION 3.16)
project(native_banks_nh3api CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(native_banks SHARED
    src/main.cpp
    src/mapping.cpp
    src/config.cpp
    src/log.cpp
    src/version.rc
)

target_include_directories(native_banks PRIVATE
    src
    third_party/NH3API            # nh3api/core/... (patcher_x86.hpp itd.)
)

set_target_properties(native_banks PROPERTIES
    PREFIX ""
    OUTPUT_NAME "native_banks"
)

if(MSVC)
    set_property(TARGET native_banks PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_compile_options(native_banks PRIVATE /W4 /EHsc)
else()
    target_compile_options(native_banks PRIVATE -Wall -Wextra -m32)
    target_link_options(native_banks PRIVATE
        -m32 -static -static-libgcc -static-libstdc++
        -Wl,--enable-stdcall-fixup -s)
endif()
