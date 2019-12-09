#include "WebServer.h"
#include "LogClient.h"
#include "Listener.h"

#define var const auto
#define _listener Listener::GetInstance()
#define _logClient Logging::LogClient::Instance()
namespace Jde::ApplicationServer::Web
{
	sp<MyServer> MyServer::_spInstance;
	sp<MyServer> MyServer::CreateInstance( uint16 port )noexcept
	{
		ASSERT( !_spInstance );
		return _spInstance = sp<MyServer>( new MyServer(port) );
	}
	sp<MyServer> MyServer::GetInstance()noexcept
	{
		return _spInstance;
	}

	MyServer::MyServer( uint16 port )noexcept:
		Base{ port }
	{}

	void MyServer::SendStatuses( FromServer::Statuses* pAllocated )noexcept
	{
		MyFromServer transmission;
		transmission.add_messages()->set_allocated_statuses( pAllocated );
		var pData = WebSocket::Session::TryToBuffer( transmission );
		if( !pData )
			return;

		var sessions = _statusSessions.ToSet();
		for( var id : sessions )
		{
			auto pSession = _sessions.Find( id );
			if( !pSession )
				_statusSessions.erase( id );
			else
			{
				const vector<google::protobuf::uint8>& data = *pData;
				pSession->Write2( data );
			}
		}
	}
	void MyServer::SetStatus( FromServer::Status& status )const noexcept
	{
		status.set_applicationid( _logClient.ApplicationId );
		status.set_instanceid( _logClient.InstanceId );
		status.set_hostname( Diagnostics::HostName() );
		status.set_starttime( Clock::to_time_t(IApplication::StartTime()) );
		status.set_dbloglevel( (Web::FromServer::ELogLevel)GetServerSink()->GetLogLevel() );
		status.set_fileloglevel( (Web::FromServer::ELogLevel)GetDefaultLogger()->level() );
		status.set_memory( Diagnostics::GetMemorySize() );
		status.add_values( fmt::format("Web Connections:  {}", SessionCount()) );
	}
	void MyServer::RemoveSession( WebSocket::SessionPK id )noexcept
	{
		_statusSessions.erase( id );
		Base::RemoveSession( id );
	}

	bool MyServer::AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level )noexcept
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
	void MyServer::RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )noexcept
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
				DBG("({}) removing log subscription for instance {}, new level={}.", sessionId, instanceId, minLevel );
			}
			else
			{
				l.unlock();
				DBG("({}) could not find existing subscription for instance {} logs.", sessionId, instanceId );
			}
		}		
		_listener.WebSubscribe( instanceId, (ELogLevel)minLevel );
	}

	ELogLevel MyServer::PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept
	{
		shared_lock l{ _logSubscriptionMutex };
		auto minLevel = ELogLevel::None;
		auto pSessions = _logSubscriptions.find( applicationId );
		WebSocket::SessionPK brokenSession{0};
		if( pSessions!=_logSubscriptions.end() )
		{
			for( var [sessionId,sessionLevel] : pSessions->second )
			{
				minLevel = (ELogLevel)std::min( (uint)minLevel, (uint)sessionLevel );
				if( level<sessionLevel )
					continue;
				auto pSession = _sessions.Find( sessionId );
				if( !pSession )
					brokenSession = sessionId;
				else
					pSession->PushMessage( applicationId, instanceId, time, level, messageId, fileId, functionId, lineNumber, userId, threadId, variables );
			}
		}
		if( brokenSession )
		{
			pSessions->second.erase( brokenSession );
			if( !pSessions->second.size() )
				_logSubscriptions.erase( instanceId );
		}
		return minLevel;
	}
}