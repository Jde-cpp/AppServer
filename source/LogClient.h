#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>
#include "../../Framework/source/log/server/ServerSink.h"

namespace Jde::Logging
{
	struct LogClient : public Logging::IServerSink//, IShutdown
	{
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false);
		Ω CreateInstance()noexcept(false)->void;
		Ω Instance()noexcept->LogClient&{ return (LogClient&)*Logging::Server(); }
		α Log( Logging::Messages::Message& message )noexcept->void override;
		α Log( const Logging::MessageBase& messageBase )noexcept->void override;
		α Log( const Logging::MessageBase& messageBase, vector<string>& values )noexcept->void override;

		α WebSubscribe( ELogLevel level )noexcept{ _webLevel = level; }//(ELogLevel)std::min((uint)level, (uint)_webLevel);}

		const ApplicationInstancePK InstanceId;
		const ApplicationPK ApplicationId;
	private:
		ELogLevel _webLevel{ELogLevel::None};
	};
}