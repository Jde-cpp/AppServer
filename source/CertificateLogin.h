#pragma once
#include "../../Framework/source/um/UM.h"
#include <jde/crypto/OpenSsl.h>
#include <jde/web/client/Jwt.h>

namespace Jde::App{
	struct CertificateLoginAwait : TAwait<UserPK>{
		using base = TAwait<UserPK>;
		CertificateLoginAwait( string&& jwtString, string endpoint, SRCE )ι:base{sl},_jwt{ move(jwtString) }, _endpoint{endpoint}{}
		α await_suspend( base::Handle h )ε->void override;
		α Execute()ι->UM::LoginAwait::Task;
	private:
		Web::Jwt _jwt; string _endpoint;
	};
}