#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}

export CXX=$compiler;
BUILD=../../Public/build/so.sh;
if [ $all -eq 1 ]; then
	$BUILD ../../Framework/source $type $clean $compiler || exit 1;
	$BUILD ../../MySql/source $type $clean $compiler || exit 1;
	$BUILD ../../Ssl/source $type $clean $compiler || exit 1;
	$BUILD ../../XZ/source $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/web $type $clean $compiler || exit 1;
fi
if [ ! -d .obj ];	then
	mkdir .obj;
	clean=1;
fi;
cd .obj;
if [ ! -d $type ]; then
	mkdir $type;
	clean=1;
fi;
cd $type;
if (( $clean == 1 )) || [ ! -f CMakeCache.txt ]; then
	rm -f CMakeCache.txt;
	cmake -DCMAKE_CXX_COMPILER=$compiler -DCMAKE_BUILD_TYPE=$type  ../.. > /dev/null;
	make clean;
fi
make -j;
cd - > /dev/null;
exit $?;