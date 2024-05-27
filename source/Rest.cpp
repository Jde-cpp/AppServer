#include "Rest.h"
#include "../../Framework/source/db/GraphQL.h"
#include "../../Framework/source/math/MathUtilities.h"
#include "GoogleLogin.h"
#include "WebSession.h"
#include "Listener.h"
#include "types/rest/json.h"


#define var const auto
namespace Jde::ApplicationServer{
	sp<LogTag> _logTag = Logging::Tag( "app.rest" );

	sp<TListener<RestSession>> _restListener{};//each session has a sp.

	α Listener()ε->TListener<RestSession>&{ THROW_IF( !_restListener, "RestListener not started" ); return *_restListener; }
	α StartRestService()ι->void{ _restListener = ms<TListener<RestSession>>(Settings::Get<PortType>("web/restPort").value_or(1999)); _restListener->Run(); }
	
	α SendValue( string&& value, Request&& req )ι->void{
		ISession::Send( Jde::format("{{\"value\": \"{}\"}}", value), move(req) );
	}
	
	α RestSession::HandleRequest( string&& target, flat_map<string,string>&& /*params*/, Request&& req )ι->void{
		try{
	    if( req.Method() == http::verb::get ){
				if( target=="/GoogleAuthClientId" )
					SendValue( Settings::Getɛ<string>("GoogleAuthClientId"), move(req) );
				else if( target=="/IotWebSocket" ){
					var apps = ApplicationServer::TcpListener::GetInstance().FindApplications( "Jde.IotWebSocket" );
					json japps = json::array();
					for( auto& p : apps ){
						json a;
						to_json( a, *p );
						japps.push_back( a );
					}
					json j;
					j["servers"] = japps;
					Send( j, move(req) );
				}
				else
					Send( http::status::bad_request, move(target), move(req) );
			}
			else if( req.Method() == http::verb::post ){
				if( target=="/GoogleLogin" )
					GoogleLogin( move(req) );
				else
					Send( http::status::bad_request, target, move(req) );
			}
			else if( req.Method() == boost::beast::http::verb::options )
				SendOptions( move(req) );
			else{
				Send( http::status::bad_request, Jde::format("Only get/put verb is supported {}", string{to_string(req.Method())}), move(req) );
			}
		}
		catch( const Exception& e ){
			Send( http::status::internal_server_error, e.what(), move(req) );
		}
	}


	α RestSession::GoogleLogin( Request req )ι->Task{
		string bodyv = req.ClientRequest().body();
		try{
//			Dbg( bodyv );
			json body = json::parse( bodyv );
			var j{ body.find("value") };
			if( j==body.end() || j->is_null() || !j->is_string() || j->get<string>().empty() )
				throw RequestException<http::status::bad_request>{ SRCE_CUR, "value not found." };
			var info = ( co_await GoogleLogin::Verify(j->get<string>()) ).UP<Google::TokenInfo>();
			var userId = *(co_await UM::Login(info->Email, (uint)UM::EProviderType::Google)).UP<UserPK>();
			var sessionInfo = ISession::AddSession( userId );

			SendValue( Jde::format("{:x}", sessionInfo->session_id()), move(req) );
		}
		catch( const nlohmann::json::exception& e ){
			DBG( "BadRequest({}) - {}", bodyv, e.what() );
			Send( http::status::bad_request, bodyv, move(req) );
		}
		catch( IRequestException& e ){
			Send( move(e), move(req) );
		}
		catch( const Exception& e ){
			Send( http::status::internal_server_error, e.what(), move(req) );
		}
	}
}