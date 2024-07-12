#pragma once
//#include "../../Framework/source/collections/UnorderedSet.h"

namespace Jde::DB{ struct IDataSource; }

namespace Jde::App{
	α SetDataSource( sp<DB::IDataSource> dataSource )ε->void;
	α AddInstance( sv applicationName, sv hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK,ELogLevel,ELogLevel>;
	α SaveString( AppPK applicationId, Proto::FromClient::EFields field, uint32 id, string value, SRCE )ι->void;

	α LoadApplications( AppPK pk=0 )ι->up<Proto::FromServer::Applications>;
	α LoadEntries( AppPK applicationId, AppInstancePK instanceId, ELogLevel level, const std::optional<TimePoint>& start, uint limit )ι->up<Proto::FromServer::Traces>;
	α LoadFiles( SRCE )ε->concurrent_flat_map<uint32,string>;
	α LoadFunctions( SRCE )ε->concurrent_flat_map<uint32,string>;
	α LoadMessages( SRCE )ε->concurrent_flat_map<uint32,string>;
	α LoadMessageIds( SRCE )ε->concurrent_flat_set<uint32>;

	//α SaveMessage( AppPK applicationId, AppInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>* args, SRCE )ι->void;
	α SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>* args, SRCE )ι->void;

}