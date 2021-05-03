#pragma once

#include "../../Framework/source/collections/UnorderedMap.h"

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
		UnorderedMapPtr<uint32,string> FilesPtr;
		UnorderedMapPtr<uint32,string> FunctionsPtr;
		UnorderedMapPtr<uint32,string> MessagesPtr;
		UnorderedMapPtr<uint32,string> ThreadsPtr;
		UnorderedMapPtr<uint32,string> UsersPtr;
		void Add( const Messages::Message& message )noexcept;
		shared_ptr<string> Get( Logging::EFields field, uint id )noexcept;
	};

	struct Cache
	{
		static void Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value );
		static void AddSession( uint id, shared_ptr<Messages::Application> pApplication );
		static void AddThread( uint sessionId, uint threadId, sv thread );
		static void AddMessageStrings( uint sessionId, const Messages::Message& message );
		static uint ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func );
		static void FetchStrings( uint sessionId, const Messages::RequestStrings& request );
		static sp<ApplicationStrings> Load( ApplicationPK applicationId )noexcept(false);
		static sp<ApplicationStrings> GetApplicationStrings( ApplicationPK applicationId )noexcept{ return _applicationStrings.Find(applicationId); };
	private:
		static UnorderedMap<uint,Messages::Application> _sessions;
		static UnorderedMap<ApplicationPK,ApplicationStrings> _applicationStrings;
		static UnorderedMap<uint,UnorderedMap<uint,string>> _instanceThreads;
	};
}
