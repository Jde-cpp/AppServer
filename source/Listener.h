#pragma once
#include "WebServer.h"
#include "../../Public/src/web/ProtoServer.h"


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
		Session( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )ι;
		~Session(){DBG("({})~Session - {}", Id, InstancePtr ? InstancePtr->application() : "nullptr");}
		α OnReceive( ToServer&& pValue )ι->void override;
		α Start2()ι->void;
		α WriteStrings()ι->void;
		α SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )ι->void;
		α LogLevels()ι->up<Logging::Proto::LogLevels>;
		α DbLogLevel()Ι->ELogLevel{ return _dbLevel; }
		α FileLogLevel()Ι->ELogLevel{ return _fileLogLevel; }
		α SetStatus( Web::FromServer::Status& status )Ι->void;
		α SendSessionInfo( SessionPK sessionId )ι->Task;
		α WebSubscribe( ELogLevel level )ι->void;
		ApplicationInstancePK InstanceId{0};
		ApplicationPK ApplicationId{0};
		sp<Logging::Proto::Instance> InstancePtr;
		string Status;
		uint Memory{0};
		using base=IO::Sockets::TProtoSession<ToServer,FromServer>;
		using WebRequestId=uint; //
		α WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, string&& message )ι->void;
	private:
		α OnDisconnect()ι->void override;
		Τ using CustomFunction = function<void(Web::MySession&, uint, T&&)>;
		Ŧ SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase=false )ι->void;
		ELogLevel _dbLevel;
		atomic<ELogLevel> _webLevel{ELogLevel::None};
		atomic<ELogLevel> _fileLogLevel{ELogLevel::None};
		using RequestId=uint;
		atomic<RequestId> _requestId{0};
		flat_map<RequestId,tuple<WebRequestId,IO::Sockets::SessionPK>> _customWebRequests; mutex _customWebRequestsMutex;
	};

	struct TcpListener final : public IO::Sockets::ProtoServer
	{
		TcpListener()ε;

		static TcpListener& GetInstance()ι;


		α CreateSession( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )ι->up<IO::Sockets::ProtoSession> override;

		α ForEachSession( std::function<void(const IO::Sockets::SessionPK, const Session&)> fncn )ι->uint;
		α SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )ι->void;
		α WebSubscribe( ApplicationPK applicationId, ELogLevel level )ι->void;


		α Kill( ApplicationInstancePK id )ι->void;
		α WriteCustom( ApplicationPK id, uint32 requestId, string&& message )ε->void;
		α FindApplications( const string& name )ι->vector<sp<Logging::Proto::Instance>>;
	private:
		α FindApplication( ApplicationPK applicationId )Ι->sp<Session>;
		α FindSessionByInstance( ApplicationInstancePK id )Ι->sp<Session>;

	};

#define var const auto
	Ŧ Session::SendCustomToWeb( T&& message, CustomFunction<T&&> write, bool erase )ι->void
	{
		WebRequestId webRequestId;
		const RequestId reqId = message.requestid();
		IO::Sockets::SessionPK sessionId;
		{
			lg l{_customWebRequestsMutex};
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