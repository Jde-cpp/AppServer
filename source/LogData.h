#pragma once
#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include "TypeDefs.h"
#include "../../Framework/source/log/server/proto/messages.pb.h"


namespace Jde::ApplicationServer::Web::FromServer{ class Traces; class Applications;}
namespace Jde::DB{ struct IDataSource; }

namespace Jde::Logging::Data
{
	α SetDataSource( sp<DB::IDataSource> dataSource )noexcept(false)->void;
	α AddInstance( sv applicationName, sv hostName, uint processId )noexcept(false)->std::tuple<ApplicationPK, ApplicationInstancePK,ELogLevel,ELogLevel>;
	α SaveString( ApplicationPK applicationId, Proto::EFields field, uint32 id, sp<string> pValue )noexcept->void;

	α LoadApplications( ApplicationPK pk=0 )noexcept->up<ApplicationServer::Web::FromServer::Applications>;
	α LoadEntries( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, const std::optional<TimePoint>& start, uint limit )noexcept->unique_ptr<ApplicationServer::Web::FromServer::Traces>;
	α LoadFiles( ApplicationPK applicationId )noexcept(false)->Collections::UnorderedMap<uint32,string>;
	α LoadFunctions( ApplicationPK applicationId )noexcept(false)->Collections::UnorderedMap<uint32,string>;
	α LoadMessages( ApplicationPK applicationId )noexcept(false)->Collections::UnorderedMap<uint32,string>;
	α LoadMessages()noexcept(false)->unordered_set<uint32> ;

	α PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, vector<string>&& variables )noexcept->void;
}