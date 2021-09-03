#include "Cache.h"
#include "WebSession.h"
#include "WebServer.h"
#include "Listener.h"
#include "LogData.h"
#include "../../Framework/source/DateTime.h"

#define var const auto
#define _listener Listener::GetInstance()

namespace Jde::ApplicationServer::Web
{
	MySession::MySession( sp<MyServer> pServer, uint id, boost::asio::ip::tcp::socket& socket )noexcept(false):
		Base( pServer, id, socket )
	{}

	MySession::~MySession()
	{
		DBG( "~MySession( {} )"sv, Id );
	}

	void MySession::Start()noexcept
	{
		Session::Start();
		auto pAck = new Web::FromServer::Acknowledgement();
		pAck->set_id( (uint32)Id );
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
		for( uint i=0; i<pTransmission->messages_size(); ++i )
		{
			auto pMessage = pTransmission->mutable_messages( i );
			if( pMessage->has_request() )
			{
				var& request = pMessage->request();
				if( request.value()==FromClient::ERequest::Statuses )
					SendStatuses();
				else if( request.value()==(FromClient::ERequest::Statuses|FromClient::ERequest::Negate) )
				{
					Server().RemoveStatusSession( Id );
					DBG( "({})Remove status subscription."sv, Id );
				}
				else if( request.value() == FromClient::ERequest::Applications )
				{
					auto pApplications = Logging::Data::LoadApplications();
					DBG( "({})Writing Applications count='{}'"sv, Id, pApplications->values_size() );
					MyFromServer transmission; transmission.add_messages()->set_allocated_applications( pApplications.release() );
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
					DBG( "({})SetLogLevel for instance='{}', db='{}', client='{}'"sv, Id, values.instanceid(), ELogLevelStrings[values.dbvalue()], ELogLevelStrings[values.clientvalue()] );
				Logging::Proto::LogLevels levels;
				_listener.SetLogLevel( values.instanceid(), (ELogLevel)values.dbvalue(), (ELogLevel)values.clientvalue() );
			}
			else if( pMessage->has_requestlogs() )
			{
				var value = pMessage->requestlogs();
				if( value.value()<ELogLevelStrings.size() )
					DBG( "({})AddLogSubscription application='{}' instance='{}', level='{}'"sv, Id, value.applicationid(), value.instanceid(), ELogLevelStrings[value.value()] );
				if( Server().AddLogSubscription(Id, value.applicationid(), value.instanceid(), (ELogLevel)value.value()) )//if changing level, don't want to send old logs
					std::thread{ [self=dynamic_pointer_cast<MySession>(shared_from_this()),value](){SendLogs(self,value.applicationid(), value.instanceid(), (ELogLevel)value.value(), value.start(), value.limit());} }.detach();
			}
			else if( pMessage->has_custom() )
			{
				auto pCustom = pMessage->mutable_custom();
				DBG( "({})received From Web custom reqId='{}' for application='{}'"sv, Id, pCustom->requestid(), pCustom->applicationid() );
				try
				{
					_listener.WriteCustom( pCustom->applicationid(), pCustom->requestid(), move(*up<string>(pCustom->release_message())) );
				}
				catch( const Exception& e )
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

	void MySession::SendStatuses()noexcept
	{
		auto pStatuses = new FromServer::Statuses();
		Server().SetStatus( *pStatuses->add_values() );
		std::function<void( const IO::Sockets::SessionPK&, const ApplicationServer::Session& session )> fncn = [pStatuses]( const IO::Sockets::SessionPK& /*id*/, const ApplicationServer::Session& session )
		{
			session.SetStatus( *pStatuses->add_values() );
		};
		_listener.ForEachSession( fncn );
		MyFromServer transmission;
		transmission.add_messages()->set_allocated_statuses( pStatuses );
		if( Write(transmission) )
		{
			DBG( "({})Add status subscription."sv, Id );
			Server().AddStatusSession( Id );
		}
	}
	void MySession::SendLogs( sp<MySession> self, ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, time_t start, uint limit )noexcept
	{
		std::optional<TimePoint> time = start ? Clock::from_time_t(start) : std::optional<TimePoint>{};
		auto pTraces = Logging::Data::LoadEntries( applicationId, instanceId, level, time, limit );
		pTraces->add_values();//Signify end.
		//if( pTraces->values_size() )
		{
			pTraces->set_applicationid( (google::protobuf::uint32)applicationId );
			DBG( "({})MySession::SendLogs({}, {}) write {}"sv, self->Id, applicationId, (uint)level, pTraces->values_size()-1 );
			MyFromServer transmission;
			transmission.add_messages()->set_allocated_traces( pTraces.release() );
			self->Write( transmission );
		}
	}
	void MySession::SendStrings( const FromClient::RequestStrings& request )noexcept
	{
		var reqId = request.requestid();
		TRACE( "({}) requeststrings count='{}'"sv, Id, request.values_size() );
		map<ApplicationPK,std::forward_list<FromServer::ApplicationString>> values;
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

		MyFromServer transmission;
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
	void MySession::WriteCustom( uint32 clientId, const string& message )noexcept
	{
		var pCustom = new FromServer::Custom();
		pCustom->set_requestid( clientId );
		pCustom->set_message( message );
		MyFromServer transmission; transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}
	void MySession::WriteComplete( uint32 clientId )noexcept
	{
		var pCustom = new FromServer::Complete();
		pCustom->set_requestid( clientId );
		MyFromServer transmission; transmission.add_messages()->set_allocated_complete( pCustom );
		Write( transmission );
	}

	void MySession::WriteError( string&& msg, uint32 requestId )noexcept
	{
		DBG( "({})WriteError( '{}', '{}' )"sv, Id, requestId, msg );
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
	void MySession::PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept
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

		MyFromServer transmission;
		transmission.add_messages()->set_allocated_traces( pTraces );
		Write( transmission );
	}
}