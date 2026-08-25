cmake_minimum_required(VERSION 3.14)
project(Belit_Lifter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# BAĞIMLILIKLAR (LLVM 18 & Z3 SMT)
# ==========================================
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Z3 REQUIRED)
include_directories(${Z3_INCLUDE_DIRS})

include_directories(include)

# ==========================================
# MILESTONE 1: EVM to LLVM IR Lifter Motoru
# ==========================================
add_library(EvmLifter STATIC src/evm_lifter/EvmToIR.cpp)
target_link_libraries(EvmLifter PRIVATE LLVMSupport LLVMCore)

# Unit & E2E Test Executable
add_executable(BelitTests tests/unit_tests/LifterTest.cpp)
target_link_libraries(BelitTests PRIVATE 
    EvmLifter 
    gtest 
    gtest_main 
    pthread
    LLVMSupport 
    LLVMCore 
    LLVMIRReader
)

# ==========================================
# MILESTONE 2: Deobfuscation Pass Plugin
# ==========================================
add_library(DeobfuscationPass MODULE src/llvm_passes/DeobfuscationPass.cpp)
set_target_properties(DeobfuscationPass PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(DeobfuscationPass PRIVATE ${Z3_LIBRARIES})

if(APPLE)
    set_property(TARGET DeobfuscationPass APPEND_STRING PROPERTY LINK_FLAGS "-undefined dynamic_lookup")
endif()

# ==========================================
# MILESTONE 3 & 4: Z3 SMT Formal Verification
# ==========================================
add_library(Z3Verifier STATIC src/smt_solver/Z3Verifier.cpp)
target_link_libraries(Z3Verifier PRIVATE ${Z3_LIBRARIES} LLVMCore)

target_sources(BelitTests PRIVATE 
    tests/unit_tests/Z3VerifierTest.cpp
    tests/e2e_tests/EndToEndReentrancyTest.cpp
)
target_link_libraries(BelitTests PRIVATE Z3Verifier ${Z3_LIBRARIES})

# ==========================================
# CLI ENTRY POINT (belit_cli)
# ==========================================
add_executable(belit_cli src/main.cpp)
target_link_libraries(belit_cli PRIVATE 
    EvmLifter 
    Z3Verifier 
    ${Z3_LIBRARIES} 
    LLVMSupport 
    LLVMCore 
    LLVMIRReader
)