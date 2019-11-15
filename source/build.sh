#!/bin/bash
debug=${1:-1}
all=${2:-1}
clean=${3:-0}

if [ $all -eq 1 ]; then
	../../Framework/source/build.sh $debug $clean
	if [ $? -eq 1 ]; then
		exit 1
	fi
	../../MySql/source/build.sh $debug $clean
	if [ $? -eq 1 ]; then
		exit 1
	fi
fi
if [ $clean -eq 1 ]; then
	make clean DEBUG=$debug
	if [ $debug -eq 1 ]; then
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-switch   -O0 ./pc.h -o .obj/debug/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/boostorg/boost_1_68_0 -I/home/duffyj/code/libraries/json/include  -fsanitize=address -fno-omit-frame-pointer
	else
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-switch  -DNDEBUG -O3 -I.release ./pc.h -o .obj/release/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/boostorg/boost_1_68_0 -I/home/duffyj/code/libraries/json/include
	fi
	if [ $? -eq 1 ]; then
		exit 1
	fi
fi
make -j7 DEBUG=$debug
if [ $? -eq 1 ]; then
	exit 1
fi
