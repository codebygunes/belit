cmake_minimum_required(VERSION 3.20)
project(BelitVerificationEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required for building shared modules/plugins (.so)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/W0)
else()
    add_compile_options(-w)
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Z3 REQUIRED)
message(STATUS "Found Z3 ${Z3_VERSION_STRING}")

include_directories(include)

# ==========================================
# Core Library Configuration
# ==========================================
set(BELIT_SOURCES
    src/targets/EVMLifter.cpp
    src/targets/WasmLifter.cpp
    src/ir/LLVMTranslator.cpp
    src/llvm_passes/DeobfuscationPass.cpp
    src/verifier/Z3SymbolicEngine.cpp
)

add_library(belit_core STATIC ${BELIT_SOURCES})

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    Analysis
    TransformUtils
    Passes
)

target_link_libraries(belit_core PUBLIC
    ${LLVM_LIBS}
    z3
)

# ==========================================
# LLVM Pass Plugin
# ==========================================
add_library(BelitPassPlugin MODULE
    src/llvm_passes/BelitPassPlugin.cpp
)
target_link_libraries(BelitPassPlugin PRIVATE belit_core)

# ==========================================
# Main CLI Executable Configuration
# ==========================================
add_executable(belit src/main.cpp)
target_link_libraries(belit PRIVATE belit_core)

# ==========================================
# Unit & Benchmark Tests Configuration
# ==========================================
enable_testing()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(TEST_SOURCES
    tests/test_main.cpp
    tests/unit_tests/LifterTest.cpp
    tests/unit_tests/Z3SymbolicEngineTest.cpp
    tests/unit_tests/DeobfuscationBenchmarkTest.cpp
    tests/e2e_tests/EndToEndVulnerabilityTest.cpp
)

add_executable(belit_tests ${TEST_SOURCES})

target_link_libraries(belit_tests PRIVATE
    belit_core
    ${GTEST_LIBRARIES}
    pthread
)

include(GoogleTest)
gtest_discover_tests(belit_tests)