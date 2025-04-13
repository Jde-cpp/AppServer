#include "GoogleLogin.h"
#include <jde/crypto/OpenSsl.h>
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
//#include "../../Ssl/source/Ssl.h"

#define let const auto
namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	up<jobject> jwks;

	α GoogleLoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			if( !jwks ){
				/*let openidConfiguration = ( co_await ClientHttpAwait{
					"accounts.google.com",
					"/.well-known/openid-configuration",
					443,
					{ContentType:"", Verb:http::verb::get}} ).Json();
				let uri = Json::AsString( openidConfiguration, "jwks_uri" ); THROW_IF( !uri.starts_with("https://www.googleapis.com"), "Wrong target:  '{}'", uri ); //https://www.googleapis.com/oauth2/v3/certs */
				constexpr sv uri = "https://www.googleapis.com/oauth2/v3/certs";
				jwks = mu<jobject>( (co_await ClientHttpAwait{
					"www.googleapis.com",
					string{uri.substr(sizeof("https://www.googleapis.com")-1)},
					443,
					{ContentType:"", Verb:http::verb::get}} ).Json() );
			}
			let& kid = _jwt.Kid;
			THROW_IF( kid.empty(), "Could not find kid in header {}", Str::Decode64<string>(_jwt.HeaderBodyEncoded) );
			let& keys = Json::AsArray( *jwks, "keys" );
			jobject foundKey;
			for( let& key : keys ){
				let keyString = Json::AsString( Json::AsObject(key), "kid" );
				if( keyString==kid )
					foundKey = Json::AsObject(key);
			}
			THROW_IF( foundKey.empty(), "Could not find key... '{}' in: '{}'", kid, serialize(keys) );
			_jwt.SetModulus( Json::AsString( foundKey, "n") );
			_jwt.SetExponent( Json::AsString( foundKey, "e") );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α GoogleLoginAwait::await_resume()ε->Google::TokenInfo{
		AwaitResume();
		THROW_IF( _jwt.Aud() != Settings::FindSV("/GoogleAuthClientId").value_or(""), "Invalid client id: '{}'", _jwt.Aud() );
		THROW_IF( _jwt.Iss() != "https://accounts.google.com", "Invalid iss: '{}'", _jwt.Iss() );
#ifdef NDEBUG
		let expiration = Clock::from_time_t(token.Expiration);
		THROW_IF(expiration < Clock::now(), "token expired");
#endif
		try{
			Crypto::Verify( _jwt.Modulus, _jwt.Exponent, _jwt.HeaderBodyEncoded, _jwt.Signature );
		}
		catch( IException& e ){
			THROW( "Verify failed. {}\n{}", e.what(), _jwt.Payload() );
		}
		return { _jwt.Body };
	}
}