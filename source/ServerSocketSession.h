#pragma once
#include <jde/web/client/usings.h>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/Sessions.h>
#include "await/ForwardExecutionAwait.h"

namespace Jde::App{
	using namespace Jde::Web::Server;
	//using namespace Jde::Http;
	struct ServerSocketSession : TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>{
		using base = TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>;
		ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α AppPK()Ι->AppPK{ return _appPK; }
		α Instance()Ι->const Proto::FromClient::Instance&{ return _instance; }
		α InstancePK()Ι->AppInstancePK{ return _instancePK; }
		α OnRead( Proto::FromClient::Transmission&& transmission )ι->void override;
	private:
		α OnClose()ι->void;
		//α OnConnect( SessionPK sessionId, RequestId requestId )ι->Web::UpsertAwait::Task;
		α ProcessTransmission( Proto::FromClient::Transmission&& transmission, optional<UserPK> userPK, optional<RequestId> clientRequestId )ι->void;
		α SharedFromThis()ι->sp<ServerSocketSession>{ return std::dynamic_pointer_cast<ServerSocketSession>(shared_from_this()); }
		α WriteException( IException&& e )ι->void override{ WriteException( move(e), 0 ); }
		α WriteException( IException&& e, RequestId requestId )ι->void;

		α AddSession( Proto::FromClient::AddSession&& addSession, RequestId clientRequestId, SL sl )ι->Task;
		α Execute( string&& bytes, optional<UserPK> userPK, RequestId clientRequestId )ι->void;
		α ForwardExecution( Proto::FromClient::ForwardExecution&& clientMsg, bool anonymous, RequestId clientRequestId, SRCE )ι->ForwardExecutionAwait::Task;
		α GraphQL( string&& query, uint requestId )ι->Task;
		α SaveLogEntry( Proto::FromClient::LogEntry logEntry, RequestId requestId )->void;
		α SendAck( uint id )ι->void override;
		α SessionInfo( SessionPK sessionId, RequestId requestId )ι->void;
		α SetSessionId( SessionPK sessionId, RequestId requestId )->Web::Server::Sessions::UpsertAwait::Task;

		Proto::FromClient::Instance _instance;
		App::AppPK _appPK{};
		AppInstancePK _instancePK{};
		optional<UserPK> _userPK{};
		ELogLevel _webLevel{ ELogLevel::NoLog };
		ELogLevel _dbLevel{ ELogLevel::NoLog };
	};
}