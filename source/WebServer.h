#pragma once
#include "WebSocketAsync.h"
#include "WebSession.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include "types/proto/FromServer.pb.h"

namespace Jde::ApplicationServer::Web
{
	struct WebServer final : WebSocket::TListener<FromServer::Transmission,MySession>
	{
		using base=WebSocket::TListener<FromServer::Transmission,MySession>;
		WebServer( PortType port )noexcept;

		//static sp<MyServer> CreateInstance( uint16 port )noexcept;
		ⓣ UpdateStatus( const T& app )noexcept->void;
		void SendStatuses( up<FromServer::Statuses> pAllocated )noexcept(false);
		void SetStatus( FromServer::Status& status )const noexcept;
		void AddStatusSession( WebSocket::SessionPK id )noexcept{ _statusSessions.emplace(id); }
		void RemoveStatusSession( WebSocket::SessionPK id )noexcept{ _statusSessions.erase(id); }
		void RemoveSession( WebSocket::SessionPK id )noexcept override;
		bool AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level )noexcept;
		void RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )noexcept;
		ELogLevel PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )noexcept;
	private:
		UnorderedSet<WebSocket::SessionPK> _statusSessions;
		map<ApplicationPK,map<WebSocket::SessionPK,ELogLevel>> _logSubscriptions; shared_mutex _logSubscriptionMutex;
		//static sp<MyServer> _spInstance;
	};
	WebServer& Server()noexcept;

	ⓣ WebServer::UpdateStatus( const T& app )noexcept->void
	{
		auto pStatuses = make_unique<FromServer::Statuses>();
		app.SetStatus( *pStatuses->add_values() );
		SendStatuses( move(pStatuses) );
	}
}