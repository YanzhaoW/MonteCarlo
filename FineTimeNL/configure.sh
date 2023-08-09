conan install . --output-folder=build/conanPkg/ --build=missing
cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=conanPkg/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
