#include "Listener.h"
#include <jde/Str.h>
#include "../../Public/src/web/RestServer.h"
#include "LogData.h"
#include "WebServer.h"
#include "Cache.h"
#include "LogClient.h"
#include "WebServer.h"

#define var const auto
#define _logLevel LogLevel()
#define _logClient Logging::LogClient::Instance()

#define _webServer Web::Server()
#define _webLevelUint static_cast<uint>((Jde::ELogLevel)_webLevel)
namespace Jde::ApplicationServer
{

	TcpListener _listener;
	TcpListener& TcpListener::GetInstance()ι{ return _listener; }

	TcpListener::TcpListener()ε://multiple listeners on same port
		IO::Sockets::ProtoServer{ Settings::Get<PortType>("tcpListner/port").value_or(ServerSinkDefaultPort) }
	{
		Accept();
	}


	α TcpListener::CreateSession( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )ι->up<IO::Sockets::ProtoSession>
	{
		return mu<Session>( move(socket), id );
	}
#define $(x) dynamic_pointer_cast<Session>(x)
	α TcpListener::ForEachSession( std::function<void(IO::Sockets::SessionPK, const Session&)> f )ι->uint
	{
		sl l{ _mutex };
		for( var& [id,p] : _sessions )
			f( id, *$(p) );
		return _sessions.size();
	}

	α TcpListener::FindSessionByInstance( ApplicationInstancePK id )Ι->sp<Session>
	{
		sl _{ _mutex };
		auto p = std::find_if( _sessions.begin(), _sessions.end(), [id](auto& p){ return $(p.second)->InstanceId==id;} );
		return p==_sessions.end() ? nullptr :$(p->second);
	}
	α TcpListener::FindApplication( ApplicationPK id )Ι->sp<Session>
	{
		sl _{ _mutex };
		auto p = std::find_if( _sessions.begin(), _sessions.end(), [id](auto& p){ return $(p.second)->ApplicationId==id;} );
		return p==_sessions.end() ? nullptr : $(p->second);
	}

	α TcpListener::FindApplications( const string& name )ι->vector<sp<Logging::Proto::Instance>>
	{
		vector<sp<Logging::Proto::Instance>> y;
		sl _{ _mutex };
		for( auto s : _sessions )
		{
			if( $(s.second)->InstancePtr->application()==name )
				y.push_back( $(s.second)->InstancePtr );
		}
		return y;
	}
#undef $
	α TcpListener::Kill( ApplicationInstancePK id )ι->void
	{
		sl l{ _mutex };
		if( auto p = FindSessionByInstance(id); p && p->InstancePtr )
			IApplication::Kill( p->InstancePtr->pid() );
		else
			WARN( "({})Could not find instance to kill."sv, id );
	}

	α TcpListener::WriteCustom( ApplicationPK id, uint32 requestId, string&& message )ε->void
	{
		sl l{ _mutex };
		if( auto p = FindApplication(id); p )
			p->WriteCustom( p->Id, requestId, move(message) );
		else
		{
			auto pApps = Logging::Data::LoadApplications( id );
			THROW( "Application '{}' is not running", pApps->values_size() ? pApps->values(0).name() : std::to_string(id) );
		}
	}

