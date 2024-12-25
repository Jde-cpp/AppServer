#pragma once
#include "usings.h"
#include <jde/access/access.h>
#include <jde/framework/coroutine/Await.h>

namespace Jde::DB{ struct IDataSource; }
namespace Jde::QL{ struct TableQL; }

namespace Jde::App::Server{
	struct ConfigureDSAwait : VoidAwait<>{
	//α ConfigureDatasource()ε->void;
		α Suspend()ι->void override;
		α Configure()ι->Access::ConfigureAwait::Task;
	};
	α SaveString( App::Proto::FromClient::EFields field, uint32 id, string value, SRCE )ι->void;
}
namespace Jde::App{
	α AddInstance( str applicationName, str hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK>;

	//α LoadApplications( AppPK pk=0 )ι->up<Proto::FromServer::Applications>;
	namespace Data{
		α LoadEntries( QL::TableQL table )ε->Proto::FromServer::Traces;
		α LoadStrings( SRCE )ε->void;
	}
	α SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>* args, SRCE )ι->void;

}