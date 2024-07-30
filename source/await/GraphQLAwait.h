#pragma once
#include <jde/coroutine/Await.h>

namespace Jde::App::Server{
	struct GraphQLAwait : TAwait<json>{
		using base = TAwait<json>;
		GraphQLAwait( string&& query, UserPK userPK, SRCE )ι:base{sl}, _query{move(query)}, _userPK{userPK}{}
		α await_suspend( base::Handle h )ι->void override;
		string _query;
		UserPK _userPK;
	};
}