#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/application/Application.h"
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
		OSApp::Startup( argc, argv, appName );

		Jde::Threading::SetThreadDscrptn( "Main" );
		Logging::Data::SetDataSource( DB::DataSource(Settings::Global().Get<fs::path>("dbDriver"), Settings::Global().Get<string>("connectionString")) );
		Logging::LogClient::CreateInstance();

/*		std::filesystem::path settingsPath{ fmt::format("{}.json", appName) };
		if( !fs::exists(settingsPath) )
			settingsPath = std::filesystem::path( fmt::format("../{}.json", appName) );
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		Cache::CreateInstance();
*/
	}
}
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

int main(int argc, char **argv)
{
	 _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
 	::testing::InitGoogleTest( &argc, argv );
	auto x = new char[]{"aaaaaaaaaaaaaaaaaaaaaaaaaa"};//sb 1 after
	//_CrtSetBreakAlloc( 11147 ); 

	Jde::Startup( argc, argv );

	auto result = EXIT_FAILURE;
	::testing::GTEST_FLAG(filter) = "ThreadingTest.*";//SaveToFile
	result = RUN_ALL_TESTS();

	Jde::IApplication::Instance().Wait();
	Jde::IApplication::CleanUp();
//	auto y = new char[]{"bbbbbbbbbbbbbbbbbbbbbbbbbbb"};
	return result;
}