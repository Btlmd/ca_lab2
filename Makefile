all: read mm 

read: read.cpp
	mkdir -p build
	g++ -O3 -o build/read -Wextra read.cpp
	build/read 

mm: mm.cpp
	mkdir -p build
	g++ -O0 -o build/mm mm.cpp
	build/mm