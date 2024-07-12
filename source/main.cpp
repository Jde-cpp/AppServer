#include <jde/appClient/SessionGraphQL.h>
#include <jde/appClient/AppClient.h>
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/um/UM.h"
#include "Cache.h"
#include "ExternalLogger.h"
#include "LogData.h"
#include "Server.h"


#define var const auto

#ifndef _MSC_VER
namespace Jde{ α OSApp::CompanyName()ι->string{ return "Jde-Cpp"; } }
namespace Jde{ α OSApp::ProductName()ι->sv{ return "AppServer"sv; } }
#endif

namespace Jde::App{
	α Startup()ι->void;
}

int main( int argc, char** argv ){
	using namespace Jde;
	try{
		OSApp::Startup( argc, argv, "Jde.AppServer", "jde-cpp App Server." );
		IApplication::AddThread( ms<Threading::InterruptibleThread>("Startup", [&](){App::Startup();}) );
		IApplication::Pause();
	}
	catch( const IException& e ){
		std::cout << (e.Level()==ELogLevel::Trace ? "Exiting..." : "Exiting on error:  ") << e.what() << std::endl;
	}
	IApplication::Cleanup();
	return EXIT_SUCCESS;
}

namespace Jde{
	α App::Startup()ι->void{
		try{
				//if( Settings::TryGet<bool>("um/use").value_or(false) ) currently need to configure um so meta is loaded.
			{
				UM::Configure();
			}
			SetDataSource( DB::DataSourcePtr() );
		}
		catch( IException& e ){
			std::cerr << e.what() << std::endl;
			{auto e2=e.Move();}//destructor log.
			std::this_thread::sleep_for( 1s );
			std::terminate();
		}
		var [appId, appInstanceId, dbLogLevel, fileLogLevel] = AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
		SetAppId( appId );
		SetInstanceId( appInstanceId );

		Cache::Load();
		StartWebServer();
		DB::GraphQL::Hook::Add( mu<Web::SessionGraphQL>() );
		Logging::External::Add( up<ExternalLogger>() );

		INFOT( AppTag(), "--AppServer Started.--" );
		IApplication::RemoveThread( "Startup" )->Detach();
	}
}