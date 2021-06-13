#include "Listener.h"
//#include "EtwListener.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/db/Database.h"

#define var const auto

namespace Jde::ApplicationServer
{
	void Run();
}
int main( int argc, char** argv )
{
	using namespace Jde;
	OSApp::Startup( argc, argv, "AppServer" );
	try
	{
		ApplicationServer::Run();
		IApplication::Pause();
	}
	catch( const Exception& e )
	{
		std::cout << "Exiting on error:  " << e.what() << std::endl;
	}
	IApplication::CleanUp();
	return EXIT_SUCCESS;
}

namespace Jde::ApplicationServer
{
	void Run()
	{
		Logging::Data::SetDataSource( DB::DataSource() );
		Logging::LogClient::CreateInstance();

		constexpr PortType ReceivePort = 4321;
		constexpr uint WebSocketPort = 1967;
		var spWebSocket = Web::MyServer::CreateInstance( WebSocketPort );
		IApplication::AddShutdown( spWebSocket );
		IApplication::AddShutdown( Listener::Create( ReceivePort) );
	}
}