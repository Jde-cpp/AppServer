#include <jde/coroutine/Await.h>
#include "../../Google/source/TokenInfo.h"
#include <jde/web/client/Jwt.h>

namespace Jde::App{
//	using namespace Coroutine;
	struct GoogleLoginAwait : TAwait<Google::TokenInfo>{
		using base = TAwait<Google::TokenInfo>;
		GoogleLoginAwait( string&& jwt ):_jwt{ move(jwt) }{}
		α await_suspend( base::Handle h )ι->void override;
	private:
		α Execute()ι->Jde::Task;
		Web::Jwt _jwt;
	};
}