#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};

fail()
{
    echo >&2 '
***************
*** failed ***
***************
'
    echo "An error occurred. Exiting..." >&2
    exit 1
}
#set -e
trap 'abort' 0

t=$(readlink -f "${BASH_SOURCE[0]}"); scriptName=$(basename "$t"); unset t;
appServerDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo $scriptName clean=$clean shouldFetch=$shouldFetch
function myFetch
{
	pushd `pwd`> /dev/null;
	dir=$1;
	if test ! -d $dir; then git clone https://github.com/Jde-cpp/$dir.git; else cd $dir; if (( $shouldFetch == 1 )); then git pull; cd ..; fi; fi;
	popd> /dev/null;
}
cd ..;
myFetch Framework
if [[ -z $sourceBuild ]]; then source Framework/source-build.sh; fi;
if [ ! windows ]; then set disable-completion on; fi;
Framework/framework-build.sh $clean $shouldFetch; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
if windows; then fetchBuild Odbc 0 Jde.DB.Odbc.dll; fi;

cd $appServerDir/source; moveToDir types; moveToDir proto;
findProtoc;
dir=$JDE_BASH/public/jde/log/types/proto
protoDir=$appServerDir/source/types/proto
function appProto
{
	file=$1
	dest=$protoDir/$file.pb.cc;
	if test ! -f $dest; then
		createProto $dir $file;
		mv $dir/$file.pb.cc $protoDir/$file.pb.cc
		cp $dir/$file.pb.h $protoDir/$file.pb.h
	fi;
}

appProto FromServer
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ApplicationDefaultTypeInternal _Application_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ApplicationDefaultTypeInternal _Application_default_instance_;/' $protoDir/$file.pb.cc;
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StatusDefaultTypeInternal _Status_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StatusDefaultTypeInternal _Status_default_instance_;/' $protoDir/$file.pb.cc;
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ApplicationStringDefaultTypeInternal _ApplicationString_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ApplicationStringDefaultTypeInternal _ApplicationString_default_instance_;/' $protoDir/$file.pb.cc;
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ErrorMessageDefaultTypeInternal _ErrorMessage_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ErrorMessageDefaultTypeInternal _ErrorMessage_default_instance_;/' $protoDir/$file.pb.cc;
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' $protoDir/$file.pb.cc;
appProto FromClient;
echo x=$protoDir/$file.pb.cc;
sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' $protoDir/$file.pb.cc;


cd $appServerDir/source;
build AppServer 0 Jde.AppServer.exe;
cd $appServerDir/tests;
build Tests.AppServer 0 Jde.AppServer.exe;

trap : 0

echo >&2 '
************
*** DONE ***
************
'