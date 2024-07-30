#pragma once
#include "usings.h"

namespace Jde::DB{ struct IDataSource; struct TableQL; }

namespace Jde::App::Server{
	α ConfigureDatasource()ε->void;
	α SaveString( App::Proto::FromClient::EFields field, uint32 id, string value, SRCE )ι->void;
}
namespace Jde::App{
	α AddInstance( sv applicationName, sv hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK,ELogLevel,ELogLevel>;

	//α LoadApplications( AppPK pk=0 )ι->up<Proto::FromServer::Applications>;
	namespace Data{
		α LoadEntries( DB::TableQL table )ε->Proto::FromServer::Traces;
		α LoadStrings( SRCE )ε->void;
	}
	α SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>* args, SRCE )ι->void;

}