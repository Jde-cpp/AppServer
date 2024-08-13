#include "GraphQLAwait.h"
#include "../../../../Framework/source/db/GraphQL.h"

namespace Jde::App::Server{

	α GraphQLAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		[this]( GraphQLAwait& self )->Coroutine::Task {
			try{
				auto j = (co_await DB::CoQuery(move(_query), _userPK, "CoQuery", _sl) ).UP<json>();
				self.Resume( move(*j) );
			}
			catch( IException& e ){
				self.ResumeExp( move(e) );
			}
		}( *this );
	}
}