#include "AppStartupAwait.h"
#include "WebServer.h"
#include "graphQL/AppInstanceHook.h"
#include <jde/web/server/SettingQL.h>
#include "ExternalLogger.h"
#include <jde/web/server/SessionGraphQL.h>

#define let const auto
namespace Jde::App{
	α AppStartupAwait::Execute()ι->VoidAwait<>::Task{
		try{
			co_await Server::ConfigureDSAwait{};
			let [appId, appInstanceId] = AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
			Server::SetAppPK( appId );
			IApplicationServer::SetInstancePK( appInstanceId );

			Data::LoadStrings();
			Server::StartWebServer( move(_webServerSettings) );
			QL::Hook::Add( mu<AppInstanceHook>() );
			QL::Hook::Add( mu<Web::Server::SettingQL>() );
			QL::Hook::Add( mu<Web::Server::SessionGraphQL>() );
			Logging::External::Add( mu<ExternalLogger>() );

			Information( ELogTags::App, "--AppServer Started.--" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
			//OSApp::UnPause();
		}
	}
}