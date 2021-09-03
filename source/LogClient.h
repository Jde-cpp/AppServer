#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>
#include "../../Framework/source/log/server/ServerSink.h"

namespace Jde::Logging
{
	struct LogClient : public Logging::IServerSink//, IShutdown
	{
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false);
		static void CreateInstance()noexcept(false);
		static LogClient& Instance()noexcept{ return (LogClient&)*_pServerSink; }
		void Log( Logging::Messages::Message&& message )noexcept override;
		void Log( Logging::MessageBase&& messageBase )noexcept override;
		void Log( Logging::MessageBase messageBase, vector<string> values )noexcept override;

		void WebSubscribe( ELogLevel level )noexcept{ _webLevel = level; }//(ELogLevel)std::min((uint)level, (uint)_webLevel);}

		const ApplicationInstancePK InstanceId;
		const ApplicationPK ApplicationId;
	private:
		ELogLevel _webLevel{ELogLevel::None};
	};
}