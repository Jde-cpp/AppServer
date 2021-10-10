#pragma once
#include "WebSocketAsync.h"
#include "../../Framework/source/io/sockets/Socket.h"
#include <jde/log/types/proto/FromServer.pb.h>
#include <jde/log/types/proto/FromClient.pb.h>

namespace Jde::ApplicationServer::Web
{
	namespace beast = boost::beast;
	using tcp = boost::asio::ip::tcp;
	struct WebServer;
	struct MySession final : WebSocket::TSession<FromServer::Transmission,FromClient::Transmission>
	{
		typedef WebSocket::TSession<FromServer::Transmission,FromClient::Transmission> base;
		MySession( WebSocket::WebListener& server, IO::Sockets::SessionPK id, tcp::socket&& socket )noexcept(false);
		~MySession();
		α OnConnect()noexcept->void;
		α OnRead( FromClient::Transmission transmission )noexcept->void override;
		α OnAccept( beast::error_code ec )noexcept->void override;
		α SendStatuses()noexcept->void;
		Ω SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )noexcept->void;
		α PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept->void;
		α WriteCustom( uint32 clientId, const string& message )noexcept->void;
		α WriteComplete( uint32 clientId )noexcept->void;
		α Run()noexcept->void override;
	private:
		α SendStrings( const FromClient::RequestStrings& request )noexcept->void;
		α WriteError( string&& msg, uint32 requestId=0 )noexcept->void;
		α Write( const FromServer::Transmission& tranmission )noexcept(false)->void;
		α Server()noexcept->WebServer&;
	};
}