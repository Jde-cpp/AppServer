#include <jde/framework.h>
#include <jde/app/shared/exports.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include "AppStartupAwait.h"

namespace Jde{
#ifndef _MSC_VER
	α Process::CompanyName()ι->string{ return "Jde-Cpp"; }
	α Process::ProductName()ι->sv{ return "AppServer"; }
#endif

	α startup( int argc, char** argv )ε->void{
		OSApp::Startup( argc, argv, "Jde.AppServer", "jde-cpp App Server." );
		auto settings = Settings::FindObject( "http" );
		BlockVoidAwait<App::AppStartupAwait>( App::AppStartupAwait{settings ? move(*settings) : jobject{}} );
	}
}

α main( int argc, char** argv )->int{
	using namespace Jde;
	optional<int> exitCode;
	try{
		startup( argc, argv );
		exitCode = Process::Pause();
	}
	catch( const IException& e ){
		exitCode = e.Code;
		std::cerr << (e.Level()==ELogLevel::Trace ? "Exiting..." : "Exiting on error:  ") << e.what() << std::endl;
	}
	Process::Shutdown( exitCode.value_or(EXIT_FAILURE) );
	return exitCode.value_or( EXIT_FAILURE );
}