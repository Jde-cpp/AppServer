#include "stdafx.h"
#include "Listener.h"
#include "LogData.h"
#include "WebServer.h"
#include "Cache.h"
#include "LogClient.h"
#include "WebServer.h"

#define var const auto
#define _logClient Logging::LogClient::Instance()

#define _webServer (*Web::MyServer::GetInstance())
#define _webLevelUint static_cast<uint>((Jde::ELogLevel)_webLevel)
namespace Jde::ApplicationServer
{
	//using Messages::EMessages; using Messages::Application; using Messages::Message;
//namespace Server
//{
	shared_ptr<Listener> Listener::_pInstance{nullptr};
	shared_ptr<Listener> Listener::Create( PortType port )noexcept(false)
	{
		ASSRT_TR( _pInstance==nullptr );
		try
		{
			return _pInstance = shared_ptr<Listener>( new Listener(port) );
		}
		catch( const std::system_error& e )
		{
			CRITICAL0( e.what() );
			THROW( Exception("Could not create listener:  '{}'", e.what()) );
		}
	}
	shared_ptr<Listener>& Listener::GetInstancePtr()noexcept
	{
		ASSRT_TR( _pInstance!=nullptr );
		return _pInstance;
	}

	Listener::Listener( PortType port )noexcept(false):
		IO::Sockets::ProtoServer{ port }
	{
		Accept();
		RunAsyncHelper();//_pThread = make_shared<Threading::InterruptibleThread>( [&](){Run();} );
		INFO( "Accepting on port '{}'", port );
	}

	
	vector<google::protobuf::uint8> data( 4, 0 ); 
	sp<IO::Sockets::ProtoSession> Listener::CreateSession( basio::ip::tcp::socket socket, IO::Sockets::SessionPK id )noexcept
	{
		//auto onDone = [&]( std::error_code ec, std::size_t length )
		//{
		//	if( ec )
		//		ERR0( CodeException::ToString(ec) );
		//	else
		//		DBG("length={}", length);
		//};
		//basio::async_write( socket, basio::buffer(data.data(), data.size()), onDone );
		return sp<Session>( new Session{socket, id} );
		//return sp<Session>{};
	}

	uint Listener::ForEachSession( std::function<void(const IO::Sockets::SessionPK&, const Session&)>& fncn )noexcept
	{
		std::function<void(const IO::Sockets::SessionPK&, const IO::Sockets::ProtoSession&)> fncn2 = [&fncn](const IO::Sockets::SessionPK& id, const IO::Sockets::ProtoSession& session )
		{
			fncn( id, dynamic_cast<const Session&>(session) );
		};
		return _sessions.ForEach( fncn2 );
	}
	sp<Session> Listener::FindSession( IO::Sockets::SessionPK id )noexcept
	{
		std::function<bool(const IO::Sockets::ProtoSession&)> fnctn = [id]( var& session )
		{
			return dynamic_cast<const Session&>(session).ProcessId==id;
		};
		auto pSession = _sessions.FindFirst( fnctn );
		if( !pSession )
			WARN( "Could not find session '{}'.", id );
		return dynamic_pointer_cast<Session>( pSession );
	}
	sp<Session> Listener::FindSessionByInstance( ApplicationInstancePK id )noexcept
	{
		std::function<bool(const IO::Sockets::ProtoSession&)> fnctn = [id]( var& session )
		{
			return dynamic_cast<const Session&>(session).InstanceId==id;
		};
		auto pSession = _sessions.FindFirst( fnctn );
		if( !pSession )
			WARN( "Could not find instance '{}'.", id );
		return dynamic_pointer_cast<Session>( pSession );
	}
	void Listener::SetLogLevel( ApplicationInstancePK instanceId, ELogLevel dbLevel, ELogLevel clientLevel )noexcept
	{
		if( instanceId==_logClient.InstanceId )
		{
			GetDefaultLogger()->set_level( (spdlog::level::level_enum)clientLevel );
			if( GetServerSink() )
				GetServerSink()->SetLogLevel( dbLevel );
			Web::MyServer::GetInstance()->UpdateStatus( *Web::MyServer::GetInstance() );
		}
		else
		{
			auto pSession = FindSessionByInstance( instanceId );
			if( pSession )
				pSession->SetLogLevel( dbLevel, clientLevel );
		}
	}

