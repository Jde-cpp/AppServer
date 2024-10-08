#include "CertificateLoginAwait.h"
#include <jde/io/Json.h>
//#include "../../Ssl/source/Ssl.h"
#include <jde/web/client/Jwt.h>

#define var const auto
namespace Jde::App{
	α CertificateLoginAwait::Suspend()ι->void{
		Execute();
	}
	α CertificateLoginAwait::Execute()ι->UM::LoginAwait::Task{
		if( std::abs(time(nullptr)-_jwt.Iat)>60*10 )
			THROW( "Invalid iat.  Expected ~'{}', found '{}'.", time(nullptr), _jwt.Iat );

		Crypto::Verify( _jwt.Modulus, _jwt.Exponent, _jwt.HeaderBodyEncoded, _jwt.Signature );
		try{
			auto userPK = co_await UM::LoginAwait( move(_jwt.Modulus), move(_jwt.Exponent), move(_jwt.UserName), move(_jwt.UserTarget), move(_jwt.Description), {} );
			Resume( move(userPK) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}