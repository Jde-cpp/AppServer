#include "ServerSocketSession.h"
#include <jde/appClient/proto/App.FromServer.h>
#include <jde/appClient/proto/App.FromClient.h>
#include "LogData.h"
#include "Server.h"
#include "Cache.h"
#define var const auto

namespace Jde::App{
	ServerSocketSession::ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α ForwardExecution( RequestId optional<UserPK> userPK, Proto::FromClient::ForwardExecution execution, RequestId clientRequestId, RequestId sessionRequestId, sp<IServerSocketSession> serverSocketSession )ι->CustomRequestAwait::Task{
		try{
			string result = co_await CustomRequestAwait{ userPK, execution, sessionRequestId, serverSocketSession };
			Write( FromServer::ExecuteResponseTransmission(clientRequestId, move(result)) );
		}
		catch( IException& e ){
			serverSocketSession->WriteException( clientRequestId, e );
		}
	}
	α ServerSocketSession::OnRead( Proto::FromClient::Transmission&& transmission )ι->void{
		uint cLog{}, cString{};
		for( auto i=0; i<transmission.messages_size(); ++i ){
			auto m = transmission.mutable_messages( i );
			using enum Proto::FromClient::Message::ValueCase;
			var requestId = m->request_id();
			switch( m->Value_case() ){
			[[unlikely]]case kInstance:{
				_instance = Proto::FromClient::Instance{ move(*m->mutable_instance()) };
				var [appPK,instancePK, dbLogLevel_, fileLogLevel_] = AddInstance( _instance.application(), _instance.host(), _instance.pid() );//TODO Don't block
				INFOT( SocketServerReceivedTag(), "[{:x}]Adding application app:{}@{} pid:{} instancePK:{:x}", Id(), _instance.application(), _instance.host(), _instance.pid(), instancePK );//TODO! add sessionId  endpoint:{}
				_instancePK = instancePK; _appPK = appPK;
				//WriteStrings(); Before this was sending down file/functions/messages/etc for every application.
				break;}
			[[likely]]case kLogEntry:{
				if( !_appPK || !_instancePK ){
					WriteException( Exception("ApplicationId or InstanceId not set.", ELogLevel::Warning) );
					continue;
				}
				++cLog;
				auto& logEntry = *m->mutable_log_entry();
				var level = (ELogLevel)logEntry.level();
				vector<string> args = IO::Proto::ToVector( move(*logEntry.mutable_args()) );
				if( _dbLevel!=ELogLevel::NoLog && _dbLevel<=level )
					SaveMessage( _appPK, _instancePK, logEntry, &args );//TODO don't block
				if( _webLevel!=ELogLevel::NoLog && _webLevel<=level )
					_webLevel = BroadcastLogEntry( 0, _appPK, _instancePK, logEntry, move(args) );
				break;}
			[[likely]]case kString:{
				if( !_appPK || !_instancePK ){
					WriteException( Exception("ApplicationId or InstanceId not set.", ELogLevel::Warning) );
					continue;
				}
				++cString;
				auto& s = *m->mutable_string();
				App::Cache::Add( _appPK, s.field(), s.id(), move(s.value()) );
				break;}
			[[likely]]case kStatus:{
				auto& status = *m->mutable_status();
				//:10L
				LogReceived( Jde::format("Status memory: {:10L}", (uint)status.memory()) );//https://stackoverflow.com/questions/58938378/is-it-possible-to-format-number-with-thousands-separator-using-fmt
				BroadcastStatus( _appPK, _instancePK, _instance.host(), move(status) );
				break;}
			case kForwardExecution:
			case kForwardExecutionAnonymous:{
				var anonymous = m->Value_case()==kForwardExecutionAnonymous;
				auto forward = anonymous ? m->mutable_forward_execution_anonymous() : m->mutable_forward_execution();
				LogReceived( Jde::Format("kForwardExecution{} appPK: {}, appInstancePK: {:x}, size: {:10L}", anonymous ? "Anonymous" : "", forward->app_pk(), forward->app_instance_pk(), forward->executionTransmission.size()) );
				ForwardExecution( ++_requestId, anonymous ? nullopt : UserPK, move(*m->mutable_forward_execution()), shared_from_this() );
				break;}

					// auto& custom = *pMessage->mutable_custom();
					// LogReceived( "Custom size: {:10L}", custom.size() );
					// CustomFunction<Logging::Proto::CustomMessage> fnctn = []( Web::MySession& webSession, uint a, Logging::Proto::CustomMessage&& b ){ webSession.WriteCustom((uint32)a, b.message()); };
					// SendCustomToWeb<Logging::Proto::CustomMessage>( move(), fnctn );
				//auto& custom = *m->mutable_custom();
				//LogReceived( "Received Custom." );
//				break;}
			}
		}
	}

	α ServerSocketSession::OnClose()ι->void{
		RemoveSession( Id() );
		base::OnClose();
	}
	α ServerSocketSession::WriteException( const IException& e )ι->void{
		Write( FromServer::ExceptionTransmission(e) );
	}

}