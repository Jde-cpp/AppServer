#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/access/types/GoogleTokenInfo.h>
#include <jde/web/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>

namespace Jde::App{
//	using namespace Coroutine;
	struct GoogleLoginAwait : TAwait<Google::TokenInfo>{
		using base = TAwait<Google::TokenInfo>;
		GoogleLoginAwait( string&& jwt ):_jwt{ move(jwt) }{}
		α await_ready()ι->bool override{ return !_jwt.PublicKey.Modulus.empty() && !_jwt.PublicKey.Exponent.empty(); }
		α Suspend()ι->void override{ Execute(); }
		α await_resume()ε->Google::TokenInfo;
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};
}