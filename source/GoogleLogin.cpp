#include "GoogleLogin.h"

#define var const auto
namespace Jde::GoogleLogin
{
	α Process( string credential, variant<up<Google::TokenInfo>,up<IException>>& y, HCoroutine h )ι->Task
	{
		//constexpr EAuthType type{ EAuthType::Google };
		var parts = Str::Split( move(credential), '.');
		Google::TokenInfo token;
		//Web::FromServer::MessageUnion y;
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
					try{
						if( auto f = IApplication::ApplicationDataFolder()/(keyString+".json"); !fs::exists(f) )
							(co_await IO::Write( move(f), ms<string>(foundKey.dump()) )).CheckError();
					}catch( const IOException& )
					{}
#endif
					break;
				}
				//break;
			}
#ifndef NDEBUG
			if( foundKey.is_null() && fs::exists(IApplication::ApplicationDataFolder()/(kidString+".json")) )
				foundKey = json::parse( IO::FileUtilities::Load(IApplication::ApplicationDataFolder()/(kidString+".json")) );
#endif
			THROW_IF( foundKey.is_null(), "Could not find key... '{}' in: '{}'", pKid->get<string>(), pKeys->dump() );
			var alg = foundKey["alg"].get<string>();
			var exponent = foundKey["e"].get<string>();
			var modulus = foundKey["n"].get<string>();
			//var token = Ssl::Get<Google::TokenInfo>( "oauth2.googleapis.com", format("/tokeninfo?id_token={}"sv, credential) );
			Ssl::Verify( Ssl::Decode64<vector<unsigned char>>(modulus, true), Ssl::Decode64<vector<unsigned char>>(exponent, true), string{parts[0]}+'.'+string{parts[1]}, Ssl::Decode64(string{parts[2]}, true) );
			THROW_IF(token.Aud != Settings::Get<string>("GoogleAuthClientId"), "Invalid client id");
			THROW_IF(token.Iss != "accounts.google.com" && token.Iss != "https://accounts.google.com", "Invalid iss");
#ifdef NDEBUG
		// TODO uncomment
		//	var expiration = Clock::from_time_t(token.Expiration);
		//	THROW_IF(expiration < Clock::now(), "token expired");
#endif
			y = mu<Google::TokenInfo>( move(token) );
		}
		catch( const nlohmann::json::exception& e ){
			CRITICALT( AppTag(), "json exception - {}", e.what() );
			y = mu<Exception>( "Authentication Failed" );
		}
		catch( Exception& e )
		{
			y = e.Move();
		}
		h.resume();
		co_return;
	}
	α GoogleLoginAwait::await_suspend( HCoroutine h )ι->void
	{
		IAwait::await_suspend(h);
		Process( move(_token), _result, h );
	}
	α GoogleLoginAwait::await_resume()ι->AwaitResult
	{
		return _result.index()==0 ? AwaitResult{ move(get<0>(_result)) } : AwaitResult{ move(get<1>(_result)) };
	}
}