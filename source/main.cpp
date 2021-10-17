#include "Listener.h"
//#include "EtwListener.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/db/Database.h"

#define var const auto

#ifndef _MSC_VER
namespace Jde{  string OSApp::CompanyName()noexcept{ return "Jde-Cpp"; } }
#endif

namespace Jde
{
	void Run();
}
int main( int argc, char** argv )
{
	using namespace Jde;
	try
	{
		OSApp::Startup( argc, argv, "AppServer", "jde-cpp App Server." );
		Logging::Data::SetDataSource( DB::DataSource() );
		Logging::LogClient::CreateInstance();

		IApplication::Pause();
	}
	catch( const Exception& e )
	{
		std::cout << "Exiting on error:  " << e.what() << std::endl;
	}
	IApplication::CleanUp();
	return EXIT_SUCCESS;
}

// namespace Jde
// {
// 	void Run()
// 	{
// 		//
// 		//constexpr uint WebSocketPort = 1967;
// 		//var spWebSocket = ApplicationServer::Web::MyServer::CreateInstance( WebSocketPort );
// 		//IApplication::AddShutdown( spWebSocket );
// 		//IApplication::AddShutdown( ApplicationServer::Listener::Create( ReceivePort) );
// 	}
// }