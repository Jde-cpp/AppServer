#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};
#buildPrivate=${3:-0};

t=$(readlink -f "${BASH_SOURCE[0]}"); scriptName=$(basename "$t"); unset t;
appServerDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo $scriptName clean=$clean shouldFetch=$shouldFetch #buildPrivate=$buildPrivate
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
#echo `pwd`;
Framework/framework-build.sh $clean $shouldFetch; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
if windows; then fetchBuild Odbc 0 Jde.DB.Odbc.dll; fi;

cd $appServerDir/source;
#echo `pwd`;
findProtoc;
createProto types/proto FromServer;
createProto types/proto FromClient;
build AppServer 0 Jde.AppServer.exe;
cd $appServerDir/tests;
build Tests.AppServer 0 Jde.AppServer.exe;

