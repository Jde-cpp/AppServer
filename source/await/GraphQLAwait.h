#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::App::Server{
	struct GraphQLAwait : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		GraphQLAwait( string&& query, UserPK userPK, SRCE )ι:base{sl}, _query{move(query)}, _userPK{userPK}{}
		α Suspend()ι->void override;
		string _query;
		UserPK _userPK;
	};
}