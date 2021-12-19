#pragma once
#include "WebServer.h"
#include "../../Framework/source/io/sockets/ProtoServer.h"


namespace Jde::ApplicationServer
{

	using Logging::Proto::ToServer;
	using Logging::Proto::FromServer;
	namespace Web
	{
		struct MySession;
		namespace FromServer{ class Status; }
	}
	namespace basio=boost::asio;

	struct Session final: IO::Sockets::TProtoSession<ToServer,FromServer>
	{
		Session( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept;
		~Session(){DBG("({})~Session - {}", Id, Name);}
		α OnReceive( ToServer&& pValue )noexcept->void override;
		α Start2()noexcept->void;
		α WriteStrings()noexcept->void;
		α SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )noexcept->void;
		α AllocatedLogLevels()noexcept->Logging::Proto::LogLevels*;
		α DbLogLevel()const noexcept->ELogLevel{ return _dbLevel; }
		α FileLogLevel()const noexcept->ELogLevel{ return _fileLogLevel; }
		α SetStatus( Web::FromServer::Status& status )const noexcept->void;
		α WebSubscribe( ELogLevel level )noexcept->void;
		ApplicationInstancePK InstanceId{0};
		ApplicationPK ApplicationId{0};
		string Name;
		uint ProcessId{0};
		string HostName;
		string Status;
		TimePoint StartTime;
		uint Memory{0};
		using base=IO::Sockets::TProtoSession<ToServer,FromServer>;
		using WebRequestId=uint; //
		α WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, string&& message )noexcept->void;
	private:
		α OnDisconnect()noexcept->void override;
		Τ using CustomFunction = function<void(Web::MySession&, uint, T&&)>;
		ⓣ SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase=false )noexcept->void;
		ELogLevel _dbLevel;
		atomic<ELogLevel> _webLevel{ELogLevel::None};
		atomic<ELogLevel> _fileLogLevel{ELogLevel::None};
		using RequestId=uint;
		atomic<RequestId> _requestId{0};
		map<RequestId,tuple<WebRequestId,IO::Sockets::SessionPK>> _customWebRequests; mutex _customWebRequestsMutex;
	};

	struct TcpListener final : public IO::Sockets::ProtoServer
	{
		TcpListener()noexcept(false);

		static TcpListener& GetInstance()noexcept;


		α CreateSession( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept->up<IO::Sockets::ProtoSession> override;

		α ForEachSession( std::function<void(const IO::Sockets::SessionPK, const Session&)> fncn )noexcept->uint;
		α SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )noexcept->void;
		α WebSubscribe( ApplicationPK applicationId, ELogLevel level )noexcept->void;


		α Kill( ApplicationInstancePK id )noexcept->void;
		α WriteCustom( ApplicationPK id, uint32 requestId, string&& message )noexcept(false)->void;
	private:
		α FindApplication( ApplicationPK applicationId )noexcept->Session*;
		α FindSessionByInstance( ApplicationInstancePK id )noexcept->Session*;

	};

#define var const auto
	ⓣ Session::SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase )noexcept->void
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
			write( *pSession, webRequestId, move(message) );
		else
			DBG( "({})Could not find web session."sv, sessionId );
	}
#undef var
}