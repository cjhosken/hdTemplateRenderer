mkdir build
cd build

cmake .. -DCMAKE_PREFIX_PATH=../lib/usd
cmake --build . 
cmake --install . --prefix=..