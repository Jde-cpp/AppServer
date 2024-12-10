#include "WebServer.h"
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/Sessions.h>
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/ql/types/FilterQL.h>
#include "../../Framework/source/coroutine/Alarm.h"
#include <jde/ql/ql.h>
//#include "../../Framework/source/db/GraphQuery.h"
#include "LogData.h"
#include "ServerSocketSession.h"
//#include "await/GraphQLAwait.h"
//#include "usings.h"

#define let const auto
namespace Jde::App{
	using QL::FilterQL;
	concurrent_flat_map<AppInstancePK,sp<ServerSocketSession>> _sessions;
	concurrent_flat_map<AppInstancePK,FilterQL> _logSubscriptions;
	concurrent_flat_map<AppInstancePK,Proto::FromServer::Status> _statuses;
	concurrent_flat_set<AppInstancePK> _statusSubscriptions;

	AppPK _appId;
	AppInstancePK _instanceId;
	atomic<RequestId> _requestId{0};

	struct ApplicationServer final : Web::Server::IApplicationServer{
		α GraphQL( string&& q, UserPK userPK, SL sl )ι->up<TAwait<jvalue>> override{ return mu<QL::QLAwait>( move(q), userPK, sl ); }
		α SessionInfoAwait( SessionPK, SL )ι->up<TAwait<App::Proto::FromServer::SessionInfo>> override{ return {}; }
	};

	α Server::GetAppPK()ι->AppPK{ return _appId; }
	α Server::SetAppPK( AppPK x )ι->void{ _appId=x; }
	α Server::InstancePK()ι->AppInstancePK{ return _instanceId;}
	α Server::SetInstancePK( AppInstancePK x )ι->void{ _instanceId=x; }
	α UpdateStatuses()ι->Task;
	α Server::StartWebServer()ε->void{
		Web::Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
		Process::AddShutdownFunction( [](bool /*terminate*/){Server::StopWebServer();} );//TODO move to Web::Server
		UpdateStatuses();
	}

	α Server::StopWebServer()ι->void{
		Web::Server::Stop();
	}

	α UpdateStatuses()ι->Task{
		while( !Process::ShuttingDown() ){
			Server::BroadcastAppStatus();
			co_await Threading::Alarm::Wait( 1min );
		}
	}

	α TestLogPub( const FilterQL& subscriptionFilter, AppPK /*appId*/, AppInstancePK /*instancePK*/, const Logging::ExternalMessage& m )ι->bool{
		bool passesFilter{ true };
		let logTags = ELogTags::Socket | ELogTags::Server | ELogTags::Subscription;
		for( let [jsonColName, columnFilters] : subscriptionFilter.ColumnFilters ){
			if( jsonColName=="level" )
				passesFilter = FilterQL::Test( underlying(m.Level), columnFilters, logTags );
			else if( jsonColName=="time" )
				passesFilter = FilterQL::Test( m.TimePoint, columnFilters, logTags );
			else if( jsonColName=="message" )
				passesFilter = FilterQL::Test( string{m.MessageView}, columnFilters, logTags );
			else if( jsonColName=="file" )
				passesFilter = FilterQL::Test( string{m.File}, columnFilters, logTags );
			else if( jsonColName=="function" )
				passesFilter = FilterQL::Test( string{m.Function}, columnFilters, logTags );
			else if( jsonColName=="line" )
				passesFilter = FilterQL::Test( m.LineNumber, columnFilters, logTags );
			else if( jsonColName=="user_pk" )
				passesFilter = FilterQL::Test( m.UserPK, columnFilters, logTags );
			else if( jsonColName=="thread_Id" )
				passesFilter = FilterQL::Test( m.ThreadId, columnFilters, logTags );
			// else if( jsonColName=="tags" ) TODO
			// 	passesFilter = FilterQL::Test( m.Tags(), columnFilters, logTags );
			// else if( jsonColName=="args" ) TODO
			// 	passesFilter = FilterQL::Test( m.Args, columnFilters, logTags );
			if( !passesFilter )
				break;
		}
		return passesFilter;
	}

