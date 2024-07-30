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
#	$BUILD ../../Ssl/source $type $clean $compiler || exit 1;
#	$BUILD ../../XZ/source $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/crypto $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/web/client $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/web/server $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/app/shared $type $clean $compiler || exit 1;
	$BUILD ../../Public/src/app/client $type $clean $compiler || exit 1;
	$BUILD `pwd` $type $clean $compiler || exit 1;
	$BUILD .. $type $clean $compiler || exit 1;
	$BUILD ../tests $type $clean $compiler || exit 1;
fi
exit $?;