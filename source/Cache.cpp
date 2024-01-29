#include "Cache.h"
#include "LogData.h"
#include "../../Framework/source/collections/UnorderedSet.h"

#define var const auto

namespace Jde::ApplicationServer
{
	namespace Cache
	{
		UnorderedMap<uint,Messages::Application> _sessions;
		ApplicationStrings _applicationStrings;
		UnorderedMap<uint,UnorderedMap<uint,string>> _instanceThreads;
	}
	UnorderedSet<uint32> _messages;
	α Cache::Messages()ι->const UnorderedSet<uint32>&{ return _messages; }
	α Cache::AddSession( uint id, sp<Messages::Application> pApplication )ι->void{ _sessions.emplace( id, pApplication ); }
	α Cache::AppStrings()ι->ApplicationStrings&{ return _applicationStrings; };

	std::once_flag _single;
	mutex _loadMutex;
	α Cache::Load( /*ApplicationPK applicationId*/ )ε->ApplicationStrings&
	{

		std::lock_guard _{_loadMutex};
		std::call_once( _single, []()
		{
			_applicationStrings.Files = Logging::Data::LoadFiles();
			_applicationStrings.Functions = Logging::Data::LoadFunctions();
			_applicationStrings.Messages = Logging::Data::LoadMessages();
		//	if( !_messages.size() )
			_messages = Logging::Data::LoadMessageIds();
		} );
		return _applicationStrings;
	}

	α Cache::Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value )ι->void
	{
		//auto pStrings = _applicationStrings.Find( applicationId ); if( !pStrings ) return DBG( "No application strings loaded for {}", applicationId );
		Load();
		auto pValue = ms<string>( value.size() ? value : "{null}" );
		bool save = false;
		if( field==Logging::Proto::EFields::MessageId )
			save = _messages.emplace( id );
		else if( field==Logging::Proto::EFields::FileId )
			save = _applicationStrings.Files.emplace( id, pValue ).second;
		else if( field==Logging::Proto::EFields::FunctionId )
			save = _applicationStrings.Functions.emplace( id, pValue ).second;
		else
			ERRT( AppTag(), "unknown field {}.", field );
		if( save )
			Logging::Data::SaveString( applicationId, field, id, pValue );
	}

	α Cache::AddThread( uint sessionId, uint threadId, sv thread )ι->void
	{
		function<void(UnorderedMap<uint,string>&)> afterInsert = [threadId, thread](UnorderedMap<uint,string>& value){ value.Set( threadId, ms<string>(thread) ); };
		_instanceThreads.Insert( afterInsert, sessionId, sp<UnorderedMap<uint,string>>{ new UnorderedMap<uint,string>() } );
	}

	α Cache::ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func )ι->uint{
		return ((const UnorderedMap<uint,Messages::Application>&)_sessions).ForEach( (decltype(func))func );
	}

	α ApplicationStrings::Get( Logging::EFields field, Logging::MessageBase::ID id )ι->sp<string>{
		sp<string> pString;
		if( field==Logging::EFields::Message )
			pString = Messages.Find( id );
		else if( field==Logging::EFields::File )
			pString = Files.Find( id );
		else if( field==Logging::EFields::Function )
			pString = Functions.Find( id );
		else if( field==Logging::EFields::User )
			pString = UsersPtr->Find( id );
		else
			WARNT( AppTag(), "requested string for field '{}'", field );
		return pString;
	}
}