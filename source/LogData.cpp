#include "LogData.h"

#include "../../Framework/source/db/DBQueue.h"
#include "../../Framework/source/db/Syntax.h"

/*
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning( default : 4715)

#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/db/DataSource.h"
#include "../../Framework/source/db/Row.h"
#include <jde/Str.h>
#include <jde/io/File.h>
#include "../../Framework/source/Settings.h"
*/
#define var const auto

namespace Jde{
	using boost::concurrent_flat_map;
	using nlohmann::json;
	//using ApplicationServer::Web::FromServer::Traces;
	//using ApplicationServer::Web::FromServer::Applications;
	sp<DB::IDataSource> _dataSource;
	sp<DB::DBQueue> _pDbQueue;
	static sp<LogTag> _logTag = Logging::Tag( "app.log" );
	α Configure()ε->void{
		if( auto p = Settings::Get<fs::path>( "db/meta" ); p ){
			INFO( "db meta='{}'"sv, p->string() );
			json j;
			try{
				if( !fs::exists(*p) )
					*p = _debug && _msvc ? "../config/meta.json" : IApplication::ApplicationDataFolder() / *p;//TODO combine with UM.cpp and move somewhere else.
				j = json::parse( IO::FileUtilities::Load(*p) );
			}
			catch( const nlohmann::json::exception& e ){
				throw IOException( SRCE_CUR, *p, ELogLevel::Critical, "Error reading '{}'", e.what() );
			}
			if( _dataSource && Settings::Get<bool>("db/createSchema").value_or(true) )
				_dataSource->SchemaProc()->CreateSchema( j, fs::path{*p}.parent_path() );
		}
		else
			INFO( "db/meta not specified" );
	}
}
namespace Jde{
	α App::SetDataSource( sp<DB::IDataSource> dataSource )ε->void{
		//TRACE( "SetDataSource='{}'"sv, dataSource ? "on" : "off" );
		_dataSource = dataSource;
		function<void()> clean =  [](){ DBG( "LogData::_dataSource = nullptr;"sv ); _dataSource = nullptr;  };
		DB::ShutdownClean( clean );
		_pDbQueue = ms<DB::DBQueue>( dataSource );
		IApplication::AddShutdown( _pDbQueue );
		IApplication::AddShutdownFunction( [](){_pDbQueue=nullptr;} );


		Configure();
	}
	α App::AddInstance( sv applicationName, sv hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK,ELogLevel,ELogLevel>{
		AppPK applicationId;
		AppInstancePK applicationInstanceId;
		optional<uint> dbLogLevelInt; optional<uint> fileLogLevelInt;
		auto fnctn = [&applicationId, &applicationInstanceId, &dbLogLevelInt,&fileLogLevelInt](const DB::IRow& row){
			row >> applicationId >> applicationInstanceId >> dbLogLevelInt >> fileLogLevelInt;
		};
		while( !_dataSource )
			std::this_thread::sleep_for( 1s );
		_dataSource->ExecuteProc( "log_application_instance_insert(?,?,?)", {applicationName, hostName, processId}, fnctn );

		ELogLevel dbLogLevel = dbLogLevelInt.has_value() ? (ELogLevel)dbLogLevelInt.value() : ELogLevel::Information;
		ELogLevel fileLogLevel = fileLogLevelInt.has_value() ? (ELogLevel)fileLogLevelInt.value() : ELogLevel::Information;

		return make_tuple( applicationId, applicationInstanceId, dbLogLevel, fileLogLevel );
	}

	α Fetch( string sql, SRCE )ε->concurrent_flat_map<uint32,string>{
		concurrent_flat_map<uint32,string> map;
		_dataSource->Select( move(sql), [&map]( const DB::IRow& row ){ map.emplace( row.GetUInt32(0), row.GetString(1) ); }, {}, sl );
		return map;
	}

	α App::LoadFiles( SL sl )ε->concurrent_flat_map<uint32,string>{
		return Fetch( "select id, value from log_files", sl );
	}
	α App::LoadFunctions( SL sl )ε->concurrent_flat_map<uint32,string>{
		return Fetch( "select id, value from log_functions", sl );
	}
	α App::LoadMessages( SL sl )ε->concurrent_flat_map<uint32,string>{
		return Fetch( "select log_messages.id, value from logs join log_messages on logs.message_id=log_messages.id", sl );
	}
	α App::LoadMessageIds( SL sl )ε->concurrent_flat_set<uint32>{
		concurrent_flat_set<uint32> y;
		_dataSource->Select( "select log_messages.id from log_messages", [&y]( const DB::IRow& row ){ y.emplace( row.GetUInt32(0) ); }, sl );
		return y;
	}
	#define _pQueue if( auto p = _pDbQueue; p )p
	α App::SaveString( AppPK /*applicationId*/, Proto::FromClient::EFields field, StringPK id, string value, SL sl )ι->void{

		sv table = "log_messages";
		sv frmt = "insert into {}(id,value)values(?,?)";
		if( field==Proto::FromClient::EFields::MessageId )
			frmt = "insert into {}(id,value)values(?,?)";
		else if( field==Proto::FromClient::EFields::FileId )
			table = "log_files";
		else if( field==Proto::FromClient::EFields::FunctionId )
			table = "log_functions";
		else{
			ERRX( "unknown field '{}'.", (int)field );
			return;
		}
		var sql = Jde::format( fmt::runtime(frmt), table );
		auto pParameters = ms<vector<DB::object>>();  pParameters->reserve(3);
		//ASSERT( Calc32RunTime(*pValue)==id );
		if( Calc32RunTime(value)!=id )
			return ERRX( "id '{}' does not match crc of '{}'", id, value );//locks itself on server log.
		pParameters->push_back( id );
		pParameters->push_back( move(value) );
		_pQueue->Push( sql, pParameters, false, sl );
	}
#define _syntax DB::DefaultSyntax()
	α App::LoadEntries( AppPK applicationId, AppInstancePK instanceId, ELogLevel /*level*/, const std::optional<TimePoint>& pStart, uint limit )ι->up<Proto::FromServer::Traces>{
		auto pTraces = mu<Proto::FromServer::Traces>();
		flat_map<LogPK,Proto::FromServer::TraceMessage*> mapTraces;
		auto fnctn = [&pTraces, &mapTraces]( const DB::IRow& row )
		{
			auto pTrace = pTraces->add_values();

			uint i=0;
			var id = row.GetUInt32(i++);
			pTrace->set_id( id );
			pTrace->set_instance_id( row.GetUInt32(i++) );
			pTrace->set_file_id( row.GetUInt32(i++) );
			pTrace->set_function_id( row.GetUInt32(i++) );
			pTrace->set_line_number( row.GetUInt32(i++) );
			pTrace->set_message_id( row.GetUInt32(i++) );
			pTrace->set_level( (Jde::Proto::ELogLevel)row.GetUInt16(i++) );
			pTrace->set_thread_id( row.GetUInt32(i++) );
			*pTrace->mutable_time() = IO::Proto::ToTimestamp( row.GetTimePoint(i++) );
			pTrace->set_user_pk( row.GetUInt32(i++) );
			mapTraces.emplace( id, pTrace );
		};

		try
		{
			vector<string> where;
			if( pStart )
				where.push_back( Jde::format("CONVERT_TZ(time, @@session.time_zone, '+00:00')>'{}'", ToIsoString(*pStart)) );
			std::vector<DB::object> params;
			if( applicationId>0 )
			{
				where.push_back( "application_id=?" );
				params.push_back( applicationId );
			}
			if( instanceId>0 )
			{
				where.push_back( "application_instance_id=?" );
				params.push_back( instanceId );
			}

			var sql = Jde::format( "select id, application_instance_id, file_id, function_id, line_number, message_id, severity, thread_id, {}, user_id from logs", _syntax.DateTimeSelect("time") );
			auto whereString = where.size() ? Jde::format( " where {}", Str::AddSeparators(where, " and ") ) : string{};
			var orderDirection = pStart ? "asc"sv : "desc"sv;
			_dataSource->Select( _syntax.Limit(Jde::format("{}{} order by id {}", sql, whereString, orderDirection), limit), fnctn, params );
			//_dataSource->Select( _syntax.Limit( Jde::format("{}{} order by id {} limit {}", sql, whereString, orderDirection, limit), fnctn, params );
			if( mapTraces.size() )
			{
				auto fnctn2 = [&mapTraces]( const DB::IRow& row )
				{
					var id = row.GetUInt32( 0 );
					auto pTrace = mapTraces.find( id );
					if( pTrace!=mapTraces.end() )
						*pTrace->second->add_variables() = row.GetString( 1 );
				};
				constexpr sv variableSql = "select log_id, value, variable_index from log_variables join logs on logs.id=log_variables.log_id"sv;
				if( whereString.size() && mapTraces.size()==limit )
				{
					whereString += " and logs.id>=?";
					params.push_back( mapTraces.begin()->first );
				}
				_dataSource->Select( Jde::format("{}{} order by log_id {}, variable_index", variableSql, whereString, orderDirection), fnctn2, params );
			}
		}
		catch(const std::exception& /*e*/)
		{}

		return pTraces;
	}
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

		constexpr sv sql = "select id, name, db_log_level, file_log_level from log_applications"sv;
		Try( [&]()
		{
			if( id )
				_dataSource->Select( Jde::format("{} where id=?"sv, sql), fnctn, {id} );
			else
				_dataSource->Select( string{sql}, fnctn, {} );
		} );
		return pApplications;
	}

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
}