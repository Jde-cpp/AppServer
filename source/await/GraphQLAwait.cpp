#include "GraphQLAwait.h"
#include "../../../../Framework/source/db/GraphQL.h"

namespace Jde::App::Server{

	α GraphQLAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		[&]()->Coroutine::Task {
			try{
				auto j = (co_await DB::CoQuery(move(_query), _userPK, "CoQuery", _sl) ).UP<json>();
				Promise()->Resume( move(*j), h );
			}
			catch( IException& e ){
				Promise()->ResumeWithError( move(e), h );
			}
		}();
	}
}