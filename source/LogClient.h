#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>

namespace Jde::Logging
{
	struct LogClient : public IServerSink//, IShutdown
	{
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )ε;
		α Close()ι->void override{};
		α ApplicationId()ι->ApplicationPK override{return _applicationId;}
		Ω CreateInstance()ε->void;
		α FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait override{CRITICAL("calling GetSessionInfo from server."); return SessionInfoAwait{sessionId}; }
		α InstanceId()ι->ApplicationInstancePK override{return _instanceId;}
		α IsLocal()ι->bool override{ return true; }
		α Log( Messages::ServerMessage& message )ι->void override;
		α Log( const MessageBase& messageBase )ι->void override;
		α Log( const MessageBase& messageBase, vector<string>& values )ι->void override;

		α Write( Logging::Proto::ToServer&& m )ι->void override{ CRITICAL("Tried to write on local LogClient"); }
		α WebSubscribe( ELogLevel level )ι->void override{ _webLevel = level; }

		const ApplicationPK _applicationId;
	private:
		ELogLevel _webLevel{ELogLevel::None};
	};
}