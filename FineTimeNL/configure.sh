conan profile detect --force
conanfolder=conanPkg
conan install . --output-folder=build/${conanfolder} --build=missing --settings=build_type=Debug
source build/${conanfolder}/conanbuild.sh
cmake --fresh -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=${conanfolder}/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
