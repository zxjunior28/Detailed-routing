# Detailed routing
## Implement a 2-layer detailed router to complete channel routing problems

## Build with Makefile directly 
```console
$ make clean
$ make
$ ./Lab4 "data/case1.txt" "data/ans/output_case1.txt"
```

## Build with CMake by scripts
```console
$ source scripts/boost.sh "data/case1.txt" "data/ans/output_case1.txt"
```

## Build with CMake directly 
```console
$ rm -rf build/
$ cmake -S . -B build/ -DBUILD_EXAMPLES=ON
$ cmake --build build/ -j4
$ ./build/bin/Lab4 "data/case1.txt" "data/ans/output_case1.txt"
```
