#include "stdafx.h"
#include "LogData.h"
#include "types/proto/FromServer.pb.h"

#define var const auto

namespace Jde::Logging::Data
{
	using ApplicationServer::Web::FromServer::Traces;
	using ApplicationServer::Web::FromServer::Applications;
	sp<DB::IDataSource> _dataSource;
	sp<DB::DBQueue> _pDbQueue;
	void SetDataSource( sp<DB::IDataSource> dataSource )noexcept
	{
		TRACE( "SetDataSource='{}'", dataSource ? "on" : "off" );
		_dataSource = dataSource;
		_pDbQueue = make_shared<DB::DBQueue>( dataSource );
		Application::AddShutdown( _pDbQueue );
	}
	std::tuple<ApplicationPK, ApplicationInstancePK,ELogLevel,ELogLevel> AddInstance( string_view applicationName, string_view hostName, uint processId )noexcept(false)
	{
		ApplicationPK applicationId;
		ApplicationInstancePK applicationInstanceId;
		optional<uint> dbLogLevelInt; optional<uint> fileLogLevelInt;
		auto fnctn = [&applicationId, &applicationInstanceId, &dbLogLevelInt,&fileLogLevelInt](const DB::IRow& row)
		{
			row >> applicationId >> applicationInstanceId >> dbLogLevelInt >> fileLogLevelInt;
		};
		_dataSource->ExecuteProc( "log_application_instance_insert2(?,?,?)", {applicationName, hostName, processId}, fnctn );

		ELogLevel dbLogLevel = dbLogLevelInt.has_value() ? (ELogLevel)dbLogLevelInt.value() : ELogLevel::Information;
		ELogLevel fileLogLevel = fileLogLevelInt.has_value() ? (ELogLevel)fileLogLevelInt.value() : ELogLevel::Information;

		return make_tuple( applicationId, applicationInstanceId, dbLogLevel, fileLogLevel );
	}

	Collections::UnorderedMapPtr<uint32,string> Fetch( string_view sql, ApplicationPK applicationId )noexcept(false)
	{
		auto pMap = make_shared<Collections::UnorderedMap<uint32,string>>();
		auto fnctn = [&pMap]( const DB::IRow& row )
		{
			pMap->emplace( row.GetUInt32(0), make_shared<string>(row.GetString(1)) );
		};
		_dataSource->Select( sql, fnctn, {applicationId} );
		return pMap;
	}

	Collections::UnorderedMapPtr<uint32,string> LoadFiles( ApplicationPK applicationId )noexcept(false)
	{
		return Fetch( "select id, value from log_files where application_id=?", applicationId );
	}
	Collections::UnorderedMapPtr<uint32,string> LoadFunctions( ApplicationPK applicationId )noexcept(false)
	{
		return Fetch( "select id, value from log_functions where application_id=?", applicationId );
	}
	Collections::UnorderedMapPtr<uint32,string> LoadMessages( ApplicationPK applicationId )noexcept(false)
	{
		return Fetch( "select id, value from log_messages where application_id=?", applicationId );
	}

	void SaveString( ApplicationPK applicationId, Proto::EFields field, uint32 id, sp<string> pValue )noexcept
	{
		string_view table = "";
		switch( field )
		{
		case Proto::EFields::MessageId:
			table = "log_messages"sv;
			break;
		case Proto::EFields::FileId:
			table = "log_files"sv;
			break;
		case Proto::EFields::FunctionId:
			table = "log_functions"sv;
			break;
		default:
			ERR( "unknown field '{}'.", field );
			return;
		// case Proto::EFields::ThreadId:
		// 	pStrings->MessagesPtr->Emplace( id, value );
		// 	break;
		}
		var sql = fmt::format( "insert into {}(application_id,id,value)values(?,?,?);", table );
		auto pParameters = make_shared<vector<DB::DataValue>>();  pParameters->reserve(3);
		pParameters->push_back( applicationId );
		pParameters->push_back( static_cast<uint>(id) );
		pParameters->push_back( pValue );
		_pDbQueue->Push( sql, pParameters, false );
	}
	
