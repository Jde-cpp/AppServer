#include "WebServer.h"
#include "LogClient.h"
#include "Listener.h"

#define var const auto
#define _listener Jde::ApplicationServer::TcpListener::GetInstance()
#define _logClient Logging::LogClient::Instance()
namespace Jde::ApplicationServer::Web
{
	static sp<LogTag> _logTag{ Logging::Tag("app.socket") };
	WebServer _instance{ Settings::Get<PortType>("web/socketPort").value_or(1967) };
	α Server()ι->WebServer&{ return _instance; }

	WebServer::WebServer( PortType port )ι:
		base{ port }{
		INFO( "WebSocket listening on port={}", port );
	}

	α WebServer::SendStatuses( up<FromServer::Statuses> pAllocated )ε->void{
		FromServer::Transmission t;
		t.add_messages()->set_allocated_statuses( pAllocated.release() );
		var data = IO::Proto::ToString( t );
		var sessions = _statusSessions.ToSet();
		for( var id : sessions )
		{
			shared_lock l{ _sessionMutex };
			if( auto pSession = _sessions.find(id); pSession!=_sessions.end() ){
				sp<IO::Sockets::ISession> p = pSession->second;
				static_pointer_cast<MySession::base>( p )->Write( mu<string>(move(data)) );
			}
			else
				_statusSessions.erase( id );
		}
	}
	α WebServer::SetStatus( FromServer::Status& status )const ι->void{
		status.set_application_id( (google::protobuf::uint32)Logging::Server::ApplicationId() );
		status.set_instance_id( (google::protobuf::uint32)Logging::Server::InstanceId() );
		status.set_host_name( IApplication::HostName() );
		status.set_start_time( (google::protobuf::uint32)Clock::to_time_t(IApplication::StartTime()) );
		status.set_db_log_level( (Web::FromServer::ELogLevel)Logging::ServerLevel() );
		status.set_file_log_level( (Web::FromServer::ELogLevel)Logging::Default().level() );
		status.set_memory( IApplication::MemorySize() );
		status.add_values( fmt::format("Web Connections:  {}", SessionCount()) );
	}
	α WebServer::RemoveSession( WebSocket::SessionPK id )ι->void{
		_statusSessions.erase( id );
		base::RemoveSession( id );
	}

	α WebServer::AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK /*instanceId*/, ELogLevel level )ι->bool{
		bool newSubscription;
		uint minLevel = (uint)ELogLevel::Critical;
		{
			unique_lock l{ _logSubscriptionMutex };
			auto& sessions = _logSubscriptions.try_emplace( applicationId, flat_map<WebSocket::SessionPK,ELogLevel>{} ).first->second;
			newSubscription = sessions.try_emplace( sessionId, level ).second;
			for( var& subscriber : sessions )
				minLevel = std::min(minLevel, (uint)subscriber.second );
		}
		_listener.WebSubscribe( applicationId, (ELogLevel)minLevel );

		return newSubscription;
	}
	α WebServer::RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )ι->void{
		uint minLevel = (uint)ELogLevel::Critical;
		{
			unique_lock l{ _logSubscriptionMutex };
			auto pSubscriptions = _logSubscriptions.find( instanceId );
			if( pSubscriptions!=_logSubscriptions.end() ){
				pSubscriptions->second.erase( sessionId );
				for( var& subscriber : pSubscriptions->second )
					minLevel = std::min(minLevel, (uint)subscriber.second );
				if( !pSubscriptions->second.size() )
					_logSubscriptions.erase( pSubscriptions );
				l.unlock();
				DBG("({}) removing log subscription for instance {}, new level={}."sv, sessionId, instanceId, minLevel );
			}
			else{
				l.unlock();
				DBG("({}) could not find existing subscription for instance {} logs."sv, sessionId, instanceId );
			}
		}
		_listener.WebSubscribe( instanceId, (ELogLevel)minLevel );
	}

	α WebServer::PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )ι->ELogLevel{
		unique_lock l{ _logSubscriptionMutex };
		auto minLevel{ ELogLevel::Critical };
		auto pSessions = _logSubscriptions.find( applicationId );
		if( pSessions==_logSubscriptions.end() )
			return minLevel;

		WebSocket::SessionPK brokenSession{ 0 };
		for( var& [sessionId,sessionLevel] : pSessions->second ){
			if( level<sessionLevel )
				continue;
			minLevel = (ELogLevel)std::min( (int8)minLevel, (int8)sessionLevel );
			shared_lock l3{ _sessionMutex };
			if( auto pSession = _sessions.find( sessionId ); pSession!=_sessions.end() ){
				sp<IO::Sockets::ISession> p = pSession->second;
				static_pointer_cast<MySession>(p)->PushMessage( id, applicationId, instanceId, time, level, messageId, fileId, functionId, lineNumber, userId, threadId, variables );
			}
			else
				brokenSession = sessionId;
		}
		if( brokenSession )
			pSessions->second.erase( brokenSession );
		if( !pSessions->second.size() )
			_logSubscriptions.erase( applicationId );

		return minLevel;
	}

	α WebServer::CoSend( FromServer::MessageUnion&& msg, SessionPK id )ι->PoolAwait{
		return PoolAwait( [m=move(msg),id]()mutable{ Send(move(m), id);} );
	}

	α WebServer::Send( FromServer::MessageUnion&& m, SessionPK id )ι->void{
		try
		{
			Server().AddOutgoing( move(m), id );
		}
		catch( IException& )
		{}
	}

	α WebServer::AddOutgoing( FromServer::MessageUnion&& msg, SessionPK id )ι->void{
		AddOutgoing( vector<FromServer::MessageUnion>{move(msg)}, id );
	}

	α WebServer::AddOutgoing( const vector<FromServer::MessageUnion>& messages, SessionPK id )ι->void{
		FromServer::Transmission t;
		for( auto&& msg : messages )
			*t.add_messages() = move( msg );
		const size_t size = t.ByteSizeLong();
		auto pBuffer = make_shared<std::vector<char>>( size );
		t.SerializeToArray( pBuffer->data(), (int)pBuffer->size() );
		var pKeyValue = _sessions.find( id ); 
		if( pKeyValue!=_sessions.end() ){
			auto p = dynamic_pointer_cast<MySession>( pKeyValue->second );
			p->Write( t );
		}
		else
			TRACE( "({})Could not find session for outgoing transmission.", id );
	}

	BeastException::BeastException( sv what, beast::error_code&& ec, ELogLevel level, const source_location& sl )ι:
		IException{ {std::to_string(ec.value()), ec.message()}, format("{} returned ({{}}){{}}", what), sl, (uint)ec.value(), level },
		ErrorCode{ move(ec) }
	{}

	α BeastException::LogCode( const beast::error_code& ec, ELogLevel level, sv what, SL sl )ι->void{
		if( BeastException::IsTruncated(ec) || ec.value()==125 || ec.value()==995 || what=="~DoSession" )
			LOGSL( level, AppTag(), "{} - ({}){}", what, ec.value(), ec.message() );
		else
			LOGSL( ELogLevel::Warning, AppTag(), "{} - ({}){}", what, ec.value(), ec.message() );
	}
}