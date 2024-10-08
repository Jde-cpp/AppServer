#include "ServerSocketSession.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Framework/source/um/UM.h"
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/app/shared/StringCache.h>
#include "LogData.h"
#include "WebServer.h"
#define var const auto

namespace Jde::App{
	α ToProto( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission;

	ServerSocketSession::ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α ServerSocketSession::AddSession( Proto::FromClient::AddSession m, RequestId requestId, SL sl )ι->Task{
		var _ = shared_from_this();
		try{
			LogRead( Ƒ("AddSession user: '{}', endpoint: '{}', provider: {}, is_socket: {}", m.domain()+"/"+m.login_name(), m.user_endpoint(), m.provider_pk(), m.is_socket()), requestId );
			var userPK = *( co_await UM::Login(m.login_name(), m.provider_pk(), m.domain()) ).UP<UserPK>();

			auto sessionInfo = Web::Server::Sessions::Add( userPK, move(*m.mutable_user_endpoint()), m.is_socket() );
			LogWrite( Ƒ("AddSession id: {:x}", sessionInfo->SessionId), requestId );
			Write( ToProto(*sessionInfo, requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::Execute( string&& bytes, optional<UserPK> userPK, RequestId clientRequestId )ι->void{
		try{
			auto t = IO::Proto::Deserialize<Proto::FromClient::Transmission>( move(bytes) );
			ProcessTransmission( move(t), userPK, clientRequestId );
		}
		catch( IException& e ){
			WriteException( move(e), clientRequestId );
		}
	}

	α ServerSocketSession::ForwardExecution( Proto::FromClient::ForwardExecution&& m, bool anonymous, RequestId requestId, SL sl )ι->ForwardExecutionAwait::Task{
		sv functionSuffix = anonymous ? "Anonymous" : "";
		LogRead( Ƒ("ForwardExecution{} appPK: {}, appInstancePK: {:x}, size: {:10L}", functionSuffix, m.app_pk(), m.app_instance_pk(), m.execution_transmission().size()), requestId );
		try{
			string result = co_await ForwardExecutionAwait{ _userPK.value_or(0), move(m), SharedFromThis(), sl };
			LogWrite( Ƒ("ForwardExecution{} size: {:10L}", functionSuffix, result.size()), requestId );
			Write( FromServer::Execute(move(result), requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::GraphQL( string&& query, RequestId requestId )ι->Task{
		var _ = shared_from_this();
		try{
			LogRead( Ƒ("GraphQL: {}", query), requestId );
			auto j = ( co_await DB::CoQuery(move(query), 0, "Sock::GraphQL") ).UP<json>();
			auto y = j->dump();
			LogWrite( Ƒ("GraphQL: {}", y.substr(0,100)), requestId );
			Write( FromServer::GraphQL(move(y), requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}
	α ServerSocketSession::SaveLogEntry( Proto::FromClient::LogEntry l, RequestId requestId )->void{
		if( !_appPK || !_instancePK ){
			WriteException( Exception{"ApplicationId or InstanceId not set.", ELogLevel::Warning}, requestId );
			return;
		}
		var level = (ELogLevel)l.level();
		vector<string> args = IO::Proto::ToVector( move(*l.mutable_args()) );
		if( _dbLevel!=ELogLevel::NoLog && _dbLevel<=level )
			SaveMessage( _appPK, _instancePK, l, &args );//TODO don't block
		if( _webLevel!=ELogLevel::NoLog && _webLevel<=level ){
			Logging::ExternalMessage y{ Logging::MessageBase{ (ELogLevel)l.level(), l.message_id(), l.file_id(), l.function_id(), l.line(), l.user_pk(), l.thread_id()}, IO::Proto::ToVector(l.args()), IO::Proto::ToTimePoint(l.time()) };
			using enum Logging::EFields;
			y._pMessage = mu<string>( StringCache::GetMessage(l.message_id()) );
			y.MessageView = *y._pMessage;
			y._fileName = StringCache::GetFile( l.file_id() );
			y.File = y._fileName.c_str();
			//y.Function = Cache::AppStrings().Get( Function, l.function_id() ); TODO function is char* and no string to hold it.

			Server::BroadcastLogEntry( 0, _appPK, _instancePK, y, move(args) );
		}
	}
	α ServerSocketSession::SendAck( uint32 id )ι->void{
		Write( FromServer::Ack(id) );
	}

	α ServerSocketSession::SessionInfo( SessionPK sessionId, RequestId requestId )ι->void{
		LogRead( Ƒ("SessionInfo={:x}", sessionId), requestId );
		if( auto info = Web::Server::Sessions::Find( sessionId ); info ){
			LogWrite( Ƒ("SessionInfo userPK: {}, endpoint: {}, hasSocket: {}", info->UserPK, info->UserEndpoint, info->HasSocket), requestId );
			Write( ToProto(move(*info), requestId) );
		}else
			WriteException( Exception{"Session not found."}, requestId );
	}
	α ServerSocketSession::SetSessionId( SessionPK sessionId, RequestId requestId )->Web::Server::Sessions::UpsertAwait::Task{
		try{
			LogRead( Ƒ("SetSessionId={:x}", sessionId), requestId );
			co_await Web::Server::Sessions::UpsertAwait( Ƒ("{:x}", sessionId), _userEndpoint.address().to_string(), true );
			base::SetSessionId( sessionId );
			Write( FromServer::Complete(requestId) );
		}
		catch( IException& e ){
			WriteException( move(e), requestId );
		}
	}


	α ServerSocketSession::OnRead( Proto::FromClient::Transmission&& t )ι->void{
		ProcessTransmission( move(t), _userPK, nullopt );
	}

	α ServerSocketSession::ProcessTransmission( Proto::FromClient::Transmission&& transmission, optional<UserPK> /*userPK*/, optional<RequestId> clientRequestId )ι->void{
		uint cLog{}, cString{};
		if( transmission.messages_size()==0 )
			LogRead( "No messages in transmission.", 0, ELogLevel::Error );

		for( auto i=0; i<transmission.messages_size(); ++i ){
			auto& m = *transmission.mutable_messages( i );
			using enum Proto::FromClient::Message::ValueCase;
			var requestId = clientRequestId.value_or( m.request_id() );
			switch( m.Value_case() ){
			[[unlikely]]case kInstance:{
				_instance = move( *m.mutable_instance() );
				var [appPK,instancePK, dbLogLevel_, fileLogLevel_] = AddInstance( _instance.application(), _instance.host(), _instance.pid() );//TODO Don't block
				Information{ ELogTags::SocketServerRead, "[{:x}]Adding application app:{}@{}:{} pid:{}, instancePK:{:x}, sessionId: {:x}, endpoint: '{}'", Id(), _instance.application(), _instance.host(), _instance.web_port(), _instance.pid(), instancePK, _instance.session_id(), _userEndpoint.address().to_string() };

				_instancePK = instancePK; _appPK = appPK;
				//WriteStrings(); Before this was sending down file/functions/messages/etc for every application.
				break;}
			case kAddSession:{
				AddSession( move(*m.mutable_add_session()), requestId, SRCE_CUR );
				break;}
			case kException:
				if( !requestId )
					Debug( ELogTags::SocketServerRead | ELogTags::Exception, "[{:x}.{:x}]Exception - {}", Id(), 0, m.exception().what() );
				else if( !ForwardExecutionAwait::Resume( move(*m.mutable_execute_response()), requestId) )
					LogRead( Ƒ("Exception not handled - {}", m.exception().what()), requestId, ELogLevel::Critical );
				break;
			case kExecute:
			case kExecuteAnonymous:{
				bool isAnonymous = m.Value_case()==kExecuteAnonymous;
				auto bytes = isAnonymous ? move( *m.mutable_execute_anonymous() ) : move( *m.mutable_execute()->mutable_transmission() );
				optional<UserPK> executor = m.Value_case()==kExecuteAnonymous ? nullopt : optional<UserPK>(m.execute().user_pk() );
				LogRead( Ƒ("Execute{} size: {:10L}", isAnonymous ? "Anonymous" : "", bytes.size()), requestId );
				Execute( move(bytes), executor, requestId );
				break;}
			case kExecuteResponse:
				if( !ForwardExecutionAwait::Resume( move(*m.mutable_execute_response()), requestId) )
					LogRead( Ƒ("ExecuteResponse requestId:{} not found.", requestId), requestId, ELogLevel::Critical );
				break;
			case kForwardExecution:
			case kForwardExecutionAnonymous:{
				var anonymous = m.Value_case()==kForwardExecutionAnonymous;
				auto forward = anonymous ? m.mutable_forward_execution_anonymous() : m.mutable_forward_execution();
				ForwardExecution( move(*forward), anonymous, requestId );
				break;}
			[[likely]]case kGraphQl:
				GraphQL( move(*m.mutable_graph_ql()), requestId );
				break;
			[[likely]]case kLogEntry:
				++cLog;
				SaveLogEntry( move(*m.mutable_log_entry()), requestId );
				break;
			case kSessionId:
				if( !m.session_id() )
					WriteException( Exception{"SessionId not set."}, requestId );
				else
					SetSessionId( m.session_id(), requestId );
			break;
			case kSessionInfo:
				SessionInfo( m.session_info(), requestId );
				break;
			[[likely]]case kStatus:{
				auto& status = *m.mutable_status();
				//:10L
				LogRead( "Status", requestId );
				Server::BroadcastStatus( _appPK, _instancePK, _instance.host(), move(status) );
				break;}
			[[likely]]case kStringValue:{
				if( !_appPK || !_instancePK ){
					WriteException( Exception{"ApplicationId or InstanceId not set.", ELogLevel::Warning}, requestId );
					continue;
				}
				++cString;
				auto& s = *m.mutable_string_value();
				if( StringCache::Add( s.field(), s.id(), s.value(), ELogTags::SocketServerRead) )
					Server::SaveString( (Proto::FromClient::EFields)s.field(), s.id(), move(*s.mutable_value()) );
				break;}
			case kSubscribeLogs:{
				if( m.subscribe_logs().empty() ){
					LogRead( Ƒ("SubscribeLogs unsubscribe"), requestId );
					Server::UnsubscribeLogs( InstancePK() );
				}
				else{
					try{
						LogRead( Ƒ("SubscribeLogs subscribe - {}", m.subscribe_logs()), requestId );
						Server::SubscribeLogs( move(*m.mutable_subscribe_logs()), SharedFromThis() );
					}
					catch( IException& e ){
						WriteException( move(e), requestId );
					}
				}
				break;}
			case kSubscribeStatus:
				LogRead( Ƒ("SubscribeStatus - {}", m.subscribe_status()), requestId );
				if( m.subscribe_status() )
					Server::SubscribeStatus( *this );
				else
					Server::UnsubscribeStatus( InstancePK() );
				break;
			default:
				LogRead( Ƒ("Unknown message type '{}'", underlying(m.Value_case())), requestId, ELogLevel::Critical );
			}
		}
		if( cLog || cString )
			Trace{ ELogTags::SocketServerRead, "[{:x}] log entries recieved: {} strings received: {}.", Id(), cLog, cString };
	}

	α ServerSocketSession::OnClose()ι->void{
		LogRead( "OnClose", 0 );
		Server::RemoveSession( Id() );
		base::OnClose();
	}

	α ServerSocketSession::WriteException( IException&& e, RequestId requestId )ι->void{
		LogWriteException( e, requestId );
		Write( FromServer::Exception(move(e), requestId) );
	}

	α ToProto( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		auto& response = *m.mutable_session_info();
		*response.mutable_expiration() = IO::Proto::ToTimestamp( Chrono::ToClock<Clock,steady_clock>(session.Expiration) );
		response.set_session_id( session.SessionId );
		response.set_user_pk( session.UserPK );
		response.set_user_endpoint( session.UserEndpoint );
		response.set_has_socket( session.HasSocket );
		return t;
	}
}