/*
#include "GraphQLAwait.h"
#include <jde/ql/QLAwait.h>

namespace Jde::App::Server{

	α GraphQLAwait::Suspend()ι->void{
		[this]( GraphQLAwait& self )->QL::QLAwait::Task {
			try{
				auto j = co_await QL::QLAwait( move(_query), _userPK, _sl );
				self.Resume( move(j) );
			}
			catch( IException& e ){
				self.ResumeExp( move(e) );
			}
		}( *this );
	}
}
*/