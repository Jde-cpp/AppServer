#include "HttpRequestAwait.h"
#include "../../Framework/source/io/AsioContextThread.h"
#include "../../Framework/source/um/UM.h"
#include "GoogleLogin.h"
#include "Server.h"
#include "types/rest/json.h"
#define var const auto

namespace Jde::App{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α ValueJson( string&& value )ι{ return Json::Parse( Jde::format("{{\"value\": \"{}\"}}", value) ); }

	α GoogleLogin( HttpRequest&& req, HttpRequestAwait::Handle h )ι->Task{
		try{
			req.LogReceived();
			json body = req.Body();
			var j{ body.find("value") };
			if( j==body.end() || j->is_null() || !j->is_string() || j->get<string>().empty() )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(req), "value not found." };
			var info = ( co_await GoogleLogin::Verify(j->get<string>()) ).UP<Google::TokenInfo>();
			req.SessionInfo.UserPK = *(co_await UM::Login(info->Email, (uint)UM::EProviderType::Google)).UP<UserPK>();
			h.promise().SetValue( {move(req), ValueJson( Jde::format("{:x}", req.SessionInfo.SessionId))} );
		}
		catch( IException& e ){
			h.promise().SetError( e.Move() );
		}
		h.resume();
	}


	α HttpRequestAwait::await_ready()ι->bool{
		optional<json> result;
		if( _request.Method() == http::verb::get ){
			if( _request.Target()=="/GoogleAuthClientId" ){
				_request.LogReceived();
				_readyResult = mu<json>( ValueJson(Settings::Get<string>("GoogleAuthClientId").value_or("GoogleAuthClientId Not Configured.")) );
			}
			else if( _request.Target()=="/IotWebSocket" ){
				_request.LogReceived();
				var apps = FindApplications( "Jde.IotWebSocket" );
				json japps = json::array();
				for( auto& app : apps ){
					json a;
					to_json( a, app );
					japps.push_back( a );
				}
				_readyResult = mu<json>();
				(*_readyResult)["servers"] = japps;
			}
		}
		return _readyResult!=nullptr;
	}
	α HttpRequestAwait::await_suspend( base::Handle h )ε->void{
		base::await_suspend(h);
		up<IException> pException;
		if( _request.Method() == http::verb::post ){
			if( _request.Target()=="/GoogleLogin" )
				GoogleLogin( move(_request), h );
		}
		if( _request.Target().size() ){
			_request.LogReceived();
			RestException<http::status::not_found> e{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
			h.promise().SetError( mu<RestException<http::status::not_found>>(move(e)) );
			h.resume();
		}
	}

	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		if( auto e = Promise() ? Promise()->Error() : nullptr; e ){
			auto pRest = dynamic_cast<IRestException*>( e.get() );
			if( pRest )
				pRest->Throw();
			else
				throw RestException<http::status::internal_server_error>{ SRCE_CUR, move(_request), move(*e), "" };
		}
		return _readyResult
			? HttpTaskResult{ move(_request), move(*_readyResult) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ move(_request), json{} };
	}
}