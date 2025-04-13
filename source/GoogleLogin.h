#pragma once
#include <jde/framework/coroutine/Await.h>
#include "../../Google/source/TokenInfo.h"
#include <jde/web/client/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>

namespace Jde::App{
//	using namespace Coroutine;
	struct GoogleLoginAwait : TAwait<Google::TokenInfo>{
		using base = TAwait<Google::TokenInfo>;
		GoogleLoginAwait( string&& jwt ):_jwt{ move(jwt) }{}
		α await_ready()ι->bool override{ return !_jwt.Modulus.empty() && !_jwt.Exponent.empty(); }
		α Suspend()ι->void override{ Execute(); }
		α await_resume()ε->Google::TokenInfo;
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};
}