#include "LogData.h"
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromServer.h>
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/db/DBQueue.h"
#include "../../Framework/source/db/Syntax.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Framework/source/db/GraphQuery.h"

#define var const auto

namespace Jde::App{
	sp<DB::DBQueue> _pDbQueue;
	static sp<LogTag> _logTag = Logging::Tag( "app.log" );
	α Configure()ε->void{
		if( auto dbMetaPath = Settings::Get<fs::path>( "db/meta" ); dbMetaPath ){
			INFO( "db meta='{}'"sv, dbMetaPath->string() );
			json j;
			try{
				if( !fs::exists(*dbMetaPath) )
					*dbMetaPath = _debug && _msvc ? "../config/meta.json" : IApplication::ApplicationDataFolder() / *dbMetaPath;//TODO combine with UM.cpp and move somewhere else.
				j = json::parse( IO::FileUtilities::Load(*dbMetaPath) );
			}
			catch( const nlohmann::json::exception& e ){
				throw IOException( SRCE_CUR, *dbMetaPath, ELogLevel::Critical, "Error reading '{}'", e.what() );
			}
			if( auto p = Settings::Get<bool>("db/createSchema").value_or(true) ? DB::DataSourcePtr() : nullptr; p  )
				p->SchemaProc()->CreateSchema( j, fs::path{*dbMetaPath}.parent_path() );
		}
		else
			INFO( "db/meta not specified" );
	}

