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
		Session( basio::ip::tcp::socket& socket, IO::Sockets::SessionPK id )noexcept;
		~Session(){DBG("Session::~Session"sv);}
		void OnReceive( sp<ToServer> pValue )noexcept override;
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
		void WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, const string& message )noexcept;
	private:
		void OnDisconnect()noexcept override;
		template<class T> using CustomFunction = function<void(Web::MySession&, uint, const T&)>;
		//template<typename T> typedef  CustomFunction<T>;
		template<class T>
		void SendCustomToWeb( const T& message, CustomFunction<T> write, bool erase=false )noexcept;
		ELogLevel _dbLevel;
		atomic<ELogLevel> _webLevel{ELogLevel::None};
		atomic<ELogLevel> _fileLogLevel{ELogLevel::None};
		typedef uint RequestId;
		atomic<RequestId> _requestId{0};
		map<RequestId,tuple<WebRequestId,IO::Sockets::SessionPK>> _customWebRequests; mutex _customWebRequestsMutex;
	};

	struct Listener final : public IO::Sockets::ProtoServer
	{
		static shared_ptr<Listener> Create( PortType port )noexcept(false);
		static Listener& GetInstance()noexcept{ return *GetInstancePtr(); }
		static shared_ptr<Listener>& GetInstancePtr()noexcept;

		sp<IO::Sockets::ProtoSession> CreateSession( basio::ip::tcp::socket socket, IO::Sockets::SessionPK id )noexcept override;

		uint ForEachSession( std::function<void(const IO::Sockets::SessionPK&, const Session&)>& fncn )noexcept;
		void SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )noexcept;
		void WebSubscribe( ApplicationPK applicationId, ELogLevel level )noexcept;

		sp<Session> FindSession( IO::Sockets::SessionPK id )noexcept;
		sp<Session> FindSessionByInstance( ApplicationInstancePK id )noexcept;
		sp<Session> FindApplication( ApplicationPK applicationId );
		//std::shared_ptr<SessionType> OnSessionStart( IO::Sockets::SessionPK id )noexcept override;
		//int ShouldAccept( std::shared_ptr<Session> /*pSession*/, std::string_view /*header*/ )noexcept{return 1;}
	private:
		Listener( PortType port )noexcept(false);

		//Collections::UnorderedMap<IO::Sockets::SessionPK, Session> _sessions;
		static shared_ptr<Listener> _pInstance;
		//WebSocket& _sender;
	};

#define var const auto
	template<class T>
	void Session::SendCustomToWeb( const T& message, CustomFunction<T> write, bool erase )noexcept
	{
		WebRequestId webRequestId;
		const RequestId reqId = message.requestid();
		IO::Sockets::SessionPK sessionId;
		{
			lock_guard l{_customWebRequestsMutex};
			var pRequest = _customWebRequests.find( reqId );
			if( pRequest==_customWebRequests.end() )
			{
				DBG( "Could not fine request {}"sv, reqId );
				return;
			}
			webRequestId = get<0>( pRequest->second );
			sessionId = get<1>( pRequest->second );
			if( erase )
				_customWebRequests.erase( pRequest );
		}
		var pSession = Web::MyServer::GetInstance()->Find( sessionId );
		if( pSession )
			write( *pSession, webRequestId, message );//pSession->WriteCustom( WebRequestId, message.message() );
		else
			DBG( "({})Could not find web session."sv, sessionId );
	}
#undef var
}