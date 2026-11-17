mkdir -p build
clang++ puyo.cpp -o "build/${1:-puyo}" -lncurses