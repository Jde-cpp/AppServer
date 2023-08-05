#include "Listener.h"
//#include "EtwListener.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/db/Database.h"
#include "../../Ssl/source/Ssl.h"

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
		Logging::Data::SetDataSource( DB::DataSourcePtr() );
		Logging::LogClient::CreateInstance();

		IApplication::Pause();
	}
	catch( const IException& e )
	{
		std::cout << "Exiting on error:  " << e.what() << std::endl;
	}
	IApplication::Cleanup();
	return EXIT_SUCCESS;
}