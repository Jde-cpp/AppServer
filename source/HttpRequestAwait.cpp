#include "HttpRequestAwait.h"
#include <jde/framework/thread/execution.h>
#include <jde/access/access.h>
#include "awaits/CertificateLoginAwait.h"
#include "GoogleLogin.h"
#include "../../Google/source/TokenInfo.h"
#include "WebServer.h"
#include "types/rest/json.h"
#define let const auto

namespace Jde::App{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α ValueJson( string&& value )ι{ return Json::Parse( Ƒ("{{\"value\": \"{}\"}}", value) ); }

	α CertificateLogin( HttpRequest req, HttpRequestAwait::Handle h )ι->CertificateLoginAwait::Task{
		try{
			req.LogRead();
			let token = Json::AsString( req.Body(), "jwt" );
			req.SessionInfo->UserPK = co_await CertificateLoginAwait( token, req.UserEndpoint.address().to_string() );
			jobject j{ {"expiration", ToIsoString(req.SessionInfo->Expiration)} };
			req.SessionInfo->IsInitialRequest = true;  //expecting sessionId to be set.
			h.promise().Resume( {move(j), move(req)}, h );
		}
		catch( IException& e ){
			h.promise().ResumeExp( move(e), h );
		}
	}

	Ω googleLogin( HttpRequest req, HttpRequestAwait::Handle h )ι->GoogleLoginAwait::Task{
		try{
			req.LogRead();
			let token = co_await GoogleLoginAwait{ Json::AsString(req.Body(), "value") };
			[]( str email, HttpRequest req, HttpRequestAwait::Handle h )->Access::AuthenticateAwait::Task {
				try{
					req.SessionInfo->UserPK = co_await Access::Authenticate( email, underlying(Access::EProviderType::Google) );
					h.promise().SetValue( {ValueJson(Ƒ("{:x}", req.SessionInfo->SessionId)), move(req)} );
				}
				catch( exception& e ){
					h.promise().SetExp( move(e) );
				}
				h.resume();
			}( token.Email, move(req), h );
		}
		catch( exception& e ){
			h.promise().SetExp( move(e) );
			h.resume();
		}
	}
	α Logout( HttpRequest&& req, HttpRequestAwait::Handle h )ι->void{
		try{
			req.LogRead();
			jobject j{ {"removed", Sessions::Remove(req.SessionInfo->SessionId)} };
			h.promise().SetValue( {move(j), move(req)} );
		}
		catch( IException& e ){
			h.promise().SetExp( move(e) );
		}
		h.resume();
	}

	α HttpRequestAwait::await_ready()ι->bool{
		if( _request.Method() == http::verb::get ){
			if( _request.Target()=="/GoogleAuthClientId" ){
				_request.LogRead();
				_readyResult = mu<jobject>( ValueJson(Settings::FindString("GoogleAuthClientId").value_or("GoogleAuthClientId Not Configured.")) );
			}
			else if( _request.Target()=="/IotWebSocket" ){
				_request.LogRead();
				let apps = Server::FindApplications( "Jde.IotWebSocket" );
				jarray japps;
				for( auto& app : apps )
					japps.push_back( ToJson(app) );
				jobject y{ {"servers", japps} };
				_readyResult = mu<jobject>( move(y) );
			}
		}
		return _readyResult!=nullptr;
	}
	α HttpRequestAwait::Suspend()ι->void{
		up<IException> pException;
		bool processed{ _request.Method() == http::verb::post };
		if( _request.Method() == http::verb::post ){
			if( _request.Target()=="/loginGoogle" )
				googleLogin( move(_request), _h );
			else if( _request.Target()=="/loginCertificate" )
				CertificateLogin( move(_request), _h );
			else if( _request.Target()=="/logout" )
				Logout( move(_request), _h );
			else
				processed = false;
		}
		if( !processed ){
			_request.LogRead();
			RestException<http::status::not_found> e{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
			ResumeExp( RestException<http::status::not_found>(move(e)) );
		}
	}

	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		if( auto e = Promise() ? Promise()->MoveExp() : nullptr; e ){
			auto pRest = dynamic_cast<IRestException*>( e.get() );
			if( pRest )
				pRest->Throw();
			else
				throw RestException<http::status::internal_server_error>{ move(*e), move(_request) };
		}
		return _readyResult
			? HttpTaskResult{ move(*_readyResult), move(_request) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ {}, move(_request) };
	}
}