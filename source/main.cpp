#include "Listener.h"
#include "../../Framework/source/um/UM.h"
#include "WebServer.h"
#include "LogClient.h"
#include "LogData.h"
#include "Rest.h"
#include <jde/io/Crc.h>
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/db/Database.h"
#include "../../Ssl/source/Ssl.h"

#define var const auto

#ifndef _MSC_VER
namespace Jde{ α OSApp::CompanyName()ι->string{ return "Jde-Cpp"; } }
namespace Jde{ α OSApp::ProductName()ι->sv{ return "AppServer"sv; } }
#endif

namespace Jde::ApplicationServer
{
	α Startup( bool initialCall=true )noexcept->void;
}

int main( int argc, char** argv )
{
	using namespace Jde;
	try
	{
		OSApp::Startup( argc, argv, "AppServer", "jde-cpp App Server." );
		IApplication::AddThread( ms<Threading::InterruptibleThread>("Startup", [&](){ApplicationServer::Startup();}) );
		IApplication::Pause();
	}
	catch( const IException& e )
	{
		std::cout << "Exiting on error:  " << e.what() << std::endl;
	}
	IApplication::Cleanup();
	return EXIT_SUCCESS;
}

namespace Jde
{
	α ApplicationServer::Startup( bool initialCall )noexcept->void
	{
		try
		{
				//if( Settings::TryGet<bool>("um/use").value_or(false) ) currently need to configure um so meta is loaded.
			{
				UM::Configure();
			}
			Logging::Data::SetDataSource( DB::DataSourcePtr() );
			Logging::LogClient::CreateInstance();
		}
		catch( IException& e )
		{
			std::cerr << e.what() << std::endl;
			{auto e2=e.Move();}//destructor log.
			std::this_thread::sleep_for( 1s );
			std::terminate();
		}
		Listener().DoAccept();
		DBG( "Startup Loading Complete." );
		if( IO::Crc::Calc32(__FILE__)!=Calc32RunTime(__FILE__) )
			CRITICAL( "crcs do not matchup consteval={:x} vs runtime={:x}", IO::Crc::Calc32(__FILE__), Calc32RunTime(__FILE__) );
		IApplication::RemoveThread( "Startup" )->Detach();
	}
}