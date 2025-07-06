#pragma once
#include "HttpRequestAwait.h"
#include <jde/web/server/Server.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/IRequestHandler.h>

namespace Jde{
	namespace DB{ struct AppSchema; }
	namespace QL{ struct LocalQL; }
	namespace Web::Server{ struct RestStream; }

namespace App{
	using namespace Jde::Web::Server;
	struct ServerSocketSession;
	namespace Server{
		α GetAppPK()ι->AppPK;
		α SetAppPK( AppPK x )ι->void;
		α QLPtr()ι->sp<QL::LocalQL>;
		α QL()ι->QL::LocalQL&;
		α SetLocalQL( sp<QL::LocalQL> ql )ι->void;
		α Schemas()ι->const vector<sp<DB::AppSchema>>&;


		α StartWebServer( jobject&& settings )ε->void;
		α StopWebServer()ι->void;

		α BroadcastLogEntry( LogPK id, AppPK logAppPK, AppInstancePK logInstancePK, const Logging::ExternalMessage& m, const vector<string>& args )ι->void;
		α BroadcastStatus( AppPK appId, AppInstancePK statusInstancePK, str hostName, Proto::FromClient::Status&& status )ι->void;
		α BroadcastAppStatus()ι->void;
		α FindApplications( str name )ι->vector<Proto::FromClient::Instance>;
		α FindInstance( AppInstancePK instancePK )ι->sp<ServerSocketSession>;
		α NextRequestId()->RequestId;
		α RemoveSession( AppInstancePK sessionPK )ι->void;
		α SubscribeLogs( string&& qlText, sp<ServerSocketSession> session )ε->void;
		α SubscribeStatus( ServerSocketSession& session )ι->void;

		α UnsubscribeLogs( AppInstancePK instancePK )ι->bool;
		α UnsubscribeStatus( AppInstancePK instancePK )ι->bool;
		α Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε->void;
	}

	struct RequestHandler final : IRequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override{ return Server::Schemas(); }
	};
}}