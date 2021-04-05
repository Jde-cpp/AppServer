#pragma once
#include "WebSocket.h"
#include "WebSession.h"
#include "../../Framework/source/collections/UnorderedSet.h"

namespace Jde::ApplicationServer::Web
{
	class MyServer : public WebSocket::TServer<MyServer,MyFromClient,MySession>
	{
		typedef WebSocket::TServer<MyServer,MyFromClient,MySession> Base;
	public:
		virtual ~MyServer()=default;

		static sp<MyServer> GetInstance()noexcept;//TODOrefactor
		static sp<MyServer> CreateInstance( uint16 port )noexcept;
		template<typename T>
		void UpdateStatus( const T& app )noexcept;
		void SendStatuses( FromServer::Statuses* pAllocated )noexcept;
		void SetStatus( FromServer::Status& status )const noexcept;
		void AddStatusSession( const WebSocket::SessionPK& id )noexcept{ _statusSessions.emplace(id); }
		void RemoveStatusSession( const WebSocket::SessionPK& id )noexcept{ _statusSessions.erase(id); }
		UnorderedSet<WebSocket::SessionPK> _statusSessions;
		void Shutdown()noexcept override{ Base::Shutdown(); _spInstance=nullptr;}
		void RemoveSession( WebSocket::SessionPK id )noexcept override;
		bool AddLogSubscription( WebSocket::SessionPK sessionId, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level )noexcept;
		void RemoveLogSubscription( WebSocket::SessionPK sessionId, ApplicationInstancePK instanceId )noexcept;
		ELogLevel PushMessage( LogPK id, ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept;
	private:
		MyServer( uint16 port )noexcept;
		map<ApplicationPK,map<WebSocket::SessionPK,ELogLevel>> _logSubscriptions; shared_mutex _logSubscriptionMutex;
		static sp<MyServer> _spInstance;
	};
	template<typename T>
	void MyServer::UpdateStatus( const T& app )noexcept
	{
		auto pStatuses = new FromServer::Statuses();
		app.SetStatus( *pStatuses->add_values() );
		SendStatuses( pStatuses );
	}
}