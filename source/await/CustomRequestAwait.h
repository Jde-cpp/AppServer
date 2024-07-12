#pragma once

namespace Jde::App{
	struct CustomRequestAwait final: TAwait<string>{//kCustomResponse
		using base = TAwait<string>;
		CustomRequestAwait( UserPK userPK, Proto::FromClient::Message&& customRequest, RequestId sessionRequestId, sp<IServerSocketSession> serverSocketSession, SRCE )ι;
		α await_suspend( base::Handle h )ε->void;
		Ω OnCloseConnection( AppInstancePK instancePK )ι->void;

	private:
		UserPK _userPK;
		RequestId _sessionRequestId;
		Proto::FromClient::Message _customRequest;
		sp<IServerSocketSession> _serverSocketSession;
	};
}