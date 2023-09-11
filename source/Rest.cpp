#include "Rest.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Framework/source/math/MathUtilities.h"
#include "GoogleLogin.h"
#include "WebSession.h"


#define var const auto
namespace Jde::ApplicationServer
{
	sp<TListener<RestSession>> _restListener{ ms<TListener<RestSession>>(Settings::Get<PortType>("web/restPort").value_or(1999)) };
	α Listener()ι->TListener<RestSession>&{return *_restListener;}

	α GoogleLogin( http::request<http::string_body>&& req, sp<ISession> s )ι->Task;

	α SendValue( string&& value, http::request<http::string_body>&& req, sp<ISession>&& s )ι->void
	{
		s->Send( format("{{\"value\": \"{}\"}}", value), move(req) );
	}

	α RestSession::HandleRequest( string&& target, flat_map<string,string>&& params, up<SessionInfo> pSessionInfo, http::request<http::string_body>&& req, sp<ISession> s )ι->void
	{
		try
		{
	    if( req.method() == http::verb::get )
			{
				if( target=="/GoogleAuthClientId" )
					SendValue( Settings::Getɛ<string>("GoogleAuthClientId"), move(req), move(s) );
				// else if( target.starts_with("/graphql?") && target.size()>9 )
				// {
				// 	sv target2 = "/graphql";
				// 	uint start = target2.size()+1;
				// 	auto psz = iv{target.data()+start, target.size()-start};
				// 	var params = Str::Split<iv,iv>(psz, '&');
				// 	for( auto param : params )
				// 	{
				// 		var keyValue = Str::Split<iv,iv>( param, '=' );
				// 		if( keyValue.size()==2 && keyValue[0]=="query" )
				// 		{
				// 			SendQuery( ToSV(keyValue[1]), move(req), s );
				// 			break;
				// 		}
				// 	}
				// }
				else
					s->Error( http::status::bad_request, target, move(req) );
			}
			else if( req.method() == http::verb::post )
			{
				if( target=="/GoogleLogin" )
				{
					GoogleLogin( move(req), move(s) );
					//s->Send( result, move(req) );
				}
				else
					s->Error( http::status::bad_request, target, move(req) );
			}
			else if( req.method() == boost::beast::http::verb::options )
			{
				s->SendOptions( move(req) );
			}
			else
	      s->Error( http::status::bad_request, format("Only get/put verb is supported {}",req.method()), move(req) );
		}
		catch( const Exception& e )
		{
			s->Error( http::status::internal_server_error, e.what(), move(req) );
		}
	}


	α RestSession::GoogleLogin( http::request<http::string_body>&& req, sp<ISession> s )ι->Task
	{
		try
		{
			string& bodyv = req.body();
			Dbg( bodyv );
			json body = json::parse( bodyv );
			var j{ body.find("value") };
			if( j->is_null() || !j->is_string() || j->get<string>().empty() )
				throw RequestException<http::status::bad_request>();
			var info = ( co_await GoogleLogin::Verify(j->get<string>()) ).UP<Google::TokenInfo>();
			var p = ( co_await DB::ScalerCo<UserPK>( "select id from um_users where name=? and authenticator_id=?", {info->Email,(uint)Web::EAuthType::Google} ) ).UP<UserPK>();
			var userId = p ? *p : 0;
			if( !userId )
				throw RequestException<http::status::bad_request>{ SRCE_CUR, "'{}' not found.", info->Email };

			var sessionInfo = ISession::AddSession( userId );

			SendValue( format("{:x}", sessionInfo->session_id()), move(req), move(s) );
		}
		catch( const nlohmann::json::exception& e )
		{
			DBG( "BadRequest({}) - {}", req.body(), e.what() );
			s->Error( http::status::bad_request, req.body(), move(req) );
		}
		catch( const IRequestException& e )
		{
			string error = e.What().size() ? e.What() : req.body();
			s->Error( e, error, move(req) );
		}
		catch( const Exception& e )
		{
			s->Error( http::status::internal_server_error, e.what(), move(req) );
		}
	}
}
