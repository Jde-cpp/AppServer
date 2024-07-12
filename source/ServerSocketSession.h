#pragma once
#include <jde/http/usings.h>
#include <jde/web/flex/IWebsocketSession.h>
#include <jde/appClient/Sessions.h>

namespace Jde::App{
	using namespace Jde::Web::Flex;
	using namespace Jde::Http;
	struct ServerSocketSession : TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>{
		using base = TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>;
		ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α OnRead( Proto::FromClient::Transmission&& transmission )ι->void override;
		α InstancePK()Ι->AppInstancePK{ return _instancePK; }
		α AppPK()Ι->AppPK{ return _appPK; }
		α Instance()Ι->const Proto::FromClient::Instance&{ return _instance; }
	private:
		α WriteException( const IException& e )ι->void override;
		α OnConnect( SessionPK sessionId, RequestId requestId )ι->Web::UpsertAwait::Task;
		α OnClose()ι->void;
		Proto::FromClient::Instance _instance;
		Jde::AppPK _appPK{};
		AppInstancePK _instancePK{};
		ELogLevel _webLevel{ ELogLevel::NoLog };
		ELogLevel _dbLevel{ ELogLevel::NoLog };
		uint _requestId{0};
	};
}