#include "Server.h"
#include <ranges>
#include <jde/web/flex/Flex.h>
#include <jde/appClient/proto/App.FromServer.h>
#include "usings.h"
#include <jde/appClient/AppClient.h>
#include <jde/appClient/Sessions.h>
#include "ServerSocketSession.h"

#define var const auto
namespace Jde::App{
	optional<std::jthread> _serverThread;
	concurrent_flat_map<AppInstancePK,sp<ServerSocketSession>> _sessions;
	concurrent_flat_map<AppPK,flat_map<AppInstancePK,ELogLevel>> _logSubscriptions;
	concurrent_flat_map<AppInstancePK,Proto::FromServer::Status> _statuses;//TODO! remove on close.
	concurrent_flat_set<AppInstancePK> _statusSubscriptions;

	AppPK _appId;
	AppInstancePK _instanceId;
}
namespace Jde{
	α App::AppId()ι->AppPK{ return _appId; }
	α App::SetAppId( AppPK x )ι->void{ _appId=x; }
	α App::InstanceId()ι->AppInstancePK{ return _instanceId;}
	α App::SetInstanceId( AppInstancePK x )ι->void{ _instanceId=x; }

	α App::StartWebServer()ι->void{
		_serverThread = std::jthread{ []{
			Web::Flex::Start( ms<RequestHandler>() ); //blocking
		} };
		while( !Web::Flex::HasStarted() )
			std::this_thread::sleep_for( 100ms );
	}

	α App::StopWebServer()ι->void{
		Web::Flex::Stop();
		if( _serverThread && _serverThread->joinable() ){
			_serverThread->join();
			_serverThread = {};
		}
	}
	α App::RemoveSession( AppInstancePK instancePK )ι->void{
		_logSubscriptions.visit_all( [&]( auto&& kv ){ kv.second.erase( instancePK );} );
		_logSubscriptions.erase_if( []( auto&& kv ){ return kv.second.empty(); } );
		_statusSubscriptions.erase( instancePK );
		bool erased = _sessions.erase_if( instancePK, [&]( auto&& kv ){
			_statuses.erase( kv.second->InstancePK() );
			return true;
		});
		CustomRequestAwait::OnCloseConnection( instancePK );
		TRACET( AppTag(), "[{:x}]RemoveSession erased: {}", appInstancePK, erased );
	}

	α App::FindApplications( str name )ι->vector<Proto::FromClient::Instance>{
		vector<Proto::FromClient::Instance> y;
		_sessions.visit_all( [&]( auto&& kv ){
			auto& session = kv.second;
		 	if( session->Instance().application()==name )
				y.push_back( session->Instance() );
		} );
		return y;
	}
	α App::BroadcastLogEntry( LogPK id, AppPK appId, AppInstancePK instancePK, const Proto::FromClient::LogEntry& m, const vector<string>& args )ι->ELogLevel{
		auto minLevel{ ELogLevel::Critical };
		_logSubscriptions.cvisit( appId, [&]( var& kv ){
			var& sessions = kv.second;
			for( var& [sessionId,sessionLevel] : sessions | std::views::filter([=](auto&& kv){return (uint8)m.level()>=(uint8)kv.second;}) ){
				minLevel = (ELogLevel)std::min( (int8)minLevel, (int8)sessionLevel );
				_sessions.visit( sessionId, [&](auto&& kv){
					kv.second->Write( FromServer::TraceTransmission(id, appId, instancePK, m, args) );
				} );
			}
		} );
		return minLevel;
	}
	α App::BroadcastStatus( AppPK appId, AppInstancePK instancePK, str hostName, Proto::FromClient::Status&& status )ι->void{
		auto value{ FromServer::Status(appId, instancePK, hostName, move(status)) };
		_statuses.emplace_or_visit( instancePK, value, [&](auto& kv){ kv.second = value; } );
		_statusSubscriptions.visit_all( [&](auto subInstancePK){
			_sessions.visit( subInstancePK, [&](auto&& kv){
				kv.second->Write( FromServer::StatusTransmission(value) );
			});
		});
	}
	namespace App{
		α Server::Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε{
			_sessions.visit_while( [&]( auto&& kv ){
				auto& session = kv.second;
				auto appInstPK = session->AppPK()==appPK ? session->InstancePK() : 0;
				var found = appInstPK && appInstPK==instancePK.value_or( appInstPK );
				if( found )
					session->Write( move(msg) );
				return !found;
			});
		}

		α RequestHandler::RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->void{
			auto pSession = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
			_sessions.emplace( pSession->Id(), pSession );
			pSession->Run();
		}
	}
}
