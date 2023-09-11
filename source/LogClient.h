#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>

namespace Jde::Logging
{
	struct LogClient : public IServerSink//, IShutdown
	{
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )ε;
		α IsLocal()ι->bool override{ return true; }
		Ω CreateInstance()ε->void;
		Ω Instance()ι->LogClient&{ return (LogClient&)*Server(); }
		α Log( Messages::ServerMessage& message )ι->void override;
		α Log( const MessageBase& messageBase )ι->void override;
		α Log( const MessageBase& messageBase, vector<string>& values )ι->void override;
		α FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait override{CRITICAL("calling GetSessionInfo from server."); return SessionInfoAwait{sessionId}; }
		α WebSubscribe( ELogLevel level )ι{ _webLevel = level; }//(ELogLevel)std::min((uint)level, (uint)_webLevel);}

		const ApplicationInstancePK InstanceId;
		const ApplicationPK ApplicationId;
	private:
		ELogLevel _webLevel{ELogLevel::None};
	};
}