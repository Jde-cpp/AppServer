#pragma once
#include "TypeDefs.h"
#include <jde/Assert.h>
#include "../../Framework/source/io/ServerSink.h"

namespace Jde::Logging{
	struct LogClient : public IServerSink{
		LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )ε;
		α Close()ι->void override{};
		α ApplicationId()ι->ApplicationPK override{return _applicationId;}
		Ω CreateInstance()ε->void;
		α GraphQL( string query, UserPK userPK, HCoroutine h, SL sl )ι->void override{ GraphQLTask( move(query), userPK, h, sl ); }
		α FetchSessionInfo( SessionPK sessionId )ι->SessionInfoAwait override{CRITICALT(AppTag(), "calling GetSessionInfo from server."); return SessionInfoAwait{sessionId}; }
		α InstanceId()ι->ApplicationInstancePK override{return _instanceId;}
		α IsLocal()ι->bool override{ return true; }
		α Log( Messages::ServerMessage& message )ι->void override;
		α Log( const MessageBase& messageBase )ι->void override;
		α Log( const MessageBase& messageBase, vector<string>& values )ι->void override;

		α Write( Logging::Proto::ToServer&& )ι->void override{ CRITICALT(AppTag(), "Tried to write on local LogClient"); }
		α WebSubscribe( ELogLevel level )ι->void override{ _webLevel = level; }

		const ApplicationPK _applicationId;
	private:
		Ω GraphQLTask( string query, UserPK userPK, HCoroutine h, SL sl )ι->Task;
		ELogLevel _webLevel{ELogLevel::Critical};
	};
}