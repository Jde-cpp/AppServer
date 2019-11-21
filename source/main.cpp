#include "stdafx.h"
#include "Listener.h"
//#include "EtwListener.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"

#ifndef _MSC_VER
	#include "../../Framework/source/application/ApplicationLinux.h"
#endif

#define var const auto

namespace Jde::ApplicationServer
{
	std::shared_ptr<Jde::Settings::Container> SettingsPtr;
	int Run();
}
int main( int argc, char** argv )
{
	Jde::OSApp::Startup( argc, argv, "AppServer" );
	//auto result = console ? 0 : ::daemon( 1, 0 );
	//Jde::Threading::SetThreadDescription( "app-server" );
	//INFO( "Running as console='{}' daemon='{}'", console, result );
	//Jde::Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(std::filesystem::path("log-server.json")) );
	//Jde::InitializeLogger( "log-server" );
	//INFO( "Running as console='{}' daemon='{}'", console, result );

	Jde::ApplicationServer::SettingsPtr = Jde::Settings::Global().SubContainer( "app-server" );

	std::thread{ []{Jde::ApplicationServer::Run();} }.detach();
	Jde::IApplication::Pause();

	Jde::IApplication::CleanUp();
	//_CrtDumpMemoryLeaks();
}

namespace Jde::ApplicationServer
{
	//unique_ptr<Jde::IO::Sockets::Server> pServer;
	int Run()
	{
		//var cnt = std::chrono::duration_cast<std::chrono::microseconds>( Clock::now()-TimePoint{} ).count();
		//DBG( "cnt='{}'", cnt );
		Logging::Data::SetDataSource( DB::DataSource(SettingsPtr->Get<fs::path>("dbDriver"), SettingsPtr->Get<string>("connectionString")) );
		Logging::LogClient::CreateInstance();

		constexpr PortType ReceivePort = 4321;
		constexpr uint WebSocketPort = 1967;
		//Jde::Application::SetConsoleTitle( fmt::format("Log Server web( '{}' ) receive( '{}' )", WebSocketPort, ReceivePort)  );
		var spWebSocket = Web::MyServer::CreateInstance( WebSocketPort );
		IApplication::AddShutdown( spWebSocket );
		//EtwListener::Create();
		IApplication::AddShutdown( Listener::Create( ReceivePort) );
		//pServer = make_unique<IO::Sockets::Server>( ReceivePort, Listener::GetInstancePtr() );
		
		return EXIT_SUCCESS;
	}
}