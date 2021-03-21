#pragma once
#include "WebSocket.h"

namespace Jde::ApplicationServer::Web
{
	struct MySession final : WebSocket::TSession<MyFromServer,MyFromClient>
	{
		typedef WebSocket::TSession<MyFromServer,MyFromClient> Base;
		MySession( sp<MyServer> pServer, uint id, boost::asio::ip::tcp::socket& socket )noexcept(false);
		~MySession();
		void OnConnect()noexcept;
		void OnRead( sp<MyFromClient> pTransmission )noexcept override;
		void SendStatuses()noexcept;
		static void SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )noexcept;
		void PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept;
		void WriteCustom( uint32 clientId, const string& message )noexcept;
		void WriteComplete( uint32 clientId )noexcept;
		void Start()noexcept override;
	private:
		void SendStrings( const FromClient::RequestStrings& request )noexcept;
		void WriteError( string&& msg, uint32 requestId=0 )noexcept;
		bool Write( const MyFromServer& tranmission  )noexcept;
		MyServer& Server()noexcept;
	};
}