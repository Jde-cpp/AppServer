#include <jde/framework.h>
DISABLE_WARNINGS
#include <span>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <jde/web/client/exports.h>
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include <jde/app/shared/exports.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
ENABLE_WARNINGS
#include <jde/app/shared/StringCache.h>
#include <jde/web/server/SessionGraphQL.h>
#include "graphQL/AppInstanceHook.h"
#include "ExternalLogger.h"
#include "LogData.h"
#include "WebServer.h"

#define let const auto

#ifndef _MSC_VER
namespace Jde{ α OSApp::CompanyName()ι->string{ return "Jde-Cpp"; } }
namespace Jde{ α OSApp::ProductName()ι->sv{ return "AppServer"; } }
#endif

namespace Jde::App{
	α Startup()ι->Server::ConfigureDSAwait::Task;
}

int main( int argc, char** argv ){
	using namespace Jde;
	optional<int> exitCode;
	try{
		OSApp::Startup( argc, argv, "Jde.AppServer", "jde-cpp App Server." );
		App::Startup();
		exitCode = IApplication::Pause();
	}
	catch( const IException& e ){
		std::cerr << (e.Level()==ELogLevel::Trace ? "Exiting..." : "Exiting on error:  ") << e.what() << std::endl;
	}
	Process::Shutdown( exitCode.value_or(EXIT_FAILURE) );
	return exitCode.value_or( EXIT_FAILURE );
}

namespace Jde{
	α App::Startup()ι->Server::ConfigureDSAwait::Task{
		try{
			co_await Server::ConfigureDSAwait{};
			let [appId, appInstanceId] = AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
			Server::SetAppPK( appId );
			Server::SetInstancePK( appInstanceId );

			Data::LoadStrings();
			Server::StartWebServer();
			QL::Hook::Add( mu<AppInstanceHook>() );
			QL::Hook::Add( mu<Web::Server::SessionGraphQL>() );
			Logging::External::Add( mu<ExternalLogger>() );

			Information( ELogTags::App, "--AppServer Started.--" );
		}
		catch( IException& e ){
			e.Log();
			OSApp::UnPause();
		}
	}
}