	α TcpListener::SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )ι->void
	{
		if( instanceId==Logging::Server::InstanceId() )
		{
			Logging::Default().set_level( (spdlog::level::level_enum)clientLevel );
			Logging::Server::SetLevel( dbLevel );
			Web::Server().UpdateStatus( Web::Server() );
		}
		else
		{
			auto pSession = FindSessionByInstance( instanceId );
			if( pSession )
				pSession->SetLogLevel( dbLevel, clientLevel );
		}
	}

	α TcpListener::WebSubscribe( ApplicationPK applicationId, ELogLevel level )ι->void
	{
		if( applicationId==Logging::Server::ApplicationId() )
			Logging::Server::WebSubscribe( level );
		else
		{
			auto pSession = FindApplication( applicationId );
			if( pSession )
				pSession->WebSubscribe( level );
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Session::Session( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )ι:
		IO::Sockets::TProtoSession<ToServer,FromServer>{ move(socket), id }
	{
		Start2();//0x7fffe0004760
	}

	α Session::Start2()ι->void
	{
		auto pAck = mu<Logging::Proto::Acknowledgement>();
		pAck->set_instanceid( Id );

		Logging::Proto::FromServer t;
		t.add_messages()->set_allocated_acknowledgement( pAck.release() );
		LOG( "({})Sending Ack", Id );
		Write( move(t) );
	}

	α Session::SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )ι->void
	{
		_dbLevel = dbLogLevel;
		_fileLogLevel = fileLogLevel;
		Logging::Proto::FromServer t;
		t.add_messages()->set_allocated_loglevels( LogLevels().release() );
		Write( move(t) );
	}
	α Session::LogLevels()ι->up<Logging::Proto::LogLevels>
	{
		auto pValues = mu<Logging::Proto::LogLevels>();
		pValues->set_server( (Logging::Proto::ELogLevel)std::min((uint)_dbLevel, _webLevelUint) );
		pValues->set_client( static_cast<Logging::Proto::ELogLevel>((Jde::ELogLevel)_fileLogLevel) );
		return pValues;
	}

	α Session::WriteStrings()ι->void
	{
		Logging::Proto::FromServer t;
		var& strings = Cache::AppStrings();
		auto pValues = mu<Logging::Proto::Strings>();
		strings.Files.ForEach( [&pValues](const uint32& id, str)->void{pValues->add_files(id);} );
		strings.Functions.ForEach( [&pValues](const uint32& id, str)->void{pValues->add_functions(id);} );
		strings.Messages.ForEach( [&pValues](const uint32& id, str)->void{pValues->add_messages(id);} );
		t.add_messages()->set_allocated_strings( pValues.release() );

		Write( move(t) );
	}

	α Session::SetStatus( Web::FromServer::Status& status )Ι->void
	{
		status.set_application_id( (uint32)ApplicationId );
		status.set_instance_id( (uint32)InstanceId );
		status.set_host_name( InstancePtr ? InstancePtr->host() : "" );
		status.set_start_time( InstancePtr ? InstancePtr->start_time() : 0 );
		status.set_db_log_level( (Web::FromServer::ELogLevel)DbLogLevel() );
		status.set_file_log_level( (Web::FromServer::ELogLevel)FileLogLevel() );
		status.set_memory( Memory );

		var statuses = Str::Split( Status, '\n' );
		for( var& statusDescription : statuses )
			status.add_values( string{statusDescription} );
	}
	α Session::OnDisconnect()ι->void
	{
		if( InstancePtr )
			InstancePtr->set_start_time( 0 );
		Web::Server().UpdateStatus( *this );
		TcpListener::GetInstance().RemoveSession( Id );
	}

	α Session::WebSubscribe( ELogLevel level )ι->void
	{
		var currentLevel = std::min( (uint)_dbLevel, static_cast<uint>((Jde::ELogLevel)_webLevel) );
		_webLevel = level;
		if( currentLevel!=std::min((uint)_dbLevel, _webLevelUint) )
		{
			Logging::Proto::FromServer t;
			t.add_messages()->set_allocated_loglevels( LogLevels().release() );
			Write( move(t) );
		}
	}

	α Session::WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, string&& message )ι->void
	{
		Logging::Proto::FromServer t;

		const RequestId requestId = ++_requestId;
		{
			lg l{_customWebRequestsMutex};
			_customWebRequests.emplace( requestId, make_tuple(webRequestId, webClientId) );
		}
		auto pCustom = new Logging::Proto::CustomMessage();
		pCustom->set_requestid( (uint32)requestId );
		pCustom->set_message( move(message) );
		DBG( "({}) sending custom message to '{}' reqId='{}' from webClient='{}'('{}')"sv, InstanceId, InstancePtr ? InstancePtr->application() : "", requestId, webClientId, webRequestId );
		t.add_messages()->set_allocated_custom( pCustom );
		Write( move(t) );
	}

	α Session::SendSessionInfo( SessionPK sessionId )ι->Task
	{
		up<SessionInfo> pInfo;
		try
		{
			pInfo = (co_await Jde::Web::Rest::ISession::FetchSessionInfo(sessionId)).UP<SessionInfo>();
		}
		catch( const Exception& )
		{}

		Logging::Proto::FromServer t; t.add_messages()->set_allocated_session_info( pInfo ? pInfo.release() : new SessionInfo{} );
		Write( move(t) );
	}

	α Session::OnReceive( Logging::Proto::ToServer&& t )ι->void
	{
		try
		{
			CHECK( t.messages_size() );
			for( int i=0; i<t.messages_size(); ++i )
			{
				auto pMessage = t.mutable_messages( i );
				if( pMessage->has_message() )//todo push multiple in one shot
				{
					CONTINUE_IF( !ApplicationId || !InstanceId, "sent message but have no instance." );
					auto& message = *pMessage->mutable_message();
					const Jde::TimePoint time = TimePoint{} + std::chrono::seconds{ message.time().seconds() } + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{ message.time().nanos() });
					var level = (uint)message.level();
					vector<string> variables; variables.reserve( message.variables_size() );
					for( int i2=0; i2<message.variables_size(); ++i2 )
						variables.push_back( move(*message.mutable_variables(i2)) );
					bool sendWeb = level>=_webLevelUint;
					if( level>=(uint)_dbLevel )
						Logging::Data::PushMessage( ApplicationId, InstanceId, time, (ELogLevel)message.level(), message.messageid(), message.fileid(), message.functionid(), message.linenumber(), message.userid(), message.threadid(), sendWeb ? variables : move(variables) );
					if( sendWeb )
						_webLevel = Web::Server().PushMessage( 0, ApplicationId, InstanceId, time, (ELogLevel)message.level(), message.messageid(), message.fileid(), message.functionid(), message.linenumber(), message.userid(), message.threadid(), move(variables) );
				}
				else if( pMessage->has_string() )
				{
					CONTINUE_IF( !ApplicationId || !InstanceId, "sent string but have no instance.  ApplicationId='{}' InstanceId='{}'", ApplicationId, InstanceId );
					auto pStrings = pMessage->mutable_string();
					Cache::Add( ApplicationId, pStrings->field(), pStrings->id(), move(pStrings->value()) );
				}
				else if( pMessage->has_status() )
				{
					auto& status = *pMessage->mutable_status();
					Memory = status.memory();
					ostringstream os;
					for( α i2=0; i2<status.details_size(); ++i2 )
						os << move(*status.mutable_details(i2)) << std::endl;
					Status = os.str();
					Web::Server().UpdateStatus( *this );
				}
				else if( pMessage->has_custom() )
				{
					DBG( "({})Received custom message, sending to web."sv, InstanceId );
					CustomFunction<Logging::Proto::CustomMessage> fnctn = []( Web::MySession& webSession, uint a, Logging::Proto::CustomMessage&& b ){ webSession.WriteCustom((uint32)a, b.message()); };
					SendCustomToWeb<Logging::Proto::CustomMessage>( move(*pMessage->mutable_custom()), fnctn );
				}
				else if( pMessage->has_complete() )
				{
					CustomFunction<Logging::Proto::CustomComplete> fnctn = []( Web::MySession& webSession, uint a, Logging::Proto::CustomComplete&& ){ webSession.WriteComplete((uint32)a); };
					SendCustomToWeb<Logging::Proto::CustomComplete>( move(*pMessage->mutable_complete()), fnctn, true );
				}
				else if( pMessage->has_instance() )
				{
					InstancePtr = ms<Logging::Proto::Instance>( move(*pMessage->mutable_instance()) );
					var [applicationId,instanceId, dbLogLevel_, fileLogLevel_] = Logging::Data::AddInstance( InstancePtr->application(), InstancePtr->host(), InstancePtr->pid() );//TODO don't use db for level
					DBG( "({})Adding application app={}@{} pid={}", Id, InstancePtr->application(), InstancePtr->host(), InstancePtr->pid() );
					InstanceId = instanceId; ApplicationId = applicationId;// _dbLevel = dbLogLevel; _fileLogLevel = fileLogLevel;
					Cache::Load();
					WriteStrings();
					break;
				}
				else if( pMessage->has_session_info() )
					SendSessionInfo( pMessage->session_info().session_id() );
			}
		}
		catch( const IException& )
		{}
	}
}