rm -rf build/
cmake -S . -B build/ -DBUILD_EXAMPLES=ON
cmake --build build/ -j4
./build/bin/Lab4 $@ $@
# "data/case0.txt"
