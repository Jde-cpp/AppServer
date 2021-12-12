#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include <jde/App.h>
#include "../source/LogClient.h"
#include "../source/LogData.h"
#include "../../Framework/source/db/Database.h"

#define var const auto
namespace Jde
{
	sp<Settings::Container> SettingsPtr;
 	void Startup( int argc, char **argv )noexcept
	{
		var sv2 = "Tests.AppServer"sv;
		string appName{ sv2 };
		OSApp::Startup( argc, argv, appName, "Test app" );

		Threading::SetThreadDscrptn( "Main" );
		Logging::Data::SetDataSource( DB::DataSource() );
		Logging::LogClient::CreateInstance();
	}
}
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

int main(int argc, char **argv)
{
	using namespace Jde;
 	::testing::InitGoogleTest( &argc, argv );

	Startup( argc, argv );

	::testing::GTEST_FLAG( filter ) = Settings::TryGet<string>( "testing/tests" ).value_or( "*" );
	auto result = RUN_ALL_TESTS();

	IApplication::Shutdown();
	IApplication::Cleanup();

	return result;
}