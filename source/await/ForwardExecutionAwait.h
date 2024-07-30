#pragma once
#include <jde/coroutine/Await.h>

namespace Jde::App{
	struct ServerSocketSession;
	struct ForwardExecutionAwait final: TAwait<string>{//kCustomResponse
		using base = TAwait<string>;
		ForwardExecutionAwait( UserPK userPK, Proto::FromClient::ForwardExecution&& customRequest, sp<ServerSocketSession> serverSocketSession, SRCE )ι;
		α await_suspend( base::Handle h )ε->void;
		Ω OnCloseConnection( AppInstancePK instancePK )ι->void;
		Ω Resume( string&& results, RequestId serverRequestId )ι->bool;

	private:
		UserPK _userPK;
		Proto::FromClient::ForwardExecution _forwardExecutionMessage;
		sp<ServerSocketSession> _requestSocketSession;
	};
}