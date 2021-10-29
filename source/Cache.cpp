#include "Cache.h"
#include "LogData.h"
#include "../../Framework/source/collections/UnorderedSet.h"

#define var const auto

namespace Jde::ApplicationServer
{
	namespace Cache
	{
		UnorderedMap<uint,Messages::Application> _sessions;
		UnorderedMap<ApplicationPK,ApplicationStrings> _applicationStrings;
		UnorderedMap<uint,UnorderedMap<uint,string>> _instanceThreads;
	}
	UnorderedSet<uint32> _messages;
	α Cache::Messages()noexcept->const UnorderedSet<uint32>&{ return _messages; }
	α Cache::AddSession( uint id, shared_ptr<Messages::Application> pApplication )->void{ _sessions.emplace( id, pApplication ); }
	α Cache::AppStrings( ApplicationPK applicationId )noexcept->sp<ApplicationStrings>{ return _applicationStrings.Find(applicationId); };
	α Cache::Load( ApplicationPK applicationId )noexcept(false)->sp<ApplicationStrings>
	{
		static mutex loadMutex;
		std::lock_guard l{loadMutex};
		auto result = _applicationStrings.emplace( applicationId, make_shared<ApplicationStrings>(applicationId) );
		if( result.second )
		{
			auto& strings = *result.first.second;
			strings.Files = Logging::Data::LoadFiles( applicationId );
			strings.Functions = Logging::Data::LoadFunctions( applicationId );
			strings.Messages = Logging::Data::LoadMessages( applicationId );
			if( !_messages.size() )
				_messages = Logging::Data::LoadMessages();
		}
		return result.first.second;
	}

	α Cache::Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value )->void
	{
		auto pStrings = _applicationStrings.Find( applicationId );
		if( !pStrings )
			return DBG( "No application strings loaded for {}", applicationId );
		auto pValue = make_shared<string>( value.size() ? value : "{null}" );
		Logging::Data::SaveString( applicationId, field, id, pValue );
		switch( field )
		{
		case Logging::Proto::EFields::MessageId:
			_messages.emplace( id );
			break;
		case Logging::Proto::EFields::FileId:
			pStrings->Files.emplace( id, pValue );
			break;
		case Logging::Proto::EFields::FunctionId:
			pStrings->Functions.emplace( id, pValue );
			break;
		default:
			ERR( "unknown field {}.", field );
		}
	}


	α Cache::AddThread( uint sessionId, uint threadId, sv thread )->void
	{
		function<void(UnorderedMap<uint,string>&)> afterInsert = [threadId, thread](UnorderedMap<uint,string>& value){ value.Set( threadId, make_shared<string>(thread) ); };
		_instanceThreads.Insert( afterInsert, sessionId, shared_ptr<UnorderedMap<uint,string>>{ new UnorderedMap<uint,string>() } );
	}

	α Cache::ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func )->uint
	{
		return ((const UnorderedMap<uint,Messages::Application>&)_sessions).ForEach( (decltype(func))func );
	}



	α ApplicationStrings::Get( Logging::EFields field, uint id )noexcept->shared_ptr<string>
	{
		shared_ptr<string> pString;
		switch( field )
		{
		case Logging::EFields::Message:
			pString = Messages.Find( (uint32)id );
		break;
		case Logging::EFields::File:
			pString = Files.Find( (uint32)id );
		break;
		case Logging::EFields::Function:
			pString = Functions.Find( (uint32)id );
		break;
		case Logging::EFields::User:
			pString = UsersPtr->Find( (uint32)id );
		break;
		default:
			WARN( "requested string for field '{}'", field );
		}
		return pString;
	}
}