	α Server::ConfigureDatasource()ε->void{
//		App::SetDatasource( datasource );
//		auto clean =  [](){ DBG( "LogData::_dataSource = nullptr;"sv ); App::SetDatasource( nullptr ); };
//		DB::ShutdownClean( clean );
		_pDbQueue = ms<DB::DBQueue>( DB::DataSourcePtr() );
		Process::AddShutdown( _pDbQueue );
		Process::AddShutdownFunction( [](bool terminate){_pDbQueue=nullptr;} );
		Configure();
	}
	#define _pQueue if( auto p = _pDbQueue; p )p
	α Server::SaveString( Proto::FromClient::EFields field, StringPK id, string value, SL sl )ι->void{
		sv table = "log_messages";
		if( field==Proto::FromClient::EFields::FileId )
			table = "log_files";
		else if( field==Proto::FromClient::EFields::FunctionId )
			table = "log_functions";
		else if( field!=Proto::FromClient::EFields::MessageId ){
			ERRX( "unknown field '{}'.", (int)field );
			return;
		}
		var sql = 𐢜( "insert into {}(id,value)values(?,?)", table );
		auto pParameters = ms<vector<DB::object>>();  pParameters->reserve(3);
		//ASSERT( Calc32RunTime(*pValue)==id );
		if( Calc32RunTime(value)!=id )
			return ERRX( "id '{}' does not match crc of '{}'", id, value );//locks itself on server log.
		pParameters->push_back( id );
		pParameters->push_back( move(value) );
		_pQueue->Push( sql, pParameters, false, sl );
	}
}
namespace Jde{
	α App::AddInstance( sv applicationName, sv hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK,ELogLevel,ELogLevel>{
		AppPK applicationId;
		AppInstancePK applicationInstanceId;
		optional<uint> dbLogLevelInt; optional<uint> fileLogLevelInt;
		auto fnctn = [&applicationId, &applicationInstanceId, &dbLogLevelInt,&fileLogLevelInt](const DB::IRow& row){
			row >> applicationId >> applicationInstanceId >> dbLogLevelInt >> fileLogLevelInt;
		};
		while( !DB::DataSourcePtr() )
			std::this_thread::sleep_for( 50ms );
		DB::DataSource().ExecuteProc( "log_application_instance_insert(?,?,?)", {applicationName, hostName, processId}, fnctn );

		ELogLevel dbLogLevel = dbLogLevelInt.has_value() ? (ELogLevel)dbLogLevelInt.value() : ELogLevel::Information;
		ELogLevel fileLogLevel = fileLogLevelInt.has_value() ? (ELogLevel)fileLogLevelInt.value() : ELogLevel::Information;

		return make_tuple( applicationId, applicationInstanceId, dbLogLevel, fileLogLevel );
	}

#define _syntax DB::DefaultSyntax()
/*
	α App::LoadApplications( AppPK id )ι->up<Proto::FromServer::Applications>
	{
		auto pApplications = mu<Proto::FromServer::Applications>();
		auto fnctn = [&pApplications]( const DB::IRow& row )
		{
			auto pApplication = pApplications->add_values();
			pApplication->set_id( row.GetUInt32(0) );
			pApplication->set_name( row.GetString(1) );
			optional<uint> dbLevel = row.GetUIntOpt( 2 );
			pApplication->set_db_level( dbLevel.has_value() ? (Jde::Proto::ELogLevel)dbLevel.value() : Jde::Proto::ELogLevel::Information );
			optional<uint> fileLevel = row.GetUIntOpt( 3 );
			pApplication->set_file_level( fileLevel.has_value() ? (Jde::Proto::ELogLevel)fileLevel.value() : Jde::Proto::ELogLevel::Information );
		};

		constexpr sv baseSql = "select id, name, db_log_level, file_log_level from log_applications"sv;
		Try( [&](){
			string sql = id ? 𐢜("{} where id=?", baseSql) : string{baseSql};
			var params = id ? vector<DB::object>{id} : vector<DB::object>{};
			if( auto p = Datasource(); p )
				p->Select( sql, fnctn, params );
		} );
		return pApplications;
	}
*/
	α App::SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>* variables, SL sl )ι->void{
		var variableCount = std::min( (uint)5, variables ? variables->size() : 0 );
		auto pParameters = ms<vector<DB::object>>(); pParameters->reserve( 10+variableCount );
		pParameters->push_back( applicationId );
		pParameters->push_back( instanceId );
		pParameters->push_back( m.file_id() );
		pParameters->push_back( m.function_id() );
		pParameters->push_back( m.line() );
		pParameters->push_back( m.message_id() );
		pParameters->push_back( (uint8)m.level() );
		pParameters->push_back( m.thread_id() );
		pParameters->push_back( IO::Proto::ToTimePoint(m.time()) );
		pParameters->push_back( m.user_pk() );
		constexpr sv procedure = "log_message_insert"sv;
		constexpr sv args = "(?,?,?,?,?,?,?,?,?,?"sv;
		ostringstream os;
		os << procedure;
		if( variableCount>0 )
			os << variableCount;
		os << args;
		for( uint i=0; i<variableCount; ++i )
		{
			os << ",?";
			pParameters->push_back( (*variables)[i] );
		}
		os << ")";
		_pQueue->Push( os.str(), pParameters, true, sl );
	}
	namespace App{
		α Data::LoadEntries( DB::TableQL table )ε->Proto::FromServer::Traces{
			string whereString;
			auto [sql,params] = DB::GraphQL::SelectStatement( table, true, &whereString );
			flat_map<LogPK,Proto::FromServer::Trace> mapTraces;
			auto fnctn = [&mapTraces, &table]( const DB::IRow& row ){
				auto t=FromServer::ToTrace( row, table.Columns );
				auto id = t.id();
				mapTraces.emplace( id, move(t) );
			};
			uint limit = 1000;//TODO apply limit rows.
			//var orderDirection = pStart ? "asc"sv : "desc"sv;

			DB::DataSource().Select( _syntax.Limit(sql, limit), fnctn, params ); //TODO awaitable
			if( mapTraces.size() ){
				auto addVariables = [&mapTraces]( const DB::IRow& row ){
					var id = row.GetUInt32( 0 );
					if( auto pTrace = mapTraces.find(id); pTrace!=mapTraces.end() )
						*pTrace->second.add_args() = row.GetString( 1 );
				};
				constexpr sv variableSql = "select log_id, value, variable_index from log_variables join logs on logs.id=log_variables.log_id";
				if( whereString.size() && mapTraces.size()==limit ){
					whereString += " and logs.id>=?";
					params.push_back( mapTraces.begin()->first );
				}
				DB::DataSource().Select( 𐢜("{}{}, variable_index", variableSql, whereString), addVariables, params );
			}
			Proto::FromServer::Traces traces;
			for( auto& [id,trace] : mapTraces )
				*traces.add_values() = move(trace);
			return traces;
		}

		α LoadStrings( string sql, SRCE )ε->concurrent_flat_map<uint32,string>{
			concurrent_flat_map<uint32,string> map;
			DB::DataSource().Select( move(sql), [&map]( const DB::IRow& row ){ map.emplace( row.GetUInt32(0), row.GetString(1) ); }, {}, sl );
			return map;
		}
		α LoadFiles( SL sl )ε->concurrent_flat_map<uint32,string>{
			return LoadStrings( "select id, value from log_files", sl );
		}
		α LoadFunctions( SL sl )ε->concurrent_flat_map<uint32,string>{
			return LoadStrings( "select id, value from log_functions", sl );
		}
		α LoadMessages( SL sl )ε->concurrent_flat_map<uint32,string>{
			return LoadStrings( "select log_messages.id, value from logs join log_messages on logs.message_id=log_messages.id", sl );
		}

		α Data::LoadStrings( SL sl )ε->void{
			StringCache::Merge( LoadFiles(sl), LoadFunctions(sl), LoadMessages(sl) );
		}
	}
}