#pragma once
#include "../../Public/src/web/ProtoServer.h"
#include "../../Public/src/web/RestServer.h"

namespace Jde::ApplicationServer
{
	using namespace Jde::Web::Rest;
	struct RestSession : ISession, std::enable_shared_from_this<RestSession>
	{
		RestSession( tcp::socket&& socket ): ISession{move(socket)}{}
		virtual ~RestSession(){}
		α HandleRequest( string&& target, flat_map<string,string>&& params, Request&& req )ι->void override;
		α MakeShared()ι->sp<ISession> override{ return shared_from_this(); }

		α SendQuery( sv query, http::request<http::string_body>&& req, sp<ISession> s )ι->Task;
		α GoogleLogin( Request req )ι->Task;

		α GetUserId( SessionPK sessionId )Ι->UserPK;
	};
	α Listener()ι->TListener<RestSession>&;
}