#include "GoogleLogin.h"
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
//#include "../../Ssl/source/Ssl.h"

#define let const auto
namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	//using Web::Client::ClientHttpRes;

	α GoogleLoginAwait::Execute()ι->Jde::Task{
		try{
			//let header = Json::Parse( Ssl::Decode64(string{parts[0]}) );//{"alg":"RS256","kid":"fed80fec56db99233d4b4f60fbafdbaeb9186c73","typ":"JWT"}
			//let body = Json::Parse( Ssl::Decode64(string{parts[1]}) );
			Google::TokenInfo token{ _jwt.Body };

			//TODO cache this.
			jobject jOpenidConfiguration;
			[&]()->ClientHttpAwait::Task {
				 jOpenidConfiguration = (co_await ClientHttpAwait{"accounts.google.com", "/.well-known/openid-configuration"}).Json();
			}();
			let uri = Json::AsString( jOpenidConfiguration, "jwks_uri" ); THROW_IF( !uri.starts_with("https://www.googleapis.com"), "Wrong target:  '{}'", uri ); //https://www.googleapis.com/oauth2/v3/certs
			jobject jwks;
			[&]()->ClientHttpAwait::Task {
				 jwks = ( co_await ClientHttpAwait{"www.googleapis.com", uri.substr(sizeof("https://www.googleapis.com")-1)} ).Json();
			}();
			let& kid = _jwt.Kid;
			THROW_IF( kid.empty(), "Could not find kid in header {}", Str::Decode64<string>(_jwt.HeaderBodyEncoded) );
			let& keys = Json::AsArray( jwks, "keys" );
			jvalue foundKey;
			for( let& key : keys ){
				let keyString = Json::AsString( Json::AsObject(key), "kid" );
				if( keyString==kid ){
					foundKey = key;
#ifndef NDEBUG
					try{
						if( auto f = IApplication::ApplicationDataFolder()/(keyString+".json"); !fs::exists(f) )
							(co_await IO::Write( move(f), ms<string>(serialize(foundKey)) )).CheckError();
					}catch( const IOException& )
					{}
#endif
					break;
				}
				//break;
			}
#ifndef NDEBUG
			if( foundKey.is_null() && fs::exists(IApplication::ApplicationDataFolder()/(kid+".json")) )
				foundKey = Json::Parse( IO::FileUtilities::Load(IApplication::ApplicationDataFolder()/(kid+".json")) );
#endif
			THROW_IF( foundKey.is_null(), "Could not find key... '{}' in: '{}'", kid, serialize(keys) );
			let alg = Json::AsString( Json::AsObject(foundKey), "alg" );
			Crypto::Verify( _jwt.Modulus, _jwt.Exponent, _jwt.HeaderBodyEncoded, _jwt.Signature );
			THROW_IF(token.Aud != Settings::FindSV("/GoogleAuthClientId").value_or(""), "Invalid client id");
			THROW_IF(token.Iss != "accounts.google.com" && token.Iss != "https://accounts.google.com", "Invalid iss");
#ifdef NDEBUG
			let expiration = Clock::from_time_t(token.Expiration);
			THROW_IF(expiration < Clock::now(), "token expired");
#endif
			Resume( move(token) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α GoogleLoginAwait::Suspend()ι->void{
		Execute();
	}
}