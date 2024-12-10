#pragma once
#include <jde/ql/QLHook.h>

namespace Jde::App{
	//namespace Jde::QL{ struct MutationQL; }
	struct AppInstanceHook final : QL::IQLHook{
		//using namespace Jde::DB;
		α Start( const QL::MutationQL& mu, UserPK userPK, SRCE )ι->HookResult;
		α Stop( const QL::MutationQL& mu, UserPK userPK, SRCE )ι->HookResult;
	};
}