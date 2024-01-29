#pragma once
#include "../../Framework/source/collections/UnorderedSet.h"
#include "TypeDefs.h"


namespace Jde::ApplicationServer::Web::FromServer{ class Traces; class Applications;}
namespace Jde::DB{ struct IDataSource; }

namespace Jde::Logging::Data
{
	α SetDataSource( sp<DB::IDataSource> dataSource )ε->void;
	α AddInstance( sv applicationName, sv hostName, uint processId )ε->std::tuple<ApplicationPK, ApplicationInstancePK,ELogLevel,ELogLevel>;
	α SaveString( ApplicationPK applicationId, Proto::EFields field, uint32 id, sp<string> pValue, SRCE )ι->void;

	α LoadApplications( ApplicationPK pk=0 )ι->up<ApplicationServer::Web::FromServer::Applications>;
	α LoadEntries( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, const std::optional<TimePoint>& start, uint limit )ι->up<ApplicationServer::Web::FromServer::Traces>;
	α LoadFiles( SRCE )ε->Collections::UnorderedMap<uint32,string>;
	α LoadFunctions( SRCE )ε->Collections::UnorderedMap<uint32,string>;
	α LoadMessages( SRCE )ε->Collections::UnorderedMap<uint32,string>;
	α LoadMessageIds( SRCE )ε->unordered_set<uint32>;

	α PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables, SRCE )ι->void;
}