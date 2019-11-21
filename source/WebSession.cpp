#include "stdafx.h"
#include "Cache.h"
#include "WebSession.h"
#include "WebServer.h"
#include "Listener.h"
#include "LogData.h"

#define var const auto
#define _listener Listener::GetInstance()

namespace Jde::ApplicationServer::Web
{
	MySession::MySession( sp<MyServer> pServer, uint id, boost::asio::ip::tcp::socket& socket )noexcept(false):
		Base( pServer, id, socket )
	{}
	
	MySession::~MySession()
	{
		DBG( "~MySession( {} )", Id );
	}

	void MySession::Start()noexcept
	{
		Session::Start();
		auto pAck = new Web::FromServer::Acknowledgement();
		pAck->set_id( Id );
		MyFromServer transmission;
		transmission.add_messages()->set_allocated_acknowledgement( pAck );
		Write( transmission );
		Server().UpdateStatus( Server() );
	}


	MyServer& MySession::Server()noexcept
	{
		return dynamic_cast<MyServer&>( *_pServer );
	}

	void MySession::OnRead( sp<MyFromClient> pTransmission )noexcept
	{
		for( var& message : pTransmission->messages() )
		{
			if( message.has_request() )
			{
				var& request = message.request();
				if( request.value()==FromClient::ERequest::Statuses )
					SendStatuses();
				else if( request.value()==(FromClient::ERequest::Statuses|FromClient::ERequest::Negate) )
				{
					Server().RemoveStatusSession( Id );
					DBG( "({})  Remove status subscription.", Id );
				}
				else if( request.value() == FromClient::ERequest::Applications )
				{
					auto pApplications = Logging::Data::LoadApplications();
					MyFromServer transmission;
					transmission.add_messages()->set_allocated_applications( pApplications );
					DBG( "({})Writing Applications count='{}'", Id, pApplications->values_size() );
					Write( transmission );
				}
				else
					WARN( "unsupported request '{}'", request.value() );
			}
			else if( message.has_requestid() )
			{
				var& request = message.requestid();
				const ApplicationInstancePK instanceId = request.instanceid();
				var value = (int)request.value();
				if( value == -2 )//(int)(FromClient::ERequest::Power | FromClient::ERequest::Negate);
				{
					var pSession = _listener.FindSessionByInstance( instanceId );
					if( pSession )
					{
						DBG( "killing proc id='{}' from sessionId '{}'", pSession->ProcessId, Id );
						IApplication::Kill( pSession->ProcessId );
					}
				}
				if( value == -3 )//(int)(FromClient::ERequest::Logs | FromClient::ERequest::Negate);
					Server().RemoveLogSubscription( Id, instanceId );
				else if( value == FromClient::ERequest::Power )
					WARN0( "unsupported request Power" );
				else
					WARN( "unsupported request '{}'", request.value() );
			}
			else if( message.has_logvalues() )
			{
				var& values = message.logvalues();
				if( values.dbvalue()<ELogLevelStrings.size() && values.clientvalue()<ELogLevelStrings.size() )
					DBG( "({}) - SetLogLevel for instance='{}', db='{}', client='{}'", Id, values.instanceid(), ELogLevelStrings[values.dbvalue()], ELogLevelStrings[values.clientvalue()] );
				Logging::Proto::LogLevels levels;
				_listener.SetLogLevel( values.instanceid(), (ELogLevel)values.dbvalue(), (ELogLevel)values.clientvalue() );
			}
			else if( message.has_requestlogs() )
			{
				var& value = message.requestlogs();
				if( value.value()<ELogLevelStrings.size() )
					DBG( "({})AddLogSubscription application='{}' instance='{}', level='{}'", Id, value.applicationid(), value.instanceid(), ELogLevelStrings[value.value()] );
				if( Server().AddLogSubscription(Id, value.applicationid(), value.instanceid(), (ELogLevel)value.value()) )//if changing level, don't want to send old logs
					std::thread{ [&](){SendLogs( value.applicationid(), value.instanceid(), (ELogLevel)value.value(), Clock::from_time_t(value.start()) );} }.detach();
			}
			else if( message.has_custom() )
			{
				var& custom = message.custom();
				DBG( "Session '{}' received custom reqId='{}' for application='{}'", Id, custom.requestid(), custom.applicationid() );
				auto pSession = _listener.FindApplication( custom.applicationid() );
				const string message = custom.message();
				if( pSession )
					pSession->WriteCustom( Id, custom.requestid(), message );
				else
					WriteError( fmt::format("Could not find application '{}'", custom.applicationid()), custom.requestid() );
			}
			else if( message.has_requeststrings() )
			{
				var& request = message.requeststrings();
				TRACE( "({}) requeststrings count='{}'", Id, request.values_size() );
				map<ApplicationPK,forward_list<FromServer::ApplicationString>> values;
				for( auto i=0; i<request.values_size(); ++i )
				{
					var& value = request.values( i );
					auto pStrings = Cache::GetApplicationStrings( value.applicationid() );
					if( !pStrings )
						pStrings = Cache::Load( value.applicationid() );//todo wrap in try statement.
					sp<string> pString;
					if( value.type()==FromClient::EStringRequest::MessageString )
						pString = pStrings->Get( Logging::EFields::Message, value.value() );
					else if( value.type()==FromClient::EStringRequest::File )
						pString = pStrings->Get( Logging::EFields::File, value.value() );
					else if( value.type()==FromClient::EStringRequest::Function )
						pString = pStrings->Get( Logging::EFields::Function, value.value() );
					//else if( value.type()==FromClient::EStringRequest::Thread )
					//	pString = pStrings->Get( Logging::EFields::ThreadId, value.value() );
					else if( value.type()==FromClient::EStringRequest::User )
						pString = pStrings->Get( Logging::EFields::User, value.value() );
					if( pString )
					{
						FromServer::ApplicationString appString; appString.set_stringrequesttype( value.type() ); appString.set_id( value.value() ); appString.set_value( *pString );
						auto& strings = values.try_emplace(value.applicationid(), forward_list<FromServer::ApplicationString>{} ).first->second;
						strings.push_front( appString );
					}
					else
						WARN( "Could not find string type='{}', id='{}', application='{}'",value.type(), value.value(), value.applicationid() );
				}
				
				MyFromServer transmission;
				for( var& [id,strings] : values )
				{
					auto pStrings = new FromServer::ApplicationStrings();
					pStrings->set_applicationid( id );
					for( var& value : strings )
						*pStrings->add_values() = value;
					transmission.add_messages()->set_allocated_strings( pStrings );
				}
				Write( transmission );
			}
			else
				ERR( "Unknown message:  {}", (uint)message.Value_case() );
		}
	};

