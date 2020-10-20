CXX=$(HOME)/cheri/output/sdk/bin/clang++
CHERI_CXX=$(HOME)/cheri/output/sdk/bin/riscv64-unknown-freebsd13-clang++
CHERI_FLAGS=-march=rv64imafdcxcheri -mabi=l64pc128d --sysroot=$(HOME)/cheri/output/rootfs-riscv64-hybrid -mno-relax

export 

all: main

main: main.cpp
	$(CXX) -ggdb --std=c++20 -O3 main.cpp -D NDEBUG -o main

debug:
	$(CXX) -ggdb --std=c++20 -O0 main.cpp -o main

cheri:
	$(CHERI_CXX) -ggdb $(CHERI_FLAGS) --std=c++20 -O0 main.cpp -o main

bt:
	$(CXX) -ggdb --std=c++20 -O0 binary_trees.cpp -o binary_trees

bt-opt:
	$(CXX) -ggdb --std=c++20 -O3 binary_trees.cpp -D NDEBUG -o binary_trees

bt-cheri-opt:
	$(CHERI_CXX) -ggdb $(CHERI_FLAGS) --std=c++20 -O3 binary_trees.cpp -D NDEBUG -o binary_trees