	Traces* LoadEntries( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, const TimePoint& start )noexcept
	{
		auto pTraces = new Traces();
		map<LogPK,ApplicationServer::Web::FromServer::TraceMessage*> mapTraces;
		auto fnctn = [&pTraces, &mapTraces, instanceId]( const DB::IRow& row )
		{
			auto pTrace = pTraces->add_values();
			pTrace->set_instanceid( instanceId );
			uint i=0;
			var id = row.GetUInt32(i++);
			pTrace->set_instanceid( row.GetUInt32(i++) );
			pTrace->set_fileid( row.GetUInt32(i++) );
			pTrace->set_functionid( row.GetUInt32(i++) );
			pTrace->set_linenumber( row.GetUInt32(i++) );
			pTrace->set_messageid( row.GetUInt32(i++) );
			pTrace->set_level( (ApplicationServer::Web::FromServer::ELogLevel)row.GetUInt16(i++) );
			pTrace->set_threadid( row.GetUInt32(i++) );
			var time = Chrono::MillisecondsSinceEpoch( row.GetDateTime(i++) );
			pTrace->set_time( time );
			pTrace->set_userid( row.GetUInt32(i++) );
			mapTraces.emplace( id, pTrace );
		};
	
		try
		{
			//constexpr string_view whereFormat = "where{} time>now() - INTERVAL 1 DAY"sv;
			vector<string> where = { fmt::format("time>{}", ToIsoString(start)) };
			std::vector<DB::DataValue> parameters;
			if( applicationId>0 )
			{
				where.push_back( "application_id=?" );
				parameters.push_back( applicationId );
			}
			if( instanceId>0 )
			{
				where.push_back( "application_instance_id=?" );
				parameters.push_back( instanceId );
			}
			
			constexpr string_view sql = "select id, application_instance_id, file_id, function_id, line_number, message_id, severity, thread_id, UNIX_TIMESTAMP(time), user_id from logs"sv;
			string whereString = StringUtilities::AddSeparators( where, " and " );
			_dataSource->Select( fmt::format("{} where {} order by id, time", sql, whereString), fnctn, parameters );

			auto fnctn = [&mapTraces]( const DB::IRow& row )
			{
				var id = row.GetUInt32( 0 );
				auto pTrace = mapTraces.find( id );
				if( pTrace!=mapTraces.end() )
					*pTrace->second->add_variables() = row.GetString( 1 );
			};
			constexpr string_view variables = "select log_id, value, variable_index from log_variables join logs on logs.id=log_variables.log_id"sv;
			_dataSource->Select( fmt::format("{} where {} order by log_id, variable_index", variables, whereString), fnctn, parameters );
		}
		catch(const std::exception& e)
		{}

		return pTraces;
	}
	Applications* LoadApplications()noexcept
	{
		auto pApplications = new Applications();
		auto fnctn = [&pApplications]( const DB::IRow& row )
		{
			auto pApplication = pApplications->add_values();
			pApplication->set_id( row.GetUInt32(0) );
			pApplication->set_name( row.GetString(1) );
			optional<uint> dbLevel = row.GetUIntOpt( 2 );
			pApplication->set_dblevel( dbLevel.has_value() ? (ApplicationServer::Web::FromServer::ELogLevel)dbLevel.value() : ApplicationServer::Web::FromServer::ELogLevel::Information );
			optional<uint> fileLevel = row.GetUIntOpt( 3 );
			pApplication->set_filelevel( fileLevel.has_value() ? (ApplicationServer::Web::FromServer::ELogLevel)fileLevel.value() : ApplicationServer::Web::FromServer::ELogLevel::Information );
		};

		try
		{
			constexpr string_view sql = "select id, name, db_log_level, file_log_level from log_applications"sv;
			_dataSource->Select( sql, fnctn, {} );
		}
		catch(const std::exception& e)
		{}

		return pApplications;
	}
	void PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept
	{
		var variableCount = std::min( (uint)5, variables.size() );
		auto pParameters = make_shared<vector<DB::DataValue>>(); 
		pParameters->reserve( 10+variableCount );
		pParameters->push_back( applicationId );
		pParameters->push_back( instanceId );
		pParameters->push_back( (uint)fileId );
		pParameters->push_back( (uint)functionId );
		pParameters->push_back( (uint)lineNumber );
		pParameters->push_back( (uint)messageId );
		pParameters->push_back( (uint)level );
		pParameters->push_back( threadId );
		pParameters->push_back( time );
		pParameters->push_back( (uint)userId );
		constexpr string_view procedure = "log_message_insert"sv;
		constexpr string_view args = "(?,?,?,?,?,?,?,?,?,?"sv;
		ostringstream os; 
		os << procedure;
		if( variableCount>0 )
			os << variableCount;
		os << args;
		for( uint i=0; i<variableCount; ++i )
		{
			os << ",?";
			pParameters->push_back( variables[i] );
		}
		os << ")";
		_pDbQueue->Push( os.str(), pParameters );
	}
}
