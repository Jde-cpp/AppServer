#include <span>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <jde/TypeDefs.h>
#include <jde/log/Log.h>
#include <jde/Exception.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/app/shared/StringCache.h>
#include <jde/web/server/SessionGraphQL.h>
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/um/UM.h"
#include "graphQL/AppInstanceHook.h"
#include "ExternalLogger.h"
#include "LogData.h"
#include "Server.h"


#define var const auto

#ifndef _MSC_VER
namespace Jde{ α OSApp::CompanyName()ι->string{ return "Jde-Cpp"; } }
namespace Jde{ α OSApp::ProductName()ι->sv{ return "AppServer"; } }
#endif

namespace Jde::App{
	α Startup()ι->void;
}

int main( int argc, char** argv ){
	using namespace Jde;
	optional<int> exitCode;
	try{
		OSApp::Startup( argc, argv, "Jde.AppServer", "jde-cpp App Server." );
		IApplication::AddThread( ms<Threading::InterruptibleThread>("Startup", [&](){App::Startup();}) );
		exitCode = IApplication::Pause();
	}
	catch( const IException& e ){
		std::cerr << (e.Level()==ELogLevel::Trace ? "Exiting..." : "Exiting on error:  ") << e.what() << std::endl;
	}
	Process::Shutdown( exitCode.value_or(EXIT_FAILURE) );
	return exitCode.value_or( EXIT_FAILURE );
}

namespace Jde{
	α App::Startup()ι->void{
		try{
/*				//if( Settings::TryGet<bool>("um/use").value_or(false) ) currently need to configure um so meta is loaded.
			{
				UM::Configure();
			}*/
			Server::ConfigureDatasource();
			var [appId, appInstanceId, dbLogLevel, fileLogLevel] = AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
			SetAppPK( appId );
			SetInstancePK( appInstanceId );

			Data::LoadStrings();
			StartWebServer();
			DB::GraphQL::Hook::Add( mu<AppInstanceHook>() );
			DB::GraphQL::Hook::Add( mu<Web::Server::SessionGraphQL>() );
			Logging::External::Add( mu<ExternalLogger>() );
		}
		catch( IException& e ){
			Process::Shutdown( e.Code==0 ? -1 : e.Code );
			return;
		}
		Information( ELogTags::App, "--AppServer Started.--" );
		IApplication::RemoveThread( "Startup" )->Detach();
	}
}