#include "LogData.h"
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/db/DBQueue.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/IRow.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>

#define let const auto

namespace Jde::App{
	sp<DB::DBQueue> _pDbQueue;
	static sp<LogTag> _logTag = Logging::Tag( "app" );
	sp<DB::AppSchema> _logSchema;
	constexpr ELogTags _tags{ ELogTags::App };
	Ω ds()ι->DB::IDataSource&{ return *_logSchema->DS(); }
	Ω instanceTableName()ε->string{ return _logSchema->GetView("app_instances").DBName; }

namespace Server{
	α ConfigureDSAwait::Suspend()ι->void{ Configure(); }
	α ConfigureDSAwait::Configure()ι->Access::ConfigureAwait::Task{
		sp<Access::IAcl> authorize = Access::LocalAcl();
		try{
			auto accessSchema = DB::GetAppSchema( "access", authorize );
			_logSchema = DB::GetAppSchema( "log", authorize );
			QL::Configure( {accessSchema, _logSchema} );
			if( auto sync = Settings::FindBool("/dbServers/sync").value_or(true); sync ){
				DB::SyncSchema( *accessSchema, QL::Local() );
				DB::SyncSchema( *_logSchema, QL::Local() );
			}
			auto await = Access::Configure( accessSchema, {accessSchema, _logSchema}, QL::Local(), UserPK{UserPK::System} );
			co_await await;
			_pDbQueue = ms<DB::DBQueue>( _logSchema->DS() );
			Process::AddShutdown( _pDbQueue );
			EndAppInstances();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ConfigureDSAwait::EndAppInstances()ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().ExecuteCo( {Ƒ("update {} set end_time=now() where end_time is null", instanceTableName())} );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

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
		let sql = Ƒ( "insert into {}(id,value)values(?,?)", table );
		auto pParameters = ms<vector<DB::Value>>();  pParameters->reserve(3);
		//ASSERT( Calc32RunTime(*pValue)==id );
		if( Calc32RunTime(value)!=id )
			return ERRX( "id '{}' does not match crc of '{}'", id, value );//locks itself on server log.
		pParameters->push_back( {id} );
		pParameters->push_back( {move(value)} );
		_pQueue->Push( sql, pParameters, false, sl );
	}
}
namespace Jde{
	α App::AddInstance( str applicationName, str hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK>{
		AppPK applicationId;
		AppInstancePK applicationInstanceId;
		auto fnctn = [&applicationId, &applicationInstanceId](const DB::IRow& row){
			applicationId = row.GetUInt32(0);
			applicationInstanceId = row.GetUInt32(1);
		};
		ds().ExecuteProc( "log_app_instance_insert(?,?,?)", {DB::Value{applicationName}, DB::Value{hostName}, DB::Value{processId}}, fnctn );

		return make_tuple( applicationId, applicationInstanceId );
	}
	α App::EndInstance( AppInstancePK instanceId, SL sl )ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().ExecuteCo( {Ƒ("update {} set end_time=now() where id=?", instanceTableName()), {DB::Value{instanceId}}}, sl );
		}
		catch( exception& )
		{}
	}

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
			string sql = id ? Ƒ("{} where id=?", baseSql) : string{baseSql};
			let params = id ? vector<DB::object>{id} : vector<DB::object>{};
			if( auto p = Datasource(); p )
				p->Select( sql, fnctn, params );
		} );
		return pApplications;
	}
*/
	α App::SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>* variables, SL sl )ι->void{
		let variableCount = std::min( (uint)5, variables ? variables->size() : 0 );
		auto pParameters = ms<vector<DB::Value>>(); pParameters->reserve( 10+variableCount );
		pParameters->push_back( {applicationId} );
		pParameters->push_back( {instanceId} );
		pParameters->push_back( {m.file_id()} );
		pParameters->push_back( {m.function_id()} );
		pParameters->push_back( {m.line()} );
		pParameters->push_back( {m.message_id()} );
		pParameters->push_back( {(uint8)m.level()} );
		pParameters->push_back( {m.thread_id()} );
		pParameters->push_back( {Jde::Proto::ToTimePoint(m.time())} );
		pParameters->push_back( {m.user_pk()} );
		constexpr sv procedure = "log_message_insert"sv;
		constexpr sv args = "(?,?,?,?,?,?,?,?,?,?"sv;
		std::ostringstream os;
		os << procedure;
		if( variableCount>0 )
			os << variableCount;
		os << args;
		for( uint i=0; i<variableCount; ++i ){
			os << ",?";
			pParameters->push_back( {(*variables)[i]} );
		}
		os << ")";
		_pQueue->Push( os.str(), pParameters, true, sl );
	}
	namespace App{
		α Data::LoadEntries( QL::TableQL table )ε->Proto::FromServer::Traces{
			auto statement = QL::SelectStatement( table, true );
			if( !statement )
				return {};
			flat_map<LogPK,Proto::FromServer::Trace> mapTraces;
			constexpr uint limit = 1000;
			statement->Limit( limit );
			auto where = statement->Where;
			auto rows = ds().Select( statement->Move() ); //TODO awaitable
			for( auto& row : rows ){
				auto t=FromServer::ToTrace( *row, table.Columns );
				auto id = t.id();
				mapTraces.emplace( id, move(t) );
			}
			if( mapTraces.size() ){
				auto addVariables = [&mapTraces]( DB::IRow& row ){
					let id = row.GetUInt32( 0 );
					if( auto pTrace = mapTraces.find(id); pTrace!=mapTraces.end() )
						*pTrace->second.add_args() = row.MoveString( 1 );
				};
				constexpr sv variableSql = "select log_id, value, variable_index from log_variables join logs on logs.id=log_variables.log_id";
				if( mapTraces.size()==limit ){
					where.Add( "logs.id<?" );
					where.Params().push_back( {mapTraces.rbegin()->first} );
				}
				ds().Select( Ƒ("{}\n{}\norder by log_id, variable_index", variableSql, where.Move()), addVariables, where.Params() );
			}
			Proto::FromServer::Traces traces;
			for( auto& [id,trace] : mapTraces )
				*traces.add_values() = move(trace);
			return traces;
		}

		Ω loadStrings( str tableName, SRCE )ε->concurrent_flat_map<uint,string>{
			let& table = _logSchema->GetTablePtr( tableName );
			DB::Statement statement{ table->Columns, DB::FromClause{DB::Join{table->GetPK(), _logSchema->GetTablePtr("entries")->GetPK(), true}}, {} };
			let rows = ds().Select( statement.Move(), false, sl );
			concurrent_flat_map<uint32,string> map;
			for( auto& row : rows )
				map.emplace( row->GetUInt(0), row->MoveString(1) );
			return map;
		}
		Ω loadFiles( SL sl )ε->concurrent_flat_map<uint32,string>{
			return loadStrings( "source_files", sl );
		}
		Ω loadFunctions( SL sl )ε->concurrent_flat_map<uint32,string>{
			return loadStrings( "source_functions", sl );
		}
		Ω loadMessages( SL sl )ε->concurrent_flat_map<uint32,string>{
			return loadStrings( "messages", sl );
		}

		α Data::LoadStrings( SL sl )ε->void{
			StringCache::Merge( loadFiles(sl), loadFunctions(sl), loadMessages(sl) );
		}
	}
}