	void Listener::WebSubscribe( ApplicationPK applicationId, ELogLevel level )noexcept
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
	sp<Session> Listener::FindApplication( ApplicationPK applicationId )
	{
		std::function<bool(const IO::Sockets::ProtoSession&)> fnctn = [applicationId]( var& session )
		{
			return dynamic_cast<const Session&>(session).ApplicationId==applicationId;
		};
		return dynamic_pointer_cast<Session>( _sessions.FindFirst(fnctn) );
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		

	Session::Session( basio::ip::tcp::socket& socket, IO::Sockets::SessionPK id )noexcept:
		IO::Sockets::TProtoSession<ToServer,FromServer>{ socket, id }
	{
		Start2();//0x7fffe0004760
	}
	
	void Session::Start2()noexcept
	{
		auto pAck = new Logging::Proto::Acknowledgement();
		pAck->set_instanceid( Id );

		Logging::Proto::FromServer transmission;
		transmission.add_messages()->set_allocated_acknowledgement( pAck );
		
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
			CRITICAL0( "!pStrings" );

		transmission.add_messages()->set_allocated_loglevels( AllocatedLogLevels() );
		Write( transmission );
	}
//send status update...
	void Session::SetStatus( Web::FromServer::Status& status )const noexcept
	{
		status.set_applicationid( ApplicationId );
		status.set_instanceid( InstanceId );
		status.set_hostname( HostName );
		status.set_starttime( Clock::to_time_t(StartTime) );
		status.set_dbloglevel( (Web::FromServer::ELogLevel)DbLogLevel() );
		status.set_fileloglevel( (Web::FromServer::ELogLevel)FileLogLevel() );
		status.set_memory( Memory );
		
		var statuses = StringUtilities::Split( Status, '\n' );
		for( var& statusDescription : statuses )
			status.add_values( statusDescription );
	}
	void Session::OnDisconnect()noexcept
	{
		StartTime = TimePoint{};
		auto pInstance = Web::MyServer::GetInstance();
		if( pInstance )
			pInstance->UpdateStatus( *this );
		Listener::GetInstance().RemoveSession( Id );
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

	void Session::WriteCustom( IO::Sockets::SessionPK webClientId, uint clientId, const string& message )noexcept
	{
		Logging::Proto::FromServer transmission;

		const uint requestId = ++_requestId;
		{
			lock_guard l{_customWebRequestsMutex};
			_customWebRequests.emplace( requestId, make_tuple(clientId, webClientId) );
		}
		auto pCustom = new Logging::Proto::CustomMessage();
		pCustom->set_requestid( requestId );
		pCustom->set_message( message );
		DBG( "'{}'('{}') sending custom message reqId='{}' for webClient='{}'('{}')", Name, InstanceId, requestId, webClientId, clientId );
		transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}
	
	void Session::OnReceive( sp<Logging::Proto::ToServer> pTransmission )noexcept
	{
		try
		{
			if( !pTransmission->messages_size() )
				THROW( Exception("!messages_size()") );
			for( var& item : pTransmission->messages() )
			{
				if( item.has_message() )//todo push multiple in one shot
				{
					if( !ApplicationId || !InstanceId )
					{
						ERR0( "sent message but have no instance." );
						continue;
					}

					var& message = item.message();
					const Jde::TimePoint time = TimePoint{} + std::chrono::seconds{ message.time().seconds() } + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{ message.time().nanos() });
					var level = (uint)message.level();
					vector<string> variables;
					for( auto i=0; i<message.variables_size(); ++i )
						variables.push_back( message.variables(i) );
					if( level>=(uint)_dbLevel )
						Logging::Data::PushMessage( ApplicationId, InstanceId, time, (ELogLevel)message.level(), message.messageid(), message.fileid(), message.functionid(), message.linenumber(), message.userid(), message.threadid(), variables );
					if( level>=_webLevelUint )
						_webLevel = Web::MyServer::GetInstance()->PushMessage( ApplicationId, InstanceId, time, (ELogLevel)message.level(), message.messageid(), message.fileid(), message.functionid(), message.linenumber(), message.userid(), message.threadid(), variables );
				}
				else if( item.has_string() )
				{
					var& strings = item.string();
					if( ApplicationId && InstanceId )
						Cache::Add( ApplicationId, strings.field(), strings.id(), strings.value() );
					else
						ERR0( "sent string but have no instance." );
				}
				else if( item.has_status() )
				{
					var& status = item.status();
					Memory = status.memory();
					ostringstream os;
					for( auto i=0; i<status.details_size(); ++i )
						os << status.details(i) << endl;
					Status = os.str();
					auto pInstance = Web::MyServer::GetInstance();
					if( pInstance )
						pInstance->UpdateStatus( *this );
				}
				else if( item.has_custom() )
				{
					var& custom = item.custom();
					uint clientId;
					IO::Sockets::SessionPK sessionId;
					{
						lock_guard l{_customWebRequestsMutex};
						var pRequest = _customWebRequests.find( custom.requestid() );
						if( pRequest==_customWebRequests.end() )
						{
							DBG( "Could not fine request {}", custom.requestid() );
							return;
						}
						clientId = get<0>( pRequest->second );
						sessionId = get<1>( pRequest->second );
						_customWebRequests.erase( pRequest );
					}
					var pSession = _webServer.Find( sessionId );
					if( pSession )
						pSession->WriteCustom( clientId, custom.message() );
					else
						DBG( "Could not web session {}", sessionId );
				}
				else if( item.has_instance() )
				{
					var& instance = item.instance();
					Name = instance.applicationname();
					HostName = instance.hostname();
					ProcessId = instance.processid();
					StartTime = Clock::from_time_t( instance.starttime() );
					try
					{
						var [applicationId,instanceId, dbLogLevel, fileLogLevel] = Logging::Data::AddInstance( Name, HostName, ProcessId );
						DBG( "Adding applicaiton ({}){}@{}", ProcessId, Name, HostName );
						InstanceId = instanceId; ApplicationId = applicationId; _dbLevel = dbLogLevel; _fileLogLevel = fileLogLevel;
						Cache::Load( ApplicationId );
						WriteStrings();
					}
					catch( const Exception& e )
					{
						ERR( "Could not create app instance - {}", e.what() );
					}
					break;
				}
			}
		}
		catch( const Exception& e )//parsing errors
		{
			ERR( "JdeException - {}", e.what() );
		}
	}
}