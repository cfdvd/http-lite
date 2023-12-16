clear

/usr/bin/cmake -B "build" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON

cmake --build "build" --target "all" --config "Debug" --parallel 

echo ""
cd ./build
chmod +x test
clear
./test 

#chmod +x cmake.sh
#chmod +x link.sh
#chmod +x run.sh
#clear
#./cmake.sh && ./link.sh && ./run.sh
