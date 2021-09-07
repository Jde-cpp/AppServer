#include "Listener.h"
#include "LogData.h"
#include "WebServer.h"
#include "Cache.h"
#include "LogClient.h"
#include "WebServer.h"
#include <jde/Str.h>

#define var const auto
#define _logClient Logging::LogClient::Instance()

#define _webServer Web::Server()
#define _webLevelUint static_cast<uint>((Jde::ELogLevel)_webLevel)
namespace Jde::ApplicationServer
{

	TcpListener _listener;
	TcpListener& TcpListener::GetInstance()noexcept{ return _listener; }
	//shared_ptr<Listener> Listener::_pInstance{nullptr};
/*	shared_ptr<Listener> Listener::Create( PortType port )noexcept(false)
	{
		ASSERT( _pInstance==nullptr );
		try
		{
			return _pInstance = shared_ptr<Listener>( new Listener(port) );
		}
		catch( const std::system_error& e )
		{
			CRITICAL( string(e.what()) );
			THROW( Exception("Could not create listener:  '{}'", e.what()) );
		}
	}
	shared_ptr<Listener>& Listener::GetInstancePtr()noexcept
	{
		ASSERT( _pInstance!=nullptr );
		return _pInstance;
	}
*/
	TcpListener::TcpListener()noexcept(false)://multiple listeners on same port
		IO::Sockets::ProtoServer{ Settings::TryGet<PortType>("tcpListner/port").value_or(ServerSinkDefaultPort) }
	{
		Accept();
	}


	up<IO::Sockets::ProtoSession> TcpListener::CreateSession( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept
	{
		return make_unique<Session>( move(socket), id );
	}

	uint TcpListener::ForEachSession( std::function<void(IO::Sockets::SessionPK, const Session&)> f )noexcept
	{
		shared_lock l{ _mutex };
		for( var& [id,p] : _sessions )
			f( id, (const ApplicationServer::Session&)*p );
		return _sessions.size();
	}
/*	sp<Session> Listener::FindSession( IO::Sockets::SessionPK id )noexcept
	{
		std::function<bool(const IO::Sockets::ProtoSession&)> fnctn = [id]( var& session )
		{
			return dynamic_cast<const Session&>(session).ProcessId==id;
		};
		auto pSession = _sessions.FindFirst( fnctn );
		if( !pSession )
			WARN( "Could not find session '{}'."sv, id );
		return dynamic_pointer_cast<Session>( pSession );
	}
*/
/*	sp<Session> Listener::FindSessionByInstance( ApplicationInstancePK id )noexcept
	{
		std::function<bool(const IO::Sockets::ProtoSession&)> fnctn = [id]( var& session )
		{
			return dynamic_cast<const Session&>(session).InstanceId==id;
		};
		auto pSession = _sessions.FindFirst( fnctn );
		if( !pSession )
			WARN( "Could not find instance '{}'."sv, id );
		return dynamic_pointer_cast<Session>( pSession );
	}
	*/
	Session* TcpListener::FindSessionByInstance( ApplicationInstancePK id )noexcept
	{
		auto p = std::find_if( _sessions.begin(), _sessions.end(), [id](auto& p){ return ((Session*)p.second.get())->InstanceId==id;} );
		return p==_sessions.end() ? nullptr : (Session*)p->second.get();
	}
	Session* TcpListener::FindApplication( ApplicationPK id )noexcept
	{
		auto p = std::find_if( _sessions.begin(), _sessions.end(), [id](auto& p){ return ((Session*)p.second.get())->ApplicationId==id;} );
		return p==_sessions.end() ? nullptr : (Session*)p->second.get();
	}

	void TcpListener::Kill( ApplicationInstancePK id )noexcept
	{
		shared_lock l{ _mutex };
		if( auto p = FindSessionByInstance(id); p )
			IApplication::Kill( p->ProcessId );
		else
			WARN( "({})Could not find instance to kill."sv, id );
	}

	void TcpListener::WriteCustom( ApplicationPK id, uint32 requestId, string&& message )noexcept
	{
		shared_lock l{ _mutex };
		if( auto p = FindApplication(id); p )
			p->WriteCustom( p->Id, requestId, move(message) );
		else
		{
			auto pApps = Logging::Data::LoadApplications( id );
			THROW( "Application '{}' is not running", pApps->values_size() ? pApps->values(0).name() : std::to_string(id) );
		}
	}

	void TcpListener::SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )noexcept
	{
		if( instanceId==_logClient.InstanceId )
		{
			_logger.set_level( (spdlog::level::level_enum)clientLevel );
			if( _pServerSink )
				_serverLogLevel = dbLevel;
			Web::Server().UpdateStatus( Web::Server() );
		}
		else
		{
			auto pSession = FindSessionByInstance( instanceId );
			if( pSession )
				pSession->SetLogLevel( dbLevel, clientLevel );
		}
	}

