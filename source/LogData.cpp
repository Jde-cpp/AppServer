#include "LogData.h"
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning( default : 4715)

#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/db/DataSource.h"
#include "../../Framework/source/db/Syntax.h"
#include "../../Framework/source/db/DBQueue.h"
#include "../../Framework/source/db/Row.h"
#include <jde/Str.h>
#include <jde/io/File.h>
#include "../../Framework/source/Settings.h"

#define var const auto

namespace Jde::Logging
{
	using nlohmann::json;
	using ApplicationServer::Web::FromServer::Traces;
	using ApplicationServer::Web::FromServer::Applications;
	sp<DB::IDataSource> _dataSource;
	sp<DB::DBQueue> _pDbQueue;
	α Configure()noexcept(false)->void
	{
		var p = Settings::Env( "db/meta" );
		if( p )
		{
			INFO( "db meta='{}'"sv, *p );
			json j;
			try
			{
				j = json::parse( IO::FileUtilities::Load(*p) );
			}
			catch( const nlohmann::json::exception& e )
			{
				THROW( "Error reading {} - {}", *p, e.what() );
			}
			if( _dataSource && Settings::Get<bool>("db/createSchema").value_or(true) )
			{
				_dataSource->SchemaProc()->CreateSchema( j, fs::path{*p}.parent_path() );
			}
		}
		else
			INFO( "db/meta not specified" );
	}

	α Data::SetDataSource( sp<DB::IDataSource> dataSource )noexcept(false)->void
	{
		//TRACE( "SetDataSource='{}'"sv, dataSource ? "on" : "off" );
		_dataSource = dataSource;
		function<void()> clean =  [](){ DBG( "LogData::_dataSource = nullptr;"sv ); _dataSource = nullptr;  };
		DB::ShutdownClean( clean );
		_pDbQueue = ms<DB::DBQueue>( dataSource );
		IApplication::AddShutdown( _pDbQueue );
		IApplication::AddShutdownFunction( [](){_pDbQueue=nullptr;} );


		Configure();
	}
	α Data::AddInstance( sv applicationName, sv hostName, uint processId )noexcept(false)->std::tuple<ApplicationPK, ApplicationInstancePK,ELogLevel,ELogLevel>
	{
		ApplicationPK applicationId;
		ApplicationInstancePK applicationInstanceId;
		optional<uint> dbLogLevelInt; optional<uint> fileLogLevelInt;
		auto fnctn = [&applicationId, &applicationInstanceId, &dbLogLevelInt,&fileLogLevelInt](const DB::IRow& row)
		{
			row >> applicationId >> applicationInstanceId >> dbLogLevelInt >> fileLogLevelInt;
		};
		while( !_dataSource )
			sleep( 1 );
		_dataSource->ExecuteProc( "log_application_instance_insert(?,?,?)", {applicationName, hostName, processId}, fnctn );

		ELogLevel dbLogLevel = dbLogLevelInt.has_value() ? (ELogLevel)dbLogLevelInt.value() : ELogLevel::Information;
		ELogLevel fileLogLevel = fileLogLevelInt.has_value() ? (ELogLevel)fileLogLevelInt.value() : ELogLevel::Information;

		return make_tuple( applicationId, applicationInstanceId, dbLogLevel, fileLogLevel );
	}

	α Fetch( string sql, SRCE )noexcept(false)->Collections::UnorderedMap<uint32,string>
	{
		Collections::UnorderedMap<uint32,string> map;
		_dataSource->Select( move(sql), [&map]( const DB::IRow& row ){ map.emplace( row.GetUInt32(0), ms<string>(row.GetString(1)) ); }, {}, sl );
		return map;
	}

