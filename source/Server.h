#pragma once
#include "HttpRequestAwait.h"
#include <jde/web/flex/Flex.h>

namespace Jde::App{
	using namespace Jde::Web::Flex;

	α AppId()ι->AppPK;
	α SetAppId( AppPK x )ι->void;
	α InstanceId()ι->AppInstancePK;
	α SetInstanceId( AppInstancePK x )ι->void;

	α StartWebServer()ι->void;
	α StopWebServer()ι->void;
	α RemoveSession( AppInstancePK sessionPK )ι->void;
	α FindApplications( str name )ι->vector<Proto::FromClient::Instance>;
	α BroadcastLogEntry( LogPK id, AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>& args )ι->ELogLevel;
	α BroadcastStatus( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& status )ι->void;
	namespace Server{
		α Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε;
	}

	struct RequestHandler final : IRequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->void override;
	};
}
