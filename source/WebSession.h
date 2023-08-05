#pragma once
#include "WebSocketAsync.h"
#include "../../Framework/source/io/sockets/Socket.h"
#include <jde/log/types/proto/FromServer.pb.h>
#include <jde/log/types/proto/FromClient.pb.h>

namespace Jde::ApplicationServer::Web
{
	namespace beast = boost::beast;
	namespace websocket = beast::websocket;

	using tcp = boost::asio::ip::tcp;
	struct WebServer;
	typedef uint32 ClientId ;
	enum class EAuthType : uint8
	{
		None = 0,
		Google=1
	};
	constexpr array<sv,2> AuthTypeStrings = { "None", "Google" };

	struct MySession final : WebSocket::TSession<FromServer::Transmission,FromClient::Transmission>
	{
		typedef WebSocket::TSession<FromServer::Transmission,FromClient::Transmission> base;
		MySession( WebSocket::WebListener& server, IO::Sockets::SessionPK id, tcp::socket&& socket )ε;
		~MySession();
		α OnConnect()ι->void;
		α OnRead( FromClient::Transmission transmission )ι->void override;
		α OnAccept( beast::error_code ec )ι->void override;
		α SendStatuses()ι->void;
		Ω SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )ι->void;
		α PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )ι->void;
		α WriteCustom( uint32 clientId, const string& message )ι->void;
		α WriteComplete( uint32 clientId )ι->void;
		α Run()ι->void override;
	private:
		α SendStrings( const FromClient::RequestStrings& request )ι->void;
		α WriteError( string&& msg, uint32 requestId=0 )ι->void;
		α Write( const FromServer::Transmission& tranmission )ε->void;
		α Server()ι->WebServer&;
		α GoogleLogin( string&& credential, ClientId clientId )ι->Task;

		sp<std::atomic_flag> WriteLockPtr{ ms<std::atomic_flag>() };
		EAuthType AuthType{EAuthType::None};
		string Email;
		bool EmailVerified{false};
		string Name;
		string PictureUrl;
		TimePoint Expiration;
		UserPK UserId;

		friend WebServer;
	};
}