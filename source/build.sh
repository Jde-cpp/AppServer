#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
compiler=${4:-g++-13}

export CXX=$compiler;
BUILD=$JDE_DIR/Public/build/so.sh;
pushd `pwd` > /dev/null;
cd ../..;
if [ $all -eq 1 ]; then
	$BUILD Framework/source $type $clean $compiler || exit 1;
	cd Public/libs;
#	$BUILD ../../MySql/source $type $clean $compiler || exit 1;
	$BUILD crypto/src $type $clean $compiler || exit 1;
	$BUILD db/src $type $clean $compiler || exit 1;
	$BUILD db/src/drivers/mysql $type $clean $compiler || exit 1;
	$BUILD ql $type $clean $compiler || exit 1;
	$BUILD access/src $type $clean $compiler || exit 1;
	$BUILD web/client $typekw$clean $compiler || exit 1;
	$BUILD web/server $type $clean $compiler || exit 1;
	$BUILD app/shared $type $clean $compiler || exit 1;
	$BUILD app/client $type $clean $compiler || exit 1;
fi
popd  > /dev/null;
#$BUILD ../tests $type $clean $compiler || exit 1;
$BUILD `pwd` $type $clean $compiler || exit 1;
$BUILD .. $type $clean $compiler || exit 1;

exit $?;