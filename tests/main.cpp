#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include <jde/App.h>
#include "../source/LogClient.h"
#include "../source/LogData.h"
#include "../../Framework/source/db/Database.h"

#define var const auto
namespace Jde
{
	shared_ptr<Settings::Container> SettingsPtr;
 	void Startup( int argc, char **argv )noexcept
	{
		var sv2 = "Tests.AppServer"sv;
		string appName{ sv2 };
		OSApp::Startup( argc, argv, appName, "Test app" );

		Jde::Threading::SetThreadDscrptn( "Main" );
		Logging::Data::SetDataSource( DB::DataSource(Settings::Global().Get<fs::path>("dbDriver"), Settings::Global().Get<string>("connectionString")) );
		Logging::LogClient::CreateInstance();
	}
}
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

int main(int argc, char **argv)
{
	 _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
 	::testing::InitGoogleTest( &argc, argv );

	Jde::Startup( argc, argv );

	auto result = EXIT_FAILURE;
	::testing::GTEST_FLAG(filter) = "ThreadingTest.*";//SaveToFile
	result = RUN_ALL_TESTS();

	Jde::IApplication::Instance().Pause();
	Jde::IApplication::CleanUp();

	return result;
}