#pragma once
#include <jde/db/graphQL/GraphQLHook.h>

namespace Jde::App{
	struct AppInstanceHook final : DB::GraphQL::IGraphQLHook{
		//using namespace Jde::DB;
		α Start( sp<DB::MutationQL> mu, UserPK userPK, SRCE )ι->up<DB::GraphQL::IMutationAwait>;
		α Stop( sp<DB::MutationQL> mu, UserPK userPK, SRCE )ι->up<DB::GraphQL::IMutationAwait>;
	};
}