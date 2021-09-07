#pragma once
#include "WebServer.h"
	#include "../../Framework/source/io/sockets/ProtoServer.h"
//namespace IO
//{
//	class IncomingMessage;
//	namespace Sockets{ class Session; }
//}
//using IO::Sockets::Session;

namespace Jde::ApplicationServer
{
	//namespace Messages{ struct Application; struct Message; }

	//class WebSocket;
	using Logging::Proto::ToServer;
	using Logging::Proto::FromServer;
	namespace Web
	{
		struct MySession;
		namespace FromServer{ class Status; }
	}
	namespace basio=boost::asio;
	//typedef IO::Sockets::ISession<ToServer> SessionType;

	struct Session final: IO::Sockets::TProtoSession<ToServer,FromServer>
	{
		Session( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept;
		~Session(){DBG("Session::~Session"sv);}
		void OnReceive( ToServer&& pValue )noexcept override;
		void Start2()noexcept;
		void WriteStrings()noexcept;
		void SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )noexcept;
		Logging::Proto::LogLevels* AllocatedLogLevels()noexcept;
		ELogLevel DbLogLevel()const noexcept{ return _dbLevel; }
		ELogLevel FileLogLevel()const noexcept{ return _fileLogLevel; }
		void SetStatus( Web::FromServer::Status& status )const noexcept;
		void WebSubscribe( ELogLevel level )noexcept;
		ApplicationInstancePK InstanceId{0};
		ApplicationPK ApplicationId{0};
		string Name;
		uint ProcessId{0};
		string HostName;
		string Status;
		TimePoint StartTime;
		uint Memory{0};
		typedef IO::Sockets::TProtoSession<ToServer,FromServer> Base;
		typedef uint WebRequestId; //
		void WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, string&& message )noexcept;
	private:
		void OnDisconnect()noexcept override;
		template<class T> using CustomFunction = function<void(Web::MySession&, uint, T&&)>;
		template<class T>
		void SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase=false )noexcept;
		ELogLevel _dbLevel;
		atomic<ELogLevel> _webLevel{ELogLevel::None};
		atomic<ELogLevel> _fileLogLevel{ELogLevel::None};
		typedef uint RequestId;
		atomic<RequestId> _requestId{0};
		map<RequestId,tuple<WebRequestId,IO::Sockets::SessionPK>> _customWebRequests; mutex _customWebRequestsMutex;
	};

	struct TcpListener final : public IO::Sockets::ProtoServer
	{
		TcpListener()noexcept(false);
		//static shared_ptr<Listener> Create( PortType port )noexcept(false);
		static TcpListener& GetInstance()noexcept;
		//static shared_ptr<Listener>& GetInstancePtr()noexcept;

		up<IO::Sockets::ProtoSession> CreateSession( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept override;

		uint ForEachSession( std::function<void(const IO::Sockets::SessionPK, const Session&)> fncn )noexcept;
		void SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )noexcept;
		void WebSubscribe( ApplicationPK applicationId, ELogLevel level )noexcept;

		//sp<Session> FindSession( IO::Sockets::SessionPK id )noexcept;
		void Kill( ApplicationInstancePK id )noexcept;
		void WriteCustom( ApplicationPK id, uint32 requestId, string&& message )noexcept;
	private:
		Session* FindApplication( ApplicationPK applicationId )noexcept;
		Session* FindSessionByInstance( ApplicationInstancePK id )noexcept;
		//static shared_ptr<Listener> _pInstance;
	};

#define var const auto
	template<class T>
	void Session::SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase )noexcept
	{
		WebRequestId webRequestId;
		const RequestId reqId = message.requestid();
		IO::Sockets::SessionPK sessionId;
		{
			lock_guard l{_customWebRequestsMutex};
			var pRequest = _customWebRequests.find( reqId ); RETURN_IF( pRequest==_customWebRequests.end(), "Could not fine request {}", reqId );
			webRequestId = get<0>( pRequest->second );
			sessionId = get<1>( pRequest->second );
			if( erase )
				_customWebRequests.erase( pRequest );
		}
		if( var pSession = Web::Server().Find(sessionId); pSession )
			write( *pSession, webRequestId, move(message) );//pSession->WriteCustom( WebRequestId, message.message() );
		else
			DBG( "({})Could not find web session."sv, sessionId );
	}
#undef var
}