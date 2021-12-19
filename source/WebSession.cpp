#include "Cache.h"
#include "WebSession.h"
#include "WebServer.h"
#include "Listener.h"
#include "LogData.h"
#include "../../Framework/source/DateTime.h"

#define var const auto
#define _listener TcpListener::GetInstance()

namespace Jde::ApplicationServer::Web
{
	static const LogTag& _logLevel = Logging::TagLevel( "app-requests" );

	MySession::MySession( WebSocket::WebListener& server, IO::Sockets::SessionPK id, tcp::socket&& socket )noexcept(false):
		base( server, id, move(socket) )
	{}

	MySession::~MySession()
	{
		LOG( "({})~MySession()"sv, Id );
	}

	α MySession::Run()noexcept->void
	{
		base::Run();
		LOG( "({})MySession::Run()"sv, Id );
		Server().UpdateStatus( Server() );
	}
	α MySession::OnAccept( beast::error_code ec )noexcept->void
	{
		LOG( "({})MySession::OnAccept()"sv, Id );
		auto pAck{ mu<Web::FromServer::Acknowledgement>() }; pAck->set_id( (uint32)Id );
		FromServer::Transmission t; t.add_messages()->set_allocated_acknowledgement( pAck.release() );
		Write( t );
		base::OnAccept( ec );
	}

	α MySession::Server()noexcept->WebServer&
	{
		return dynamic_cast<WebServer&>( _server );
	}

	α MySession::OnRead( FromClient::Transmission t )noexcept->void
	{
		for( α i=0; i<t.messages_size(); ++i )
		{
			auto pMessage = t.mutable_messages( i );
			if( pMessage->has_request() )
			{
				var& request = pMessage->request();
				if( request.value()==FromClient::ERequest::Statuses )
					SendStatuses();
				else if( request.value()==(FromClient::ERequest::Statuses|FromClient::ERequest::Negate) )
				{
					Server().RemoveStatusSession( Id );
					LOG( "({})Remove status subscription."sv, Id );
				}
				else if( request.value() == FromClient::ERequest::Applications )
				{
					auto pApplications = Logging::Data::LoadApplications();
					LOG( "({})Writing Applications count='{}'"sv, Id, pApplications->values_size() );
					FromServer::Transmission transmission; transmission.add_messages()->set_allocated_applications( pApplications.release() );
					Write( transmission );
				}
				else
					WARN( "unsupported request '{}'"sv, request.value() );
			}
			else if( pMessage->has_requestid() )
			{
				var& request = pMessage->requestid();
				const ApplicationInstancePK instanceId = request.instanceid();
				var value = (int)request.value();
				if( value == -2 )//(int)(FromClient::ERequest::Power | FromClient::ERequest::Negate);
					_listener.Kill( instanceId );
				else if( value == -3 )//(int)(FromClient::ERequest::Logs | FromClient::ERequest::Negate);
					Server().RemoveLogSubscription( Id, instanceId );
				else if( value == FromClient::ERequest::Power )
					WARN( "unsupported request Power"sv );
				else
					WARN( "unsupported request '{}'"sv, request.value() );
			}
			else if( pMessage->has_logvalues() )
			{
				var& values = pMessage->logvalues();
				if( values.dbvalue()<ELogLevelStrings.size() && values.clientvalue()<ELogLevelStrings.size() )
					LOG( "({})SetLogLevel for instance='{}', db='{}', client='{}'"sv, Id, values.instanceid(), ELogLevelStrings[values.dbvalue()], ELogLevelStrings[values.clientvalue()] );
				Logging::Proto::LogLevels levels;
				_listener.SetLogLevel( values.instanceid(), (ELogLevel)values.dbvalue(), (ELogLevel)values.clientvalue() );
			}
			else if( pMessage->has_requestlogs() )
			{
				var value = pMessage->requestlogs();
				if( value.value()<ELogLevelStrings.size() )
					LOG( "({})AddLogSubscription application='{}' instance='{}', level='{}'"sv, Id, value.applicationid(), value.instanceid(), ELogLevelStrings[value.value()] );
				if( Server().AddLogSubscription(Id, value.applicationid(), value.instanceid(), (ELogLevel)value.value()) )//if changing level, don't want to send old logs
					std::thread{ [self=dynamic_pointer_cast<MySession>(shared_from_this()),value]()
					{
						SendLogs(self,value.applicationid(), value.instanceid(), (ELogLevel)value.value(), value.start(), value.limit());
					}}.detach();
			}
			else if( pMessage->has_custom() )
			{
				auto pCustom = pMessage->mutable_custom();
				LOG( "({})received From Web custom reqId='{}' for application='{}'"sv, Id, pCustom->requestid(), pCustom->applicationid() );
				try
				{
					_listener.WriteCustom( pCustom->applicationid(), pCustom->requestid(), move(*up<string>(pCustom->release_message())) );
				}
				catch( const IException& e )
				{
					WriteError( e.what(), pCustom->requestid() );
				}
			}
			else if( pMessage->has_requeststrings() )
				SendStrings( pMessage->requeststrings() );
			else
				ERR( "Unknown message:  {}"sv, (uint)pMessage->Value_case() );
		}
	};

