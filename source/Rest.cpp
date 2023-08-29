#include "Rest.h"
#include "../../Framework/source/math/MathUtilities.h"
#include "GoogleLogin.h"
#include "WebSession.h"


#define var const auto
namespace Jde::ApplicationServer::Rest
{
	sp<TListener<Session>> _listener{ ms<TListener<Session>>(Settings::Get<PortType>("web/restPort").value_or(1999)) };
	α Listener()ι->TListener<Session>&{return *_listener;}

	α GoogleLogin( http::request<http::string_body>&& req, sp<ISession> s )ι->Task;

	α Session::HandleRequest( http::request<http::string_body>&& req, sp<ISession> s )ι->void
	{
		try
		{
		  var target = req.target();
	    if( req.method() == http::verb::get )
			{
				if( target=="/GoogleAuthClientId" )
				{
					string result{ format("{{\"value\": \"{}\"}}", Settings::Getɛ<string>("GoogleAuthClientId")) };
					s->Send( result, move(req) );
				}
				else
					s->Error( http::status::bad_request, string{target}, move(req) );
			}
			else if( req.method() != http::verb::put )
			{
				if( target=="/GoogleLogin" )
				{
					GoogleLogin( move(req), move(s) );
					//s->Send( result, move(req) );
				}
				else
					s->Error( http::status::bad_request, string{target}, move(req) );
			}
			else
	      s->Error( http::status::bad_request, format("Only get/put verb is supported {}",req.method()), move(req) );
		}
		catch( const Exception& e )
		{
			s->Error( http::status::internal_server_error, e.what(), move(req) );
		}
	}

	α GoogleLogin( http::request<http::string_body>&& req, sp<ISession> s )ι->Task
	{
		json j{ req.body() };
		try
		{
			var info = ( co_await GoogleLogin::Verify(j["value"]) ).UP<Google::TokenInfo>();
			var p = ( co_await DB::ScalerCo<UserPK>( "select id from um_users where name=? and authenticator_id=?", {info->Email,(uint)Web::EAuthType::Google} ) ).UP<UserPK>();
			var userId = p ? *p : 0;
			if( !userId )
				throw Exception{ SRCE_CUR, Jde::ELogLevel::Debug, "'{}' not found.", info->Email };

			var sessionId = Math::Random();
		}
		catch( const Exception& e )
		{
			s->Error( http::status::internal_server_error, e.what(), move(req) );
		}
	}
}
