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
		ApplicationStrings( ApplicationPK id ):Id{id}
		{}
		ApplicationPK Id;
		UnorderedMap<uint32,string> Files;
		UnorderedMap<uint32,string> Functions;
		UnorderedMap<uint32,string> Messages;
		UnorderedMapPtr<uint32,string> ThreadsPtr;
		UnorderedMapPtr<uint32,string> UsersPtr;
		α Add( const Messages::Message& message )noexcept->void;
		α Get( Logging::EFields field, uint id )noexcept->shared_ptr<string>;
	};

	namespace Cache
	{
		α Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value )->void;
		α AddSession( uint id, shared_ptr<Messages::Application> pApplication )->void;
		α AddThread( uint sessionId, uint threadId, sv thread )->void;
		α AddMessageStrings( uint sessionId, const Messages::Message& message )->void;
		α ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func )->uint;
		α FetchStrings( uint sessionId, const Messages::RequestStrings& request )->void;
		α Load( ApplicationPK applicationId )noexcept(false)->sp<ApplicationStrings>;
		α AppStrings( ApplicationPK applicationId )noexcept->sp<ApplicationStrings>;
		α Messages()noexcept->const UnorderedSet<uint32>&;
	}
}
