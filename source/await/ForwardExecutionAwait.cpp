#include "ForwardExecutionAwait.h"
#include <jde/app/shared/proto/App.FromServer.h>
#include "../ServerSocketSession.h"
#include "../Server.h"
#define var const auto

namespace Jde::App{
	struct SessionHandle final{
		sp<ServerSocketSession> RequestSocketSession;
		ForwardExecutionAwait::Handle Handle;
	};
	concurrent_flat_map<RequestId,SessionHandle> _forwardExecutionMessages;
	ForwardExecutionAwait::ForwardExecutionAwait( UserPK userPK, Proto::FromClient::ForwardExecution&& customRequest, sp<ServerSocketSession> serverSocketSession, SL sl )ι:
		base{sl},
		_userPK{userPK},
		_requestSocketSession{serverSocketSession}
	{}

	α ForwardExecutionAwait::await_suspend( base::Handle h )ε->void{
		base::await_suspend( h );
		var serverRequestId = Server::NextRequestId();
		_forwardExecutionMessages.emplace( serverRequestId, SessionHandle{move(_requestSocketSession), h} );
		try{
			Server::Write( _forwardExecutionMessage.app_pk(), _forwardExecutionMessage.app_instance_pk(), FromServer::ExecuteRequest(serverRequestId, _userPK, move(*_forwardExecutionMessage.mutable_execution_transmission())) );
		}
		catch( IException& e ){//Could not find app instance.
			h.promise().ResumeWithError( move(e), h );
		}
	}

	α ForwardExecutionAwait::OnCloseConnection( AppInstancePK instancePK )ι->void{
		_forwardExecutionMessages.erase_if( [=](auto&& kv){
			if( kv.second.RequestSocketSession->InstancePK() != instancePK )
				return false;
			auto h = kv.second.Handle;
			h.promise().ResumeWithError( Exception{"Connection closed."}, h );
			return true;
		} );
	}
	α ForwardExecutionAwait::Resume( string&& results, RequestId serverRequestId )ι->bool{
		return _forwardExecutionMessages.erase_if( serverRequestId, [&](auto&& kv){
			auto h = kv.second.Handle;
			h.promise().Resume( move(results), h );
			return true;
		});
	}
}