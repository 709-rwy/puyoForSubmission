param(
    [string]$Name = "puyo"
)

mkdir build -Force | Out-Null
g++ puyo.cpp -o "build/$Name.exe" -lncurses