#include "CustomRequestAwait.h"

namespace Jde::App{
	struct CustomRequestValues final{
		RequestId SessionRequestId;
		base::Handle Handle;
	};
	concurrent_flat_map<AppInstancePK,RequestIdMap> _customRequests;
	CustomRequestAwait::CustomRequestAwait( UserPK userPK, Proto::FromClient::Message&& customRequest, RequestId sessionRequestId, sp<IServerSocketSession> serverSocketSession, SRCE )ι:
		base{sl},
		_userPK{userPK},
		_sessionRequestId{sessionRequestId},
		_request{ move(req) },
		_serverSocketSession{serverSocketSession}
	{}

	α CustomRequestAwait::await_suspend( base::Handle h )ε->void{
		base::await_suspend( h );
		_customRequests.emplace( _customRequest.InstanceId, {_customRequest.RequestId, _sessionRequestId, h} );
		try{
			Write( _customRequest.app_pk(), _customRequest.instance_pk(), FromServer::ExecuteTransmission(sessionRequestId, _userPK, *_customRequest.mutable_message()) );
		}
		catch( IException& e ){
			SetError( e.Move() );
			h.resume();
		}
	}

	α CustomRequestAwait::OnCloseConnection( AppInstancePK instancePK )ι->void{
		_customRequests.erase_if( instancePK ){
			auto h = it->second.Handle;
			h.promise().SetError( Exception{"Connection closed."} );
			it->second.Handle.resume();
			return true;
		}
	}
}