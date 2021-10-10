#include "WebServer.h"
#include "LogClient.h"
#include "Listener.h"

#define var const auto
#define _listener Jde::ApplicationServer::TcpListener::GetInstance()
#define _logClient Logging::LogClient::Instance()
namespace Jde::ApplicationServer::Web
{
	WebServer _instance{ Settings::TryGet<PortType>("web/port").value_or(1967) };
	α Server()noexcept->WebServer&{ return _instance; }

	// sp<WebServer> WebServer::CreateInstance( uint16 port )noexcept
	// {
	// 	ASSERT( !_spInstance );
	// 	return _spInstance = sp<WebServer>( new WebServer(port) );
	// }

	WebServer::WebServer( PortType port )noexcept:
		base{ port }
	{}

	α WebServer::SendStatuses( up<FromServer::Statuses> pAllocated )noexcept(false)->void
	{
		FromServer::Transmission t;
		t.add_messages()->set_allocated_statuses( pAllocated.release() );
		var data = IO::Proto::ToString( t );
		var sessions = _statusSessions.ToSet();
		for( var id : sessions )
		{
			shared_lock l{ _sessionMutex };
			if( auto pSession = _sessions.find(id); pSession!=_sessions.end() )
			{
				sp<IO::Sockets::ISession> p = pSession->second;
				static_pointer_cast<MySession::base>( p )->Write( data );
			}
			else
				_statusSessions.erase( id );
		}
	}
	α WebServer::SetStatus( FromServer::Status& status )const noexcept->void
	{
		status.set_applicationid( (google::protobuf::uint32)_logClient.ApplicationId );
		status.set_instanceid( (google::protobuf::uint32)_logClient.InstanceId );
		status.set_hostname( IApplication::HostName() );
		status.set_starttime( (google::protobuf::uint32)Clock::to_time_t(IApplication::StartTime()) );
		status.set_dbloglevel( (Web::FromServer::ELogLevel)Logging::ServerLevel() );
		status.set_fileloglevel( (Web::FromServer::ELogLevel)Logging::Default().level() );
		status.set_memory( IApplication::MemorySize() );
		status.add_values( fmt::format("Web Connections:  {}", SessionCount()) );
	}
	α WebServer::RemoveSession( WebSocket::SessionPK id )noexcept->void
	{
		_statusSessions.erase( id );
		base::RemoveSession( id );
	}

	α WebServer::AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK /*instanceId*/, ELogLevel level )noexcept->bool
	{
		bool newSubscription;
		uint minLevel = (uint)ELogLevel::None;
		{
			unique_lock l{ _logSubscriptionMutex };
			auto& sessions = _logSubscriptions.try_emplace( applicationId, map<WebSocket::SessionPK,ELogLevel>{} ).first->second;
			newSubscription = sessions.try_emplace( sessionId, level ).second;
			for( var& subscriber : sessions )
				minLevel = std::min(minLevel, (uint)subscriber.second );
		}
		_listener.WebSubscribe( applicationId, (ELogLevel)minLevel );

		return newSubscription;
	}
	α WebServer::RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )noexcept->void
	{
		uint minLevel = (uint)ELogLevel::None;
		{
			unique_lock l{ _logSubscriptionMutex };
			auto pSubscriptions = _logSubscriptions.find( instanceId );
			if( pSubscriptions!=_logSubscriptions.end() )
			{
				pSubscriptions->second.erase( sessionId );
				for( var& subscriber : pSubscriptions->second )
					minLevel = std::min(minLevel, (uint)subscriber.second );
				if( !pSubscriptions->second.size() )
					_logSubscriptions.erase( pSubscriptions );
				l.unlock();
				DBG("({}) removing log subscription for instance {}, new level={}."sv, sessionId, instanceId, minLevel );
			}
			else
			{
				l.unlock();
				DBG("({}) could not find existing subscription for instance {} logs."sv, sessionId, instanceId );
			}
		}
		_listener.WebSubscribe( instanceId, (ELogLevel)minLevel );
	}

	ELogLevel WebServer::PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )noexcept
	{
		unique_lock l{ _logSubscriptionMutex };
		auto minLevel{ ELogLevel::None };
		auto pSessions = _logSubscriptions.find( applicationId );
		if( pSessions==_logSubscriptions.end() )
			return minLevel;

		WebSocket::SessionPK brokenSession{ 0 };
		for( var [sessionId,sessionLevel] : pSessions->second )
		{
			if( level<sessionLevel )
				continue;
			minLevel = (ELogLevel)std::min( (uint)minLevel, (uint)sessionLevel );
			shared_lock l3{ _sessionMutex };
			if( auto pSession = _sessions.find( sessionId ); pSession!=_sessions.end() )
			{
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
}