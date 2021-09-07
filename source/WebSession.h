#pragma once
#include "WebSocketAsync.h"
#include "../../Framework/source/io/sockets/Socket.h"
#include "types/proto/FromServer.pb.h"
#include "types/proto/FromClient.pb.h"

namespace Jde::ApplicationServer::Web
{
	using tcp = boost::asio::ip::tcp;
	struct WebServer;
	struct MySession final : WebSocket::TSession<FromServer::Transmission,FromClient::Transmission>
	{
		typedef WebSocket::TSession<FromServer::Transmission,FromClient::Transmission> base;
		MySession( WebSocket::WebListener& server, IO::Sockets::SessionPK id, tcp::socket&& socket )noexcept(false);
		~MySession();
		void OnConnect()noexcept;
		void OnRead( FromClient::Transmission transmission )noexcept override;
		void SendStatuses()noexcept;
		static void SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )noexcept;
		void PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept;
		void WriteCustom( uint32 clientId, const string& message )noexcept;
		void WriteComplete( uint32 clientId )noexcept;
		//void Start()noexcept override;
	private:
		void SendStrings( const FromClient::RequestStrings& request )noexcept;
		void WriteError( string&& msg, uint32 requestId=0 )noexcept;
		void Write( const FromServer::Transmission& tranmission )noexcept(false);
		WebServer& Server()noexcept;
	};
}