#pragma once

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
	namespace Web::FromServer{ class Status; }
	namespace basio=boost::asio;
	//typedef IO::Sockets::ISession<ToServer> SessionType;

	struct Session final: IO::Sockets::TProtoSession<ToServer,FromServer>
	{
		Session( basio::ip::tcp::socket& socket, IO::Sockets::SessionPK id )noexcept;
		~Session(){DBG0("Session::~Session");}
		void OnReceive( sp<ToServer> pValue )noexcept override;
		void Start2()noexcept;
		void WriteStrings()noexcept;
		void SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )noexcept;
		Logging::Proto::LogLevels* AllocatedLogLevels()noexcept;
		ELogLevel DbLogLevel()const noexcept{ return _dbLevel; }
		ELogLevel FileLogLevel()const noexcept{ return _fileLogLevel; }
		void SetStatus( Web::FromServer::Status& status )const noexcept;
		void WebSubscribe( ELogLevel level )noexcept;
		void WriteCustom( IO::Sockets::SessionPK webClientId, uint requestId, const string& message )noexcept;
		ApplicationInstancePK InstanceId{0};
		ApplicationPK ApplicationId{0};
		string Name;
		uint ProcessId{0};
		string HostName;
		string Status;
		TimePoint StartTime;
		uint Memory{0};
		typedef IO::Sockets::TProtoSession<ToServer,FromServer> Base;
	private:
		void OnDisconnect()noexcept;
		ELogLevel _dbLevel;
		ELogLevel _webLevel;
		ELogLevel _fileLogLevel;
		atomic<uint> _requestId{0};
		map<uint,tuple<uint,IO::Sockets::SessionPK>> _customWebRequests; mutex _customWebRequestsMutex;
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
}