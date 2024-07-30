#include "AppInstanceHook.h"
#include <jde/db/graphQL/TableQL.h>
#include "Server.h"
#include "ServerSocketSession.h"

#define var const auto

namespace Jde::App{
	struct StartAwait : DB::GraphQL::IMutationAwait{
		StartAwait( sp<DB::MutationQL> mutation, UserPK userPK, SRCE )ι:
			DB::GraphQL::IMutationAwait{ mutation, userPK, sl }
		{}
		α await_ready()ι->bool override{ return true; }
		[[noreturn]] α await_resume()ε->uint override{
			throw Exception{ _sl, Jde::ELogLevel::Critical, "Start instance not implemented" };
		}
	};

	α AppInstanceHook::Start( sp<DB::MutationQL> mutation, UserPK userPK, SL sl )ι->up<DB::GraphQL::IMutationAwait>{
		if( mutation->JsonName!="applicationInstance" )
			return {};
		return mu<StartAwait>( mutation, userPK );
	}

	struct StopAwait : DB::GraphQL::IMutationAwait{
		StopAwait( sp<DB::MutationQL> mutation, UserPK userPK, SRCE )ι:
			DB::GraphQL::IMutationAwait{ mutation, userPK, sl }
		{}
		α await_suspend( DB::GraphQL::IMutationAwait::Handle h )ι->void override{
			base::await_suspend( h );
			var id = _mutation->Id<AppInstancePK>( ELogTags::SocketServerRead );
			auto pid = id==InstancePK() ? OSApp::ProcessId() : 0;
			if( auto p = pid ? sp<ServerSocketSession>{} : Server::FindInstance( id ); p )
				pid = p->Instance().pid();
			if( pid ){
				if( !IApplication::Kill(pid) )
					Promise()->Resume( 0, h );
				else
					Promise()->ResumeWithError( Exception{ELogTags::SocketServerRead, _sl, "Failed to kill process."}, h );
			}
			else
				Promise()->ResumeWithError( Exception{ELogTags::SocketServerRead, _sl, "Instance not found."}, h );
		}
	};

	α AppInstanceHook::Stop( sp<DB::MutationQL> mutation, UserPK userPK, SL sl )ι->up<DB::GraphQL::IMutationAwait>{
		if( mutation->JsonName!="applicationInstance" )
			return {};
		return mu<StopAwait>( mutation, userPK );
	}
}