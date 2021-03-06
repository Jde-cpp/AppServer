#pragma once
#include "../../Framework/source/collections/UnorderedMap.h"
#include "TypeDefs.h"
#include "../../Framework/source/log/server/proto/messages.pb.h"


namespace Jde::ApplicationServer::Web::FromServer{ class Traces; class Applications;}
namespace Jde::DB{ struct IDataSource; }

namespace Jde::Logging::Data
{
	void SetDataSource( sp<DB::IDataSource> dataSource )noexcept(false);
	std::tuple<ApplicationPK, ApplicationInstancePK,ELogLevel,ELogLevel> AddInstance( sv applicationName, sv hostName, uint processId )noexcept(false);
	void SaveString( ApplicationPK applicationId, Proto::EFields field, uint32 id, sp<string> pValue )noexcept;

	Jde::ApplicationServer::Web::FromServer::Applications* LoadAllocatedApplications( ApplicationPK pk=0 )noexcept;
	unique_ptr<ApplicationServer::Web::FromServer::Traces> LoadEntries( ApplicationPK applicationId, ApplicationInstancePK instanceId, ELogLevel level, const std::optional<TimePoint>& start, uint limit )noexcept;
	Collections::UnorderedMapPtr<uint32,string> LoadFiles( ApplicationPK applicationId )noexcept(false);
	Collections::UnorderedMapPtr<uint32,string> LoadFunctions( ApplicationPK applicationId )noexcept(false);
	Collections::UnorderedMapPtr<uint32,string> LoadMessages( ApplicationPK applicationId )noexcept(false);

	void PushMessage( ApplicationPK applicationId, ApplicationInstancePK instanceId, TimePoint time, ELogLevel level, uint32 messageId, uint32 fileId, uint32 functionId, uint16 lineNumber, uint32 userId, uint threadId, const vector<string>& variables )noexcept;
}