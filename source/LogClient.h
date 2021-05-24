#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>
#include "../../Framework/source/log/server/ServerSink.h"

namespace Jde::Logging
{
	struct LogClient : public Logging::IServerSink, IShutdown
	{
		static void CreateInstance()noexcept(false);
		static LogClient& Instance()noexcept{ return *_pInstance; }
		void Log( const Logging::Messages::Message& message )noexcept override;
		void Log( const Logging::MessageBase& messageBase )noexcept override;
		void Log( const Logging::MessageBase& messageBase, const vector<string>& values )noexcept override;

//		void SetWebLevel( ELogLevel level )noexcept{_webLevel = level;}
		void WebSubscribe( ELogLevel level )noexcept{ _webLevel = level; }//(ELogLevel)std::min((uint)level, (uint)_webLevel);}
		void Shutdown()noexcept override{ _pInstance = nullptr; }

		const ApplicationInstancePK InstanceId;
		const ApplicationPK ApplicationId;
	private:
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false);
		static sp<LogClient> _pInstance;
		ELogLevel _webLevel{ELogLevel::None};
	};
}