include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wcpp Has_warn)


add_executable(main main.cxx FineTimeMC.cxx )
target_link_libraries(main PUBLIC ROOTlib fmt::fmt range-v3::range-v3 cxxopts::cxxopts)
if(Has_warn)
    target_compile_options(main PRIVATE -Wno-cpp)
endif()