	α Server::BroadcastLogEntry( LogPK id, AppPK logAppPK, AppInstancePK logInstancePK, const Logging::ExternalMessage& m, const vector<string>& args )ι->void{
		_logSubscriptions.cvisit_all( [&]( let& kv ){
			if( TestLogPub(kv.second, id, logAppPK, m) ){
				_sessions.visit( kv.first, [&](auto&& kv){
						kv.second->Write( FromServer::TraceBroadcast(id, logAppPK, logInstancePK, m, args) );
				});
			}
		});
	}
	α Server::BroadcastStatus( AppPK appPK, AppInstancePK statusInstancePK, str hostName, Proto::FromClient::Status&& status )ι->void{
		auto value{ FromServer::ToStatus(appPK, statusInstancePK, hostName, move(status)) };
		_statuses.emplace_or_visit( statusInstancePK, value, [&](auto& kv){ kv.second = value; } );
		_statusSubscriptions.visit_all( [&](auto subInstancePK){
			_sessions.visit( subInstancePK, [&](auto&& kv){
				kv.second->Write( FromServer::StatusBroadcast(value) );
			});
		});
	}
	α Server::BroadcastAppStatus()ι->void{
		FromClient::Status( {} );
		BroadcastStatus( GetAppPK(), InstancePK(), IApplication::HostName(), FromClient::ToStatus({}) );
	}
	α Server::FindApplications( str name )ι->vector<Proto::FromClient::Instance>{
		vector<Proto::FromClient::Instance> y;
		_sessions.visit_all( [&]( auto&& kv ){
			auto& session = kv.second;
			if( session->Instance().application()==name )
				y.push_back( session->Instance() );
		} );
		return y;
	}

	α Server::FindInstance( AppInstancePK instancePK )ι->sp<ServerSocketSession>{
		sp<ServerSocketSession> y;
		_sessions.visit( instancePK, [&]( auto&& kv ){ y=kv.second; } );
		return y;
	}

	α Server::NextRequestId()->RequestId{ return ++_requestId; }
	α Server::Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε->void{
		if( !_sessions.visit_while( [&]( auto&& kv ){
			auto& session = kv.second;
			auto appInstPK = session->AppPK()==appPK ? session->InstancePK() : 0;
			let found = appInstPK && appInstPK==instancePK.value_or( appInstPK );
			if( found )
				session->Write( move(msg) );
			return !found;
		}) ){
			THROW( "No session found for appPK:{}, instancePK:{}", appPK, instancePK.value_or(0) );
		}
	}
	α Server::RemoveSession( AppInstancePK instancePK )ι->void{
		_logSubscriptions.erase( instancePK );
		UnsubscribeStatus( instancePK );
		UnsubscribeLogs( instancePK );
		bool erased = _sessions.erase_if( instancePK, [&]( auto&& kv ){
			_statuses.erase( kv.second->InstancePK() );
			return true;
		});
		ForwardExecutionAwait::OnCloseConnection( instancePK );
		Trace( ELogTags::App, "[{:x}]RemoveSession erased: {}", instancePK, erased );
	}

	α Server::SubscribeLogs( string&& qlText, sp<ServerSocketSession> session )ε->void{
		auto ql = QL::Parse( qlText );
		auto tables = ql.index()==0 ? get<0>( move(ql) ) : vector<QL::TableQL>{};
		THROW_IF( tables.size()!=1, "Invalid query, expecting single table" );
		auto table = move( tables.front() );
		THROW_IF( table.JsonName!="logs", "Invalid query, expecting logs query" );
		auto filter = table.Filter();//
		auto traces = Data::LoadEntries( table );//TODO awaitable
		_logSubscriptions.emplace( session->InstancePK(), move(filter) );
		traces.add_values();//Signify end.

		session->LogWrite( "SubscribeLogs existing_count: {}", traces.values_size()-1 );
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_traces() = move( traces );
		session->Write( move(t) );
	}

	α Server::SubscribeStatus( ServerSocketSession& session )ι->void{
		_statusSubscriptions.emplace( session.InstancePK() );
		Proto::FromServer::Transmission t;
		_statuses.visit_all( [&](auto&& kv){
			*t.add_messages()->mutable_status() = kv.second;
		});
		if( t.messages_size() ){
			session.LogWrite( "Writing {} statuses.", t.messages_size() );
			session.Write( move(t) );
		}
	}
	α Server::UnsubscribeLogs( AppInstancePK instancePK )ι->bool{
		return _logSubscriptions.erase( instancePK );
	}
	α Server::UnsubscribeStatus( AppInstancePK instancePK )ι->bool{
		return _statusSubscriptions.erase( instancePK );
	}

	α RequestHandler::RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->void{
		auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		session->Run();
	}
}