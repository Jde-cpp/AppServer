#include "Cache.h"
#include "LogData.h"

#define var const auto

namespace Jde::App{
	namespace Cache{
		concurrent_flat_map<SessionPK,Proto::FromServer::Application> _sessions;
		ApplicationStrings _applicationStrings;
		concurrent_flat_map<SessionPK,flat_map<ThreadPK,string>> _instanceThreads;
	}
	concurrent_flat_set<uint32> _messages;
	α Cache::Messages()ι->const concurrent_flat_set<uint32>&{ return _messages; }
	α Cache::AddSession( uint id, Proto::FromServer::Application app )ι->void{ _sessions.emplace( id, move(app) ); }
	α Cache::AppStrings()ι->ApplicationStrings&{ return _applicationStrings; };

	std::once_flag _single;
	mutex _loadMutex;
	α Cache::Load( /*AppPK applicationId*/ )ε->ApplicationStrings&{
		lg _{_loadMutex};
		std::call_once( _single, [](){
			_applicationStrings.Files = LoadFiles();
			_applicationStrings.Functions = LoadFunctions();
			_applicationStrings.Messages = LoadMessages();
			_messages = LoadMessageIds();
		} );
		return _applicationStrings;
	}

	α Cache::Add( AppPK applicationId, Proto::FromClient::EFields field, StringPK id, string value )ι->void{
		//auto pStrings = _applicationStrings.Find( applicationId ); if( !pStrings ) return DBG( "No application strings loaded for {}", applicationId );
		Load();
		if( value.empty() )
			value = "{null}";
		bool save = false;
		if( field==Proto::FromClient::EFields::MessageId )
			save = _messages.emplace( id );
		else if( field==Proto::FromClient::EFields::FileId )
			save = _applicationStrings.Files.try_emplace( id, value );
		else if( field==Proto::FromClient::EFields::FunctionId )
			save = _applicationStrings.Functions.try_emplace( id, value );
		else
			ERRT( AppTag(), "unknown field {}.", (int)field );
		if( save )
			SaveString( applicationId, field, id, move(value) );
	}

	α Cache::AddThread( SessionPK sessionId, ThreadPK threadId, string thread )ι->void{
		_instanceThreads.try_emplace_or_visit( sessionId, flat_map<ThreadPK,string>{}, [threadId,&thread](auto&& kv){kv.second[threadId]=move(thread);} );
	}

	α Cache::ForEachApplication( function<void(SessionPK,const Proto::FromServer::Application&)> f )ι->uint{
		return _sessions.cvisit_all( [&](auto&& kv){f(kv.first, kv.second);} );
		//return ((const concurrent_flat_map<uint,Proto::FromServer::Application>&)_sessions).ForEach( (decltype(func))func );
	}

	α ApplicationStrings::Get( Logging::EFields field, LogPK id )ι->string{
		string y;
		if( field==Logging::EFields::Message )
			Messages.cvisit( id, [&](var& kv){ y=kv.second; } );
		else if( field==Logging::EFields::File )
			Files.cvisit( id, [&](var& kv){ y=kv.second; } );
		else if( field==Logging::EFields::Function )
			Functions.cvisit( id, [&](var& kv){ y=kv.second; } );
		else if( field==Logging::EFields::User )
			Users.cvisit( id, [&](var& kv){ y=kv.second; } );
		else
			WARNT( AppTag(), "requested string for field '{}'", (int)field );
		return y;
	}
}