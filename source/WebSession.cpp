#include "WebSession.h"
#include <boost/algorithm/hex.hpp>
#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Google/source/TokenInfo.h"
#include "Cache.h"
#include "WebServer.h"
#include "ResultsMessage.h"
#include "Listener.h"
#include "LogData.h"

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
//		Server().UpdateStatus( Server() );
	}
	α MySession::OnAccept( beast::error_code ec )noexcept->void
	{
		LOG( "({})MySession::OnAccept()"sv, Id );
		auto pAck{ mu<FromServer::Acknowledgement>() }; pAck->set_id( (uint32)Id );
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
		var c = t.messages_size();
		for( α i=0; i<c; ++i )
		{
			auto pMessage = t.mutable_messages( i );
			if( pMessage->has_graph_ql() )
			{
				auto ql = pMessage->mutable_graph_ql();
				GraphQL( move(*ql->mutable_query()), ql->request_id() );
			}
			else if( pMessage->has_request() )
			{
				var& request = pMessage->request();
				if( request.type()==FromClient::ERequest::Statuses )
					SendStatuses();
				else if( request.type()==(FromClient::ERequest::Statuses|FromClient::ERequest::Negate) )
				{
					Server().RemoveStatusSession( Id );
					LOGT( Web::_logLevel, "({})Remove status subscription."sv, Id );
				}
				else if( request.type() == FromClient::ERequest::Applications )
				{
					auto pApplications = Logging::Data::LoadApplications();
					LOGT( Web::_logLevel, "({})Writing Applications count='{}'"sv, Id, pApplications->values_size() );
					FromServer::Transmission transmission; transmission.add_messages()->set_allocated_applications( pApplications.release() );
					Write( transmission );
				}
				else
					WARN( "unsupported request '{}'", request.type() );
			}
			else if( pMessage->has_request_app() )
			{
				var& request = pMessage->request_app();
				var instanceId = (ApplicationInstancePK)request.instance_id();
				var value = (int)request.type();
				if( value == -2 )//(int)(FromClient::ERequest::Power | FromClient::ERequest::Negate);
					_listener.Kill( instanceId );
				else if( value == -3 )//(int)(FromClient::ERequest::Logs | FromClient::ERequest::Negate);
					Server().RemoveLogSubscription( Id, instanceId );
				else if( value == FromClient::ERequest::Power )
					WARN( "unsupported request Power" );
				else
					WARN( "unsupported request '{}'", request.type() );
			}
			else if( pMessage->has_log_values() )
			{
				var& values = pMessage->log_values();
				if( values.db_value()<ELogLevelStrings.size() && values.client_value()<ELogLevelStrings.size() )
					LOGT( Web::_logLevel, "({})SetLogLevel for instance='{}', db='{}', client='{}'"sv, Id, values.instance_id(), ELogLevelStrings[values.db_value()], ELogLevelStrings[values.client_value()] );
				Logging::Proto::LogLevels levels;
				_listener.SetLogLevel((ApplicationInstancePK)values.instance_id(), (ELogLevel)values.db_value(), (ELogLevel)values.client_value() );
			}
			else if( pMessage->has_request_logs() )
			{
				var value = pMessage->request_logs();
				if( value.value()<ELogLevelStrings.size() )
					LOGT( Web::_logLevel, "({})AddLogSubscription application='{}' instance='{}', level='{}'"sv, Id, value.application_id(), value.instance_id(), ELogLevelStrings[value.value()] );
				if( Server().AddLogSubscription(Id, (ApplicationPK)value.application_id(), (ApplicationInstancePK)value.instance_id(), (ELogLevel)value.value()) )//if changing level, don't want to send old logs
					std::thread{ [self=dynamic_pointer_cast<MySession>(shared_from_this()),value]()
					{
						SendLogs(self, (ApplicationPK)value.application_id(), (ApplicationInstancePK)value.instance_id(), (ELogLevel)value.value(), value.start(), value.limit());
					}}.detach();
			}
			else if( pMessage->has_custom() )
			{
				auto pCustom = pMessage->mutable_custom();
				LOGT( Web::_logLevel, "({})received From Web custom reqId='{}' for application='{}'"sv, Id, pCustom->request_id(), pCustom->application_id() );
				try
				{
					_listener.WriteCustom( pCustom->application_id(), pCustom->request_id(), move(*up<string>(pCustom->release_message())) );
				}
				catch( const IException& e )
				{
					WriteError( e.what(), pCustom->request_id() );
				}
			}
			else if( pMessage->has_request_strings() )
				SendStrings( pMessage->request_strings() );
			else if( pMessage->has_request_value() )
			{
				auto p = pMessage->mutable_request_value();
				var id = p->request_id();
				var type = (FromClient::ERequest)p->type();
				if( type==FromClient::ERequest::GoogleLogin && p->has_string() )
					GoogleLogin( move(*p->mutable_string()), id );
				else if( type==FromClient::ERequest::GoogleAuthClientId )
					GoogleAuthClientId( p->request_id() );
			}
			else
				ERR( "Unknown message:  {}"sv, (uint)pMessage->value_case() );
		}
	};

	α MySession::GraphQL( string&& query, ClientId clientId )ι->Task
	{
		Web::FromServer::MessageUnion y;
		try
		{
			var result = ( co_await DB::CoQuery(move(query), UserId) ).UP<nlohmann::json>();
			y = ToMessageQL( result->dump(), clientId );
		}
		catch( const json::exception& e )
		{
			y = ToError( "Could not parse query", clientId );
		}
		catch( const IException& e )
		{
			y = ToError( e.what(), clientId );
		}
		co_await WebServer::CoSend( move(y), Id );
	}

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

		pTraces->set_application_id( (google::protobuf::uint32)applicationId );
		LOG( "({})MySession::SendLogs({}, {}) write {}"sv, self->Id, applicationId, (uint)level, pTraces->values_size()-1 );
		FromServer::Transmission transmission;
		transmission.add_messages()->set_allocated_traces( pTraces.release() );
		self->Write( transmission );
	}
	α MySession::SendStrings( const FromClient::RequestStrings& request )noexcept->void
	{
		var reqId = request.request_id();
		TRACE( "({}) requeststrings count='{}'"sv, Id, request.values_size() );
		map<ApplicationPK,std::forward_list<FromServer::ApplicationString>> values;
		for( auto i=0; i<request.values_size(); ++i )
		{
			var& value = request.values( i );
			auto& strings = Cache::AppStrings();
			sp<string> pString;
			if( value.type()==FromClient::EStringRequest::MessageString )
				pString = strings.Get( Logging::EFields::Message, value.value() );
			else if( value.type()==FromClient::EStringRequest::File )
				pString = strings.Get( Logging::EFields::File, value.value() );
			else if( value.type()==FromClient::EStringRequest::Function )
				pString = strings.Get( Logging::EFields::Function, value.value() );

			else if( value.type()==FromClient::EStringRequest::User )
				pString = strings.Get( Logging::EFields::User, value.value() );
			if( pString )
			{
				FromServer::ApplicationString appString; appString.set_string_request_type( value.type() ); appString.set_id( value.value() ); appString.set_value( *pString );
				auto& strings2 = values.try_emplace(value.application_id(), std::forward_list<FromServer::ApplicationString>{} ).first->second;
				strings2.push_front( appString );
			}
			else
			{
				static constexpr array<sv,5> StringTypes = {"Message","File","Function","Thread","User"};
				const string typeString = value.type()<(int)StringTypes.size() ? string(StringTypes[value.type()]) : std::to_string( value.type() );
				WARN( "Could not find string type='{}', id='{}', application='{}'"sv, typeString, value.value(), value.application_id() );
				FromServer::ApplicationString appString; appString.set_string_request_type( value.type() ); appString.set_id( value.value() ); appString.set_value( "{{error}}" );
				auto& strings2 = values.try_emplace(value.application_id(), std::forward_list<FromServer::ApplicationString>{} ).first->second;
				strings2.push_front( appString );
			}
		}

		FromServer::Transmission transmission;
		for( var& [id,strings] : values )
		{
			auto pStrings = new FromServer::ApplicationStrings();
			pStrings->set_request_id( reqId );
			pStrings->set_application_id( (google::protobuf::uint32)id );
			for( var& value : strings )
				*pStrings->add_values() = value;
			transmission.add_messages()->set_allocated_strings( pStrings );
		}
		auto pStrings = new FromServer::ApplicationStrings(); pStrings->set_request_id( reqId );
		transmission.add_messages()->set_allocated_strings( pStrings );//finished.
		Write( transmission );
	}
	α MySession::WriteCustom( uint32 clientId, const string& message )noexcept->void
	{
		var pCustom = new FromServer::Custom();
		pCustom->set_request_id( clientId );
		pCustom->set_message( message );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_custom( pCustom );
		Write( transmission );
	}
	α MySession::WriteComplete( uint32 clientId )noexcept->void
	{
		var pCustom = new FromServer::Complete();
		pCustom->set_request_id( clientId );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_complete( pCustom );
		Write( transmission );
	}

	α MySession::WriteError( string&& msg, uint32 requestId )noexcept->void
	{
		LOG( "({})WriteError( '{}', '{}' )"sv, Id, requestId, msg );
		var pError = new FromServer::ErrorMessage();
		pError->set_request_id( requestId );
		pError->set_message( msg );
		FromServer::Transmission transmission; transmission.add_messages()->set_allocated_error( pError );
		Write( transmission );
	}
	α MySession::Write( const FromServer::Transmission& t  )noexcept(false)->void
	{
		base::Write( mu<string>(IO::Proto::ToString(t)) );
	}
	α MySession::PushMessage( LogPK id, ApplicationInstancePK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept->void
	{
		auto pTraces = new FromServer::Traces();
		pTraces->set_application_id( (google::protobuf::uint32)applicationId );
		auto pTrace = pTraces->add_values();
		pTrace->set_id( id );
		pTrace->set_instance_id( instanceId );
		pTrace->set_time( Chrono::MillisecondsSinceEpoch(time) );
		pTrace->set_level( (FromServer::ELogLevel)level );
		pTrace->set_message_id( messageId );
		pTrace->set_file_id( fileId );
		pTrace->set_function_id( functionId );
		pTrace->set_line_number( lineNumber );
		pTrace->set_user_id( userId );
		pTrace->set_thread_id( threadId );
		for( var& variable : variables )
			pTrace->add_variables( variable );

		FromServer::Transmission transmission;
		transmission.add_messages()->set_allocated_traces( pTraces );
		Write( transmission );
	}

	α MySession::GoogleLogin( string&& credential, ClientId clientId )ι->Task
	{
		constexpr EAuthType type{ EAuthType::Google };
		var parts = Str::Split(credential, '.');
		Google::TokenInfo token;
		Web::FromServer::MessageUnion y;
		try
		{
			var header = json::parse( Ssl::Decode64(string{parts[0]}) );//{"alg":"RS256","kid":"fed80fec56db99233d4b4f60fbafdbaeb9186c73","typ":"JWT"}
			var body = json::parse( Ssl::Decode64(string{parts[1]}) );
			token = body.get<Google::TokenInfo>();

			json jOpenidConfiguration = json::parse( Ssl::Get<string>("accounts.google.com", "/.well-known/openid-configuration") );
			var pJwksUri = jOpenidConfiguration.find( "jwks_uri" ); THROW_IF( pJwksUri==jOpenidConfiguration.end(), "Could not find jwks_uri in '{}'", jOpenidConfiguration.dump() );
			var uri = pJwksUri->get<string>(); THROW_IF( !uri.starts_with("https://www.googleapis.com"), "Wrong target:  '{}'", uri );
			//TODO cache this.
			var jwks = json::parse( Ssl::Get<string>( "www.googleapis.com", uri.substr(sizeof("https://www.googleapis.com")-1)) );
			var pKid = header.find( "kid" ); THROW_IF( pKid== header.end(), "Could not find kid in header {}", header.dump() );
			var kidString = pKid->get<string>();
			var pKeys = jwks.find( "keys" );  THROW_IF( pKeys==jwks.end(), "Could not find pKeys in jwks {}", jwks.dump() );
			json foundKey;
			for( var& key : *pKeys )
			{
				var keyString = key["kid"].get<string>();
				if( keyString==kidString )
				{
					foundKey = key;
#ifndef NDEBUG
					if( var f = IApplication::ApplicationDataFolder()/(keyString+".json"); !fs::exists(f) )
						co_await IO::Write( f, ms<string>(foundKey.dump()) );
#endif
					break;
				}
				//break;
			}
#ifndef NDEBUG
			if( foundKey.is_null() && fs::exists(IApplication::ApplicationDataFolder()/(kidString+".json")) )
				foundKey = json::parse( IO::FileUtilities::Load(IApplication::ApplicationDataFolder()/(kidString+".json")) );
#endif
			THROW_IF( foundKey.is_null(), "Could not find key '{}' in: '{}'", pKid->get<string>(), pKeys->dump() );
			var alg = foundKey["alg"].get<string>();
			var exponent = foundKey["e"].get<string>();
			var modulus = foundKey["n"].get<string>();
			//var token = Ssl::Get<Google::TokenInfo>( "oauth2.googleapis.com", format("/tokeninfo?id_token={}"sv, credential) );
			Ssl::Verify( Ssl::Decode64<vector<unsigned char>>(modulus, true), Ssl::Decode64<vector<unsigned char>>(exponent, true), string{parts[0]}+'.'+string{parts[1]}, Ssl::Decode64(string{parts[2]}, true) );
			THROW_IF(token.Aud != Settings::Get<string>("GoogleAuthClientId"), "Invalid client id");
			THROW_IF(token.Iss != "accounts.google.com" && token.Iss != "https://accounts.google.com", "Invalid iss");
			var expiration = Clock::from_time_t(token.Expiration);
#ifdef NDEBUG
			THROW_IF(expiration < Clock::now(), "token expired");
#endif
			//var _ = ( co_await CoLockKey( "MySession::GoogleLogin", true) ).UP<CoLockGuard>();

			AuthType = type;
			Email = token.Email;
			EmailVerified = token.EmailVerified;
			Name = token.Name;
			PictureUrl = token.PictureUrl;
			Expiration = expiration;
			var p = ( co_await DB::ScalerCo<UserPK>( "select id from um_users where name=? and authenticator_id=?", {Email,(uint)AuthType} ) ).UP<UserPK>();
			if( p )
				UserId = *p;
			if( !UserId )
				throw Exception{ SRCE_CUR, Jde::ELogLevel::Debug, "'{}' not found", Email };

			y = ToMessage( FromServer::EResults::Authentication, clientId );
		}
		catch( const nlohmann::json::exception& e )
		{
			CRITICAL( "json exception - {}"sv, e.what() );
			y = ToError( "Authentication Failed", clientId );
		}
		catch( const Exception& )
		{
			y = ToError( "Authentication Failed", clientId );
		}
		co_await WebServer::CoSend( move(y), Id );
	}
	α MySession::GoogleAuthClientId( ClientId clientId )ι->Task
	{
		Web::FromServer::MessageUnion y;
		try
		{
			y = ToMessage( Settings::Getɛ<string>("GoogleAuthClientId"), clientId );
		}
		catch( const Exception& e )
		{
			y = ToError( e.what(), clientId );
		}
		co_await WebServer::CoSend( move(y), Id );
	}
}