	void TcpListener::WebSubscribe( ApplicationPK applicationId, ELogLevel level )noexcept
	{
		if( applicationId==_logClient.ApplicationId )
			_logClient.WebSubscribe( level );
		else
		{
			auto pSession = FindApplication( applicationId );
			if( pSession )
				pSession->WebSubscribe( level );
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Session::Session( basio::ip::tcp::socket&& socket, IO::Sockets::SessionPK id )noexcept:
		IO::Sockets::TProtoSession<ToServer,FromServer>{ move(socket), id }
	{
		Start2();//0x7fffe0004760
	}

	void Session::Start2()noexcept
	{
		auto pAck = make_unique<Logging::Proto::Acknowledgement>();
		pAck->set_instanceid( Id );

		Logging::Proto::FromServer transmission;
		transmission.add_messages()->set_allocated_acknowledgement( pAck.release() );
		LOG( _logLevel, "({})Sending Ack", Id );
		Write( transmission );
	}

	void Session::SetLogLevel( ELogLevel dbLogLevel, ELogLevel fileLogLevel )noexcept
	{
		_dbLevel = dbLogLevel;
		_fileLogLevel = fileLogLevel;
		Logging::Proto::FromServer transmission;
		transmission.add_messages()->set_allocated_loglevels( AllocatedLogLevels() );
		Write( transmission );
	}
	Logging::Proto::LogLevels* Session::AllocatedLogLevels()noexcept
	{
		auto pValues = new Logging::Proto::LogLevels();
		pValues->set_server( (Logging::Proto::ELogLevel)std::min((uint)_dbLevel, _webLevelUint) );
		pValues->set_client( static_cast<Logging::Proto::ELogLevel>((Jde::ELogLevel)_fileLogLevel) );
		return pValues;
	}

	void Session::WriteStrings()noexcept
	{
		Logging::Proto::FromServer transmission;
		var pStrings = Cache::GetApplicationStrings( ApplicationId );
		if( pStrings )
		{
			auto pValues = new Logging::Proto::Strings();
			std::function<void(const uint32&, const string&)> fnctn = [&pValues](const uint32& id, const string&)->void{pValues->add_files(id);};
			pStrings->FilesPtr->ForEach( fnctn );
			std::function<void(const uint32&, const string&)> fnctn2 = [&pValues](const uint32& id, const string&)->void{pValues->add_functions(id);};
			pStrings->FunctionsPtr->ForEach( fnctn2 );
			std::function<void(const uint32&, const string&)> messageFnctn = [&pValues](const uint32& id, const string&)->void{pValues->add_messages(id);};
			pStrings->MessagesPtr->ForEach( messageFnctn );

			transmission.add_messages()->set_allocated_strings( pValues );
		}
		else
			CRITICAL( "!pStrings"sv );

		transmission.add_messages()->set_allocated_loglevels( AllocatedLogLevels() );
		Write( transmission );
	}
//send status update...
	void Session::SetStatus( Web::FromServer::Status& status )const noexcept
	{
		status.set_applicationid( (uint32)ApplicationId );
		status.set_instanceid( (uint32)InstanceId );
		status.set_hostname( HostName );
		status.set_starttime( (uint32)Clock::to_time_t(StartTime) );
		status.set_dbloglevel( (Web::FromServer::ELogLevel)DbLogLevel() );
		status.set_fileloglevel( (Web::FromServer::ELogLevel)FileLogLevel() );
		status.set_memory( Memory );

		var statuses = Str::Split( Status, '\n' );
		for( var& statusDescription : statuses )
			status.add_values( statusDescription );
	}
	void Session::OnDisconnect()noexcept
	{
		StartTime = TimePoint{};
		Web::Server().UpdateStatus( *this );
		TcpListener::GetInstance().RemoveSession( Id );
	}

	void Session::WebSubscribe( ELogLevel level )noexcept
	{
		var currentLevel = std::min( (uint)_dbLevel, static_cast<uint>((Jde::ELogLevel)_webLevel) );
		_webLevel = level;
		if( currentLevel!=std::min((uint)_dbLevel, _webLevelUint) )
		{
			Logging::Proto::FromServer transmission;
			transmission.add_messages()->set_allocated_loglevels( AllocatedLogLevels() );
			Write( transmission );
		}
	}

	void Session::WriteCustom( IO::Sockets::SessionPK webClientId, WebRequestId webRequestId, string&& message )noexcept
	{
		Logging::Proto::FromServer transmission;

		const RequestId requestId = ++_requestId;
		{
			lock_guard l{_customWebRequestsMutex};
			_customWebRequests.emplace( requestId, make_tuple(webRequestId, webClientId) );
		}
		auto pCustom = new Logging::Proto::CustomMessage();
		pCustom->set_requestid( (uint32)requestId );
		pCustom->set_message( move(message) );
		DBG( "({}) sending custom message to '{}' reqId='{}' from webClient='{}'('{}')"sv, InstanceId, Name, requestId, webClientId, webRequestId );
		transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}

	void Session::OnReceive( Logging::Proto::ToServer&& transmission )noexcept
	{
		try
		{
			CHECK( t.messages_size() );
			for( uint i=0; i<transmission.messages_size(); ++i )
			{
				auto pMessage = transmission.mutable_messages( i );
				if( pMessage->has_message() )//todo push multiple in one shot
				{
					CONTINUE_IF( !ApplicationId || !InstanceId, "sent message but have no instance." );
					auto& message = *pMessage->mutable_message();
					const Jde::TimePoint time = TimePoint{} + std::chrono::seconds{ message.time().seconds() } + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{ message.time().nanos() });
					var level = (uint)message.level();
					vector<string> variables; variables.reserve( message.variables_size() );
					for( int i=0; i<message.variables_size(); ++i )
						variables.push_back( move(*message.mutable_variables(i)) );
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
					for( auto i=0; i<status.details_size(); ++i )
						os << move(*status.mutable_details(i)) << endl;
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
					auto& instance = *pMessage->mutable_instance();
					Name = move( instance.applicationname() );
					HostName = move( instance.hostname() );
					ProcessId = instance.processid();
					StartTime = Clock::from_time_t( instance.starttime() );
					var [applicationId,instanceId, dbLogLevel, fileLogLevel] = Logging::Data::AddInstance( Name, HostName, ProcessId );
					DBG( "Adding application ({}){}@{}"sv, ProcessId, Name, HostName );
					InstanceId = instanceId; ApplicationId = applicationId; _dbLevel = dbLogLevel; _fileLogLevel = fileLogLevel;
					Cache::Load( ApplicationId );
					WriteStrings();
					break;
				}
			}
		}
		catch( const Exception& e )//parsing errors
		{
			e.Log();
		}
	}
}