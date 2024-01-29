#pragma once

#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/collections/UnorderedSet.h"

namespace Jde::ApplicationServer
{
	using Collections::UnorderedMap;
	using Collections::UnorderedMapPtr;
	namespace Messages{ struct Application; struct Message; struct RequestStrings; }
	struct ApplicationStrings
	{
		ApplicationStrings(  )
		{}
		α Add( const Messages::Message& message )ι->void;
		α Get( Logging::EFields field, Logging::MessageBase::ID id )ι->sp<string>;

		UnorderedMap<uint32,string> Files;
		UnorderedMap<uint32,string> Functions;
		UnorderedMap<uint32,string> Messages;
		UnorderedMapPtr<uint32,string> ThreadsPtr;
		UnorderedMapPtr<uint32,string> UsersPtr;
	};
}
namespace Jde::ApplicationServer::Cache
{
	α Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value )ι->void;
	α AddSession( uint id, sp<Messages::Application> pApplication )ι->void;
	α AddThread( uint sessionId, uint threadId, sv thread )ι->void;
	α AddMessageStrings( uint sessionId, const Messages::Message& message )ι->void;
	α ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func )ι->uint;
	α FetchStrings( uint sessionId, const Messages::RequestStrings& request )ι->void;
	α Load()ε->ApplicationStrings&;
	α AppStrings()ι->ApplicationStrings&;
	α Messages()ι->const UnorderedSet<uint32>&;
}