	α Data::LoadFiles( SL sl )noexcept(false)->Collections::UnorderedMap<uint32,string>
	{
		return Fetch( "select id, value from log_files", sl );
	}
	α Data::LoadFunctions( SL sl )noexcept(false)->Collections::UnorderedMap<uint32,string>
	{
		return Fetch( "select id, value from log_functions", sl );
	}
	α Data::LoadMessages( SL sl )noexcept(false)->Collections::UnorderedMap<uint32,string>
	{
		return Fetch( "select log_messages.id, value from logs join log_messages on logs.message_id=log_messages.id", sl );
	}
	α Data::LoadMessageIds( SL sl )noexcept(false)->unordered_set<uint32>
	{
		unordered_set<uint32> y;
		_dataSource->Select( "select log_messages.id from log_messages", [&y]( const DB::IRow& row ){ y.emplace( row.GetUInt32(0) ); }, sl );
		return y;
	}
	#define _pQueue if( auto p = _pDbQueue; p )p
	α Data::SaveString( ApplicationPK /*applicationId*/, Proto::EFields field, uint32 id, sp<string> pValue, SL sl )noexcept->void
	{
		sv table = "log_messages"sv;
		sv frmt = "insert into {}(id,value)values(?,?)";
		if( field==Proto::EFields::MessageId )
			frmt = "insert into {}(id,value)values(?,?)";
		else if( field==Proto::EFields::FileId )
			table = "log_files"sv;
		else if( field==Proto::EFields::FunctionId )
			table = "log_functions"sv;
		else
			return ERR( "unknown field '{}'."sv, field );

		var sql = format( fmt::runtime(frmt), table );
		auto pParameters = ms<vector<DB::object>>();  pParameters->reserve(3);
		pParameters->push_back( static_cast<uint32>(id) );
		pParameters->push_back( pValue );
		_pQueue->Push( sql, pParameters, false, sl );
	}
#define _syntax DB::DefaultSyntax()
	α Data::LoadEntries( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel /*level*/, const std::optional<TimePoint>& pStart, uint limit )noexcept->up<Traces>
	{
		auto pTraces = mu<Traces>();
		flat_map<LogPK,ApplicationServer::Web::FromServer::TraceMessage*> mapTraces;
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
			pTrace->set_level( (ApplicationServer::Web::FromServer::ELogLevel)row.GetUInt16(i++) );
			pTrace->set_thread_id( row.GetUInt32(i++) );
			var time = Chrono::MillisecondsSinceEpoch( row.GetTimePoint(i++) );
			pTrace->set_time( time );
			pTrace->set_user_id( row.GetUInt32(i++) );
			mapTraces.emplace( id, pTrace );
		};

		try
		{
			vector<string> where;
			if( pStart )
				where.push_back( format("CONVERT_TZ(time, @@session.time_zone, '+00:00')>'{}'", ToIsoString(*pStart)) );
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

			var sql = format( "select id, application_instance_id, file_id, function_id, line_number, message_id, severity, thread_id, {}, user_id from logs", _syntax.DateTimeSelect("time") );
			auto whereString = where.size() ? format( " where {}", Str::AddSeparators(where, " and ") ) : string{};
			var orderDirection = pStart ? "asc"sv : "desc"sv;
			_dataSource->Select( _syntax.Limit(format("{}{} order by id {}", sql, whereString, orderDirection), limit), fnctn, params );
			//_dataSource->Select( _syntax.Limit( format("{}{} order by id {} limit {}", sql, whereString, orderDirection, limit), fnctn, params );
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
				_dataSource->Select( format("{}{} order by log_id {}, variable_index", variableSql, whereString, orderDirection), fnctn2, params );
			}
		}
		catch(const std::exception& /*e*/)
		{}

		return pTraces;
	}
	α Data::LoadApplications( ApplicationPK id )noexcept->up<ApplicationServer::Web::FromServer::Applications>
	{
		auto pApplications = mu<Applications>();
		auto fnctn = [&pApplications]( const DB::IRow& row )
		{
			auto pApplication = pApplications->add_values();
			pApplication->set_id( row.GetUInt32(0) );
			pApplication->set_name( row.GetString(1) );
			optional<uint> dbLevel = row.GetUIntOpt( 2 );
			pApplication->set_db_level( dbLevel.has_value() ? (ApplicationServer::Web::FromServer::ELogLevel)dbLevel.value() : ApplicationServer::Web::FromServer::ELogLevel::Information );
			optional<uint> fileLevel = row.GetUIntOpt( 3 );
			pApplication->set_file_level( fileLevel.has_value() ? (ApplicationServer::Web::FromServer::ELogLevel)fileLevel.value() : ApplicationServer::Web::FromServer::ELogLevel::Information );
		};

		constexpr sv sql = "select id, name, db_log_level, file_log_level from log_applications"sv;
		Try( [&]()
		{
			if( id )
				_dataSource->Select( format("{} where id=?"sv, sql), fnctn, {id} );
			else
				_dataSource->Select( string{sql}, fnctn, {} );
		} );
		return pApplications;
	}

	α Data::PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables, SL sl )noexcept->void
	{
		var variableCount = std::min( (uint)5, variables.size() );
		auto pParameters = ms<vector<DB::object>>(); pParameters->reserve( 10+variableCount );
		pParameters->push_back( applicationId );
		pParameters->push_back( instanceId );
		pParameters->push_back( fileId );
		pParameters->push_back( functionId );
		pParameters->push_back( lineNumber );
		pParameters->push_back( messageId );
		pParameters->push_back( (uint8)level );
		pParameters->push_back( threadId );
		pParameters->push_back( time );
		pParameters->push_back( userId );
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
			pParameters->push_back( move(variables[i]) );
		}
		os << ")";
		_pQueue->Push( os.str(), pParameters, true, sl );
	}
}