#include "HttpRequestAwait.h"
#include <jde/thread/Execution.h>
#include "CertificateLogin.h"
#include "GoogleLogin.h"
#include "Server.h"
#include "types/rest/json.h"
#define var const auto

namespace Jde::App{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α ValueJson( string&& value )ι{ return Json::Parse( 𐢜("{{\"value\": \"{}\"}}", value) ); }

	α CertificateLogin( HttpRequest req, HttpRequestAwait::Handle h )ι->CertificateLoginAwait::Task{
		try{
			req.LogRead();
			req.SessionInfo->UserPK = co_await CertificateLoginAwait( move(req.Body()), req.UserEndpoint.address().to_string() );
			json j{ {"expiration", ToIsoString(req.SessionInfo->Expiration)} };
			req.SessionInfo->IsInitialRequest = true;  //expecting sessionId to be set.
			h.promise().Resume( {move(j), move(req)}, h );
		}
		catch( IException& e ){
			h.promise().ResumeWithError( move(e), h );
		}
	}

	α GoogleLogin( HttpRequest&& req, HttpRequestAwait::Handle h )ι->GoogleLoginAwait::Task{
		try{
			req.LogRead();
			var info = co_await GoogleLoginAwait{ Json::Getε(req.Body(), "value") };
			[&]()->Jde::Task {
				req.SessionInfo->UserPK = *( co_await UM::Login(info.Email, underlying(UM::EProviderType::Google)) ).UP<UserPK>();
				h.promise().SetValue( {ValueJson(𐢜("{:x}", req.SessionInfo->SessionId)), move(req)} );
			}();
		}
		catch( IException& e ){
			h.promise().SetError( move(e) );
		}
		h.resume();
	}

	α HttpRequestAwait::await_ready()ι->bool{
		optional<json> result;
		if( _request.Method() == http::verb::get ){
			if( _request.Target()=="/GoogleAuthClientId" ){
				_request.LogRead();
				_readyResult = mu<json>( ValueJson(Settings::Get<string>("GoogleAuthClientId").value_or("GoogleAuthClientId Not Configured.")) );
			}
			else if( _request.Target()=="/IotWebSocket" ){
				_request.LogRead();
				var apps = Server::FindApplications( "Jde.IotWebSocket" );
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
			else if( _request.Target()=="/CertificateLogin" )
				CertificateLogin( move(_request), h );
		}
		if( _request.Target().size() ){
			_request.LogRead();
			RestException<http::status::not_found> e{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
			h.promise().SetError( RestException<http::status::not_found>(move(e)) );
			h.resume();
		}
	}

	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		if( auto e = Promise() ? Promise()->MoveError() : nullptr; e ){
			auto pRest = dynamic_cast<IRestException*>( e.get() );
			if( pRest )
				pRest->Throw();
			else
				throw RestException<http::status::internal_server_error>{ SRCE_CUR, move(_request), move(*e), "" };
		}
		return _readyResult
			? HttpTaskResult{ move(*_readyResult), move(_request) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ json{}, move(_request) };
	}
}