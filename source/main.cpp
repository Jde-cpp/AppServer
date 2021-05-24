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
	using namespace Jde;
	OSApp::Startup( argc, argv, "AppServer" );

	//Jde::ApplicationServer::SettingsPtr = Jde::Settings::Global().SubContainer( "app-server" );
	try
	{
		//std::thread{ []{ApplicationServer::Run();} }.detach();
		ApplicationServer::Run();
	}
	catch( const Exception&  )
	{
		return EXIT_FAILURE;
	}

	IApplication::Pause();

	IApplication::CleanUp();
	return EXIT_SUCCESS;
}

namespace Jde::ApplicationServer
{
	int Run()
	{
		//Threading::SetThreadDscrptn( "Startup" );
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