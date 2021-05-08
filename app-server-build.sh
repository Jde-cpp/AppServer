clean=${1:-0};
fetch=${2:-1};
set disable-completion on;
#windows() { [[ -n "$WINDIR" ]]; }
source ./common.sh
source ./source-build.sh

fetch framework-build.sh
file=framework-build.sh;
if [ ! -f $file ]; then
	if [ -f jde/Framework/$file ]; then
		if windows; then
			dest=`pwd`;
			dest=${dest////\\};
			dest=${dest/\\c/c:};
			cmd <<< "mklink $dest\\$file $dest\\jde\\Framework\\$file" > /dev/null;
		else
			ln -s jde/Framework/$file .;
		fi;
	else
		curl https://raw.githubusercontent.com/Jde-cpp/Framework/master/$file -o $file
	fi;
fi

function appServerProtoc
{
    createProto types/proto FromServer;
    createProto types/proto FromClient;
}

source $file $clean $fetch;
build Odbc 0
build AppServer 0 'appServerProtoc'