	α MySession::SendStatuses()noexcept->void
	{
		auto pStatuses = new FromServer::Statuses();
		Server().SetStatus( *pStatuses->add_values() );
		std::function<void( const IO::Sockets::SessionPK&, const ApplicationServer::Session& session )> fncn = [pStatuses]( const IO::Sockets::SessionPK& /*id*/, const ApplicationServer::Session& session )
		{
			session.SetStatus( *pStatuses->add_values() );
		};
		_listener.ForEachSession( fncn );
		FromServer::Transmission t;
		t.add_messages()->set_allocated_statuses( pStatuses );
		Try( [&t,this]{Write(t);} );
		LOG( "({})Add status subscription."sv, Id );
		Server().AddStatusSession( Id );
	}
	α MySession::SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )noexcept->void
	{
		std::optional<TimePoint> time = start ? Clock::from_time_t(start) : std::optional<TimePoint>{};
		auto pTraces = Logging::Data::LoadEntries( applicationId, instanceId, level, time, limit );
		pTraces->add_values();//Signify end.

		pTraces->set_applicationid( (google::protobuf::uint32)applicationId );
		LOG( "({})MySession::SendLogs({}, {}) write {}"sv, self->Id, applicationId, (uint)level, pTraces->values_size()-1 );
		FromServer::Transmission transmission;
		transmission.add_messages()->set_allocated_traces( pTraces.release() );
		self->Write( transmission );
	}
	α MySession::SendStrings( const FromClient::RequestStrings& request )noexcept->void
	{
		var reqId = request.requestid();
		TRACE( "({}) requeststrings count='{}'"sv, Id, request.values_size() );
		map<ApplicationPK,std::forward_list<FromServer::ApplicationString>> values;
		for( auto i=0; i<request.values_size(); ++i )
		{
			var& value = request.values( i );
			auto pStrings = Cache::AppStrings( value.applicationid() );
			if( !pStrings )
				pStrings = Cache::Load( value.applicationid() );//todo wrap in try statement.
			sp<string> pString;
			if( value.type()==FromClient::EStringRequest::MessageString )
				pString = pStrings->Get( Logging::EFields::Message, value.value() );
			else if( value.type()==FromClient::EStringRequest::File )
				pString = pStrings->Get( Logging::EFields::File, value.value() );
			else if( value.type()==FromClient::EStringRequest::Function )
				pString = pStrings->Get( Logging::EFields::Function, value.value() );

			else if( value.type()==FromClient::EStringRequest::User )
				pString = pStrings->Get( Logging::EFields::User, value.value() );
			if( pString )
			{
				FromServer::ApplicationString appString; appString.set_stringrequesttype( value.type() ); appString.set_id( value.value() ); appString.set_value( *pString );
				auto& strings = values.try_emplace(value.applicationid(), std::forward_list<FromServer::ApplicationString>{} ).first->second;
				strings.push_front( appString );
			}
			else
			{
				static constexpr array<sv,5> StringTypes = {"Message","File","Function","Thread","User"};
				const string typeString = value.type()<(int)StringTypes.size() ? string(StringTypes[value.type()]) : std::to_string( value.type() );
				WARN( "Could not find string type='{}', id='{}', application='{}'"sv, typeString, value.value(), value.applicationid() );
				FromServer::ApplicationString appString; appString.set_stringrequesttype( value.type() ); appString.set_id( value.value() ); appString.set_value( "{{error}}" );
				auto& strings = values.try_emplace(value.applicationid(), std::forward_list<FromServer::ApplicationString>{} ).first->second;
				strings.push_front( appString );
			}
		}

		FromServer::Transmission transmission;
		for( var& [id,strings] : values )
		{
			auto pStrings = new FromServer::ApplicationStrings();
			pStrings->set_requestid( reqId );
			pStrings->set_applicationid( (google::protobuf::uint32)id );
			for( var& value : strings )
				*pStrings->add_values() = value;
			transmission.add_messages()->set_allocated_strings( pStrings );
		}
		auto pStrings = new FromServer::ApplicationStrings(); pStrings->set_requestid( reqId );
		transmission.add_messages()->set_allocated_strings( pStrings );//finished.
		Write( transmission );
	}
	α MySession::WriteCustom( uint32 clientId, const string& message )noexcept->void
	{
		var pCustom = new FromServer::Custom();
		pCustom->set_requestid( clientId );
		pCustom->set_message( message );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}
	α MySession::WriteComplete( uint32 clientId )noexcept->void
	{
		var pCustom = new FromServer::Complete();
		pCustom->set_requestid( clientId );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_complete( pCustom );
		Write( transmission );
	}

	α MySession::WriteError( string&& msg, uint32 requestId )noexcept->void
	{
		LOG( "({})WriteError( '{}', '{}' )"sv, Id, requestId, msg );
		var pError = new FromServer::ErrorMessage();
		pError->set_requestid( requestId );
		pError->set_message( msg );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_error( pError );
		Write( transmission );
	}
	α MySession::Write( const FromServer::Transmission& t  )noexcept(false)->void
	{
		base::Write( make_unique<string>(IO::Proto::ToString(t)) );
	}
	α MySession::PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept->void
	{
		auto pTraces = new FromServer::Traces();
		pTraces->set_applicationid( (google::protobuf::uint32)applicationId );
		auto pTrace = pTraces->add_values();
		pTrace->set_id( id );
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

		FromServer::Transmission transmission;
		transmission.add_messages()->set_allocated_traces( pTraces );
		Write( transmission );
	}
}