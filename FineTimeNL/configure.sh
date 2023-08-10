conan profile detect --force
conan install . --output-folder=build/conanPkg/ --build=missing --settings=build_type=RelWithDebInfo
cmake --fresh -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=conanPkg/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
