#include "Listener.h"
//#include "EtwListener.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/DB/Database.h"

#define var const auto

namespace Jde::ApplicationServer
{
	int Run();
}
int main( int argc, char** argv )
{
	Jde::OSApp::Startup( argc, argv, "AppServer" );

	//Jde::ApplicationServer::SettingsPtr = Jde::Settings::Global().SubContainer( "app-server" );

	std::thread{ []{Jde::ApplicationServer::Run();} }.detach();
	Jde::IApplication::Pause();

	Jde::IApplication::CleanUp();
}

namespace Jde::ApplicationServer
{
	int Run()
	{
		Threading::SetThreadDscrptn( "Startup" );
		Logging::Data::SetDataSource( DB::DataSource(Settings::Global().Get<fs::path>("dbDriver"), Settings::Global().Get<string>("connectionString")) );
		Logging::LogClient::CreateInstance();

		constexpr PortType ReceivePort = 4321;
		constexpr uint WebSocketPort = 1967;
		var spWebSocket = Web::MyServer::CreateInstance( WebSocketPort );
		IApplication::AddShutdown( spWebSocket );
		IApplication::AddShutdown( Listener::Create( ReceivePort) );

		return EXIT_SUCCESS;
	}
}