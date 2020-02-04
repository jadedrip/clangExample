cmake_minimum_required (VERSION 3.12)
project (CLangToLLVM)

find_package(Clang REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

LINK_LIBRARIES(LLVMDemangle LLVMProfileData LLVMBitstreamReader clangDriver LLVMRemarks LLVMOption LLVMMC LLVMMCParser clangRewrite clangAST clangEdit clangParse clangAnalysis clangSema clangBasic clangFrontend clangLex clangSerialization clangCodeGen clangASTMatchers LLVMObject LLVMCoverage LLVMBitReader LLVMAnalysis LLVMTransformUtils LLVMCore LLVMSupport LLVMBinaryFormat LLVMObjectYAML)
add_executable (CLangToLLVM CLangParser.cpp )
