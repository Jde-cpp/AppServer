#include "Server.h"
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/Sessions.h>
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/db/graphQL/FilterQL.h>
#include "../../Framework/source/coroutine/Alarm.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Framework/source/db/GraphQuery.h"
#include <jde/web/server/Flex.h>
#include "LogData.h"
#include "ServerSocketSession.h"
#include "await/GraphQLAwait.h"
//#include "usings.h"

#define var const auto
namespace Jde::App{
	concurrent_flat_map<AppInstancePK,sp<ServerSocketSession>> _sessions;
	concurrent_flat_map<AppInstancePK,DB::FilterQL> _logSubscriptions;
	concurrent_flat_map<AppInstancePK,Proto::FromServer::Status> _statuses;
	concurrent_flat_set<AppInstancePK> _statusSubscriptions;

	AppPK _appId;
	AppInstancePK _instanceId;
	atomic<RequestId> _requestId{0};

	struct ApplicationServer final : Web::Server::IApplicationServer{
		α GraphQL( string&& q, UserPK userPK, SL sl )ι->up<TAwait<json>> override{ return mu<Server::GraphQLAwait>( move(q), userPK, sl ); }
		α SessionInfoAwait( SessionPK, SL )ι->up<TAwait<App::Proto::FromServer::SessionInfo>> override{ return {}; }
	};
}
namespace Jde{
	α App::GetAppPK()ι->AppPK{ return _appId; }
	α App::SetAppPK( AppPK x )ι->void{ _appId=x; }
	α App::InstancePK()ι->AppInstancePK{ return _instanceId;}
	α App::SetInstancePK( AppInstancePK x )ι->void{ _instanceId=x; }
	α UpdateStatuses()ι->Task;
	α App::StartWebServer()ε->void{
		Web::Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
		Process::AddShutdownFunction( [](bool /*terminate*/){App::StopWebServer();} );//TODO move to Web::Server
		UpdateStatuses();
	}

	α App::StopWebServer()ι->void{
		Web::Server::Stop();
	}

	α UpdateStatuses()ι->Task{
		while( !Process::ShuttingDown() ){
			App::Server::BroadcastAppStatus();
			co_await Threading::Alarm::Wait( 1min );
		}
	}

	namespace App{
		α TestLogPub( const DB::FilterQL& subscriptionFilter, AppPK /*appId*/, AppInstancePK /*instancePK*/, const Logging::ExternalMessage& m )ι->bool{
			bool passesFilter{ true };
			var logTags = ELogTags::Socket | ELogTags::Server | ELogTags::Subscription;
			for( var [jsonColName, columnFilters] : subscriptionFilter.ColumnFilters ){
				if( jsonColName=="level" )
					passesFilter = DB::FilterQL::Test( underlying(m.Level), columnFilters, logTags );
				else if( jsonColName=="time" )
					passesFilter = DB::FilterQL::Test( m.TimePoint, columnFilters, logTags );
				else if( jsonColName=="message" )
					passesFilter = DB::FilterQL::Test( m.MessageView, columnFilters, logTags );
				else if( jsonColName=="file" )
					passesFilter = DB::FilterQL::Test( string{m.File}, columnFilters, logTags );
				else if( jsonColName=="function" )
					passesFilter = DB::FilterQL::Test( string{m.Function}, columnFilters, logTags );
				else if( jsonColName=="line" )
					passesFilter = DB::FilterQL::Test( m.LineNumber, columnFilters, logTags );
				else if( jsonColName=="user_pk" )
					passesFilter = DB::FilterQL::Test( m.UserPK, columnFilters, logTags );
				else if( jsonColName=="thread_Id" )
					passesFilter = DB::FilterQL::Test( m.ThreadId, columnFilters, logTags );
				// else if( jsonColName=="tags" ) TODO
				// 	passesFilter = DB::FilterQL::Test( m.Tags(), columnFilters, logTags );
				// else if( jsonColName=="args" ) TODO
				// 	passesFilter = DB::FilterQL::Test( m.Args, columnFilters, logTags );
				if( !passesFilter )
					break;
			}
			return passesFilter;
		}

		α Server::BroadcastLogEntry( LogPK id, AppPK logAppPK, AppInstancePK logInstancePK, const Logging::ExternalMessage& m, const vector<string>& args )ι->void{
			_logSubscriptions.cvisit_all( [&]( var& kv ){
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
				var found = appInstPK && appInstPK==instancePK.value_or( appInstPK );
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
			auto ql = DB::ParseQL( qlText );
			auto tables = ql.index()==0 ? get<0>( move(ql) ) : vector<DB::TableQL>{};
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
}
