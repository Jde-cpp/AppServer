#pragma once
#include "../../Public/src/web/WebSocket.h"
#include "WebSession.h"
#include "../../Framework/source/collections/UnorderedSet.h"


namespace Jde::ApplicationServer::Web
{
	using WebSocket::SessionPK;

	struct WebServer final : WebSocket::TListener<FromServer::Transmission,MySession>
	{
		using base=WebSocket::TListener<FromServer::Transmission,MySession>;
		WebServer( PortType port )ι;

		Ŧ UpdateStatus( const T& app )ι->void;
		α SendStatuses( up<FromServer::Statuses> pAllocated )ε->void;
		α SetStatus( FromServer::Status& status )Ι->void;
		α AddStatusSession( SessionPK id )ι{ _statusSessions.emplace(id); }
		α RemoveStatusSession( SessionPK id )ι{ _statusSessions.erase(id); }
		α RemoveSession( SessionPK id )ι->void override;
		α AddLogSubscription( SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level )ι->bool;
		α RemoveLogSubscription( SessionPK sessionId, ApplicationInstancePK instanceId )ι->void;
		α PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )ι->ELogLevel;
		Ω CoSend( FromServer::MessageUnion&& m, SessionPK id )ι->PoolAwait;
		Ω Send( FromServer::MessageUnion&& m, SessionPK id )ι->void;
		α AddOutgoing( FromServer::MessageUnion&& msg, SessionPK id )ι->void;
		α AddOutgoing( const vector<FromServer::MessageUnion>& messages, SessionPK id )ι->void;

	private:
		UnorderedSet<SessionPK> _statusSessions;
		flat_map<ApplicationPK,flat_map<SessionPK,ELogLevel>> _logSubscriptions; shared_mutex _logSubscriptionMutex;
	};
	α Server()ι->WebServer&;

	namespace net = boost::asio;
	struct BeastException : public IException
	{
		BeastException( sv what, beast::error_code&& ec, ELogLevel level=ELogLevel::Trace, SRCE )noexcept;
		Ω IsTruncated( const beast::error_code& ec )noexcept{ return ec == net::ssl::error::stream_truncated; }
		Ω LogCode( const boost::system::error_code& ec, ELogLevel level, sv what )noexcept->void;

		using T=BeastException;
		α Clone()noexcept->sp<IException> override{ return ms<T>(move(*this)); }\
			α Move()noexcept->up<IException> override{ return mu<T>(move(*this)); }\
			α Ptr()->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }\
			[[noreturn]] α Throw()->void override{ throw move(*this); }
		beast::error_code ErrorCode;
	};

	Ŧ WebServer::UpdateStatus( const T& app )ι->void
	{
		auto pStatuses = mu<FromServer::Statuses>();
		app.SetStatus( *pStatuses->add_values() );
//		SendStatuses( move(pStatuses) );
	}
}