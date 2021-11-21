#pragma once
#include "WebSocketAsync.h"
#include "WebSession.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include <jde/log/types/proto/FromServer.pb.h>

namespace Jde::ApplicationServer::Web
{
	struct WebServer final : WebSocket::TListener<FromServer::Transmission,MySession>
	{
		using base=WebSocket::TListener<FromServer::Transmission,MySession>;
		WebServer( PortType port )noexcept;

		ⓣ UpdateStatus( const T& app )noexcept->void;
		α SendStatuses( up<FromServer::Statuses> pAllocated )noexcept(false)->void;
		α SetStatus( FromServer::Status& status )const noexcept->void;
		α AddStatusSession( WebSocket::SessionPK id )noexcept{ _statusSessions.emplace(id); }
		α RemoveStatusSession( WebSocket::SessionPK id )noexcept{ _statusSessions.erase(id); }
		α RemoveSession( WebSocket::SessionPK id )noexcept->void override;
		α AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level )noexcept->bool;
		α RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )noexcept->void;
		α PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )noexcept->ELogLevel;
	private:
		UnorderedSet<WebSocket::SessionPK> _statusSessions;
		flat_map<ApplicationPK,flat_map<WebSocket::SessionPK,ELogLevel>> _logSubscriptions; shared_mutex _logSubscriptionMutex;
	};
	α Server()noexcept->WebServer&;

	ⓣ WebServer::UpdateStatus( const T& app )noexcept->void
	{
		auto pStatuses = make_unique<FromServer::Statuses>();
		app.SetStatus( *pStatuses->add_values() );
		SendStatuses( move(pStatuses) );
	}
}