	void MySession::SendStatuses()noexcept
	{
		auto pStatuses = new FromServer::Statuses();
		Server().SetStatus( *pStatuses->add_values() );
		std::function<void( const IO::Sockets::SessionPK&, const ApplicationServer::Session& session )> fncn = [pStatuses]( const IO::Sockets::SessionPK& id, const ApplicationServer::Session& session )
		{
			session.SetStatus( *pStatuses->add_values() );
		};
		_listener.ForEachSession( fncn );
		MyFromServer transmission;
		transmission.add_messages()->set_allocated_statuses( pStatuses );
		if( Write(transmission) )
		{
			DBG( "({})Add status subscription.", Id );
			Server().AddStatusSession( Id );
		}
	}
	void MySession::SendLogs( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, const TimePoint& start )noexcept
	{
		auto pTraces = Logging::Data::LoadEntries( applicationId, instanceId, level, start );
		if( pTraces->values_size() )
		{
			pTraces->set_applicationid( applicationId );
			DBG( "MySession::SendLogs({}, {}) write {}", instanceId, (uint)level, pTraces->values_size() );
			MyFromServer transmission;
			transmission.add_messages()->set_allocated_traces( pTraces );
			Write( transmission );
		}
		else
			DBG( "MySession::SendLogs({}, {}) None to write", instanceId, (uint)level );
	}
	void MySession::WriteCustom( uint32 clientId, const string& message )noexcept
	{
		var pCustom = new FromServer::Custom();
		pCustom->set_requestid( clientId );
		pCustom->set_message( message );
		MyFromServer transmission; transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}

	void MySession::WriteError( string&& msg, uint32 requestId )noexcept
	{
		DBG( "WriteError( '{}', '{}' )", requestId, msg );
		var pError = new FromServer::ErrorMessage();
		pError->set_requestid( requestId );
		pError->set_message( msg );
		MyFromServer transmission; transmission.add_messages()->set_allocated_error( pError );
		Write( transmission );
	}
	bool MySession::Write( const MyFromServer& transmission  )noexcept
	{
		var pData = TryToBuffer( transmission );
		if( pData )
			Write2( *pData );
		return pData!=nullptr;
	}
	void MySession::PushMessage( ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept
	{
		auto pTraces = new FromServer::Traces();
		pTraces->set_applicationid( applicationId );
		auto pTrace = pTraces->add_values();
		pTrace->set_instanceid( instanceId );
		pTrace->set_time( Chrono::MillisecondsSinceEpoch(time) );
		pTrace->set_level( (FromServer::ELogLevel)level );
		pTrace->set_messageid( messageId );
		pTrace->set_fileid( fileId );
		pTrace->set_functionid( functionId );
		pTrace->set_linenumber( lineNumber );
		pTrace->set_userid( userId );
		pTrace->set_threadid( threadId );
		for( var& variable : variables )
			pTrace->add_variables( variable );

		MyFromServer transmission;
		transmission.add_messages()->set_allocated_traces( pTraces );
		Write( transmission );
	}
}