#include "Cache.h"
//#include "WebSocket.h"
#include "LogData.h"

//#include "../framework/log/server/ReceivedMessages.h"

#define var const auto

namespace Jde::ApplicationServer
{
	UnorderedMap<uint,Messages::Application> Cache::_sessions;
	//UnorderedMap<string,ApplicationStrings> Cache::_applicationStrings;
	UnorderedMap<ApplicationPK,ApplicationStrings> Cache::_applicationStrings;
	UnorderedMap<uint,UnorderedMap<uint,string>> Cache::_instanceThreads;

	void Cache::AddSession( uint id, shared_ptr<Messages::Application> pApplication )
	{

		_sessions.emplace( id, pApplication );
		//std::thread{ [&]{LoadMessages(pApplication);} }.detach();
		//std::thread{ [&]{LoadFiles(pApplication);} }.detach();
		//std::thread{ [&]{LoadFunctions(pApplication);} }.detach();

	}
	sp<ApplicationStrings> Cache::Load( ApplicationPK applicationId )noexcept(false)
	{
		static mutex loadMutex;
		std::lock_guard l{loadMutex};
		auto result = _applicationStrings.emplace( applicationId, new ApplicationStrings{applicationId} );
		if( result.second )
		{
			auto& strings = *result.first.second;
			strings.FilesPtr = Logging::Data::LoadFiles( applicationId );
			strings.FunctionsPtr = Logging::Data::LoadFunctions( applicationId );
			strings.MessagesPtr = Logging::Data::LoadMessages( applicationId );
		}
		return result.first.second;
	}

	void Cache::Add( ApplicationPK applicationId, Logging::Proto::EFields field, uint32 id, sv value )
	{
		auto pStrings = _applicationStrings.Find( applicationId ); 
		if( !pStrings )
		{
			Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Debug, "No application strings loaded for {}"sv, "C:\\Users\\duffyj\\source\\repos\\jde\\AppServer\\source\\Cache.cpp", __func__, 45), applicationId );
			return;
		}
		auto pValue = make_shared<string>(value);
		Logging::Data::SaveString( applicationId, field, id, pValue );
		switch( field )
		{
		case Logging::Proto::EFields::MessageId:
			pStrings->MessagesPtr->emplace( id, pValue );
			break;
		case Logging::Proto::EFields::FileId:
			pStrings->FilesPtr->emplace( id, pValue );
			break;
		case Logging::Proto::EFields::FunctionId:
			pStrings->FunctionsPtr->emplace( id, pValue );
			break;
		// case Proto::EFields::ThreadId:
		// 	pStrings->MessagesPtr->Emplace( id, value );
		// 	break;
		default:
			Logging::Log( Logging::MessageBase(ELogLevel::Error, "unknown field {}.", "C:\\Users\\duffyj\\source\\repos\\jde\\AppServer\\source\\Cache.cpp", __func__, 65, IO::Crc::Calc32RunTime("unknown field {}."), IO::Crc::Calc32RunTime("C:\\Users\\duffyj\\source\\repos\\jde\\AppServer\\source\\Cache.cpp"), IO::Crc::Calc32RunTime(__func__)), field );
			//ERR( "unknown field {}."sv, field );
		}
	}


	void Cache::AddThread( uint sessionId, uint threadId, sv thread )
	{
		function<void(UnorderedMap<uint,string>&)> afterInsert = [threadId, thread](UnorderedMap<uint,string>& value){ value.Set( threadId, make_shared<string>(thread) ); };
		_instanceThreads.Insert( afterInsert, sessionId, shared_ptr<UnorderedMap<uint,string>>{ new UnorderedMap<uint,string>() } );
	}
	void Cache::AddMessageStrings( uint /*sessionId*/, const Messages::Message& /*message*/ )
	{
		//var pApplication = _sessions.Find( sessionId );
		//if( pApplication==nullptr )
		//	ERR_ONCE( "Could not find applicaiton for session {}.", sessionId );
		//else
		//	_applicationStrings.Insert( [&message]( ApplicationStrings& strings ){ strings.Add(message); }, pApplication->Name, shared_ptr<ApplicationStrings>{ new ApplicationStrings() } );
	}

	uint Cache::ForEachApplication( std::function<void(const uint&,const Messages::Application&)> func )
	{
		return _sessions.ForEach( func );
	}

	void Cache::FetchStrings( uint /*sessionId*/, const Messages::RequestStrings& /*request*/ )
	{
/*		auto pResult = make_shared<Messages::Strings>( request.ApplicationInstanceId );
		auto pApplication = _sessions.Find( request.ApplicationInstanceId );
		if( !pApplication )
		{
			ERR( "Could not find application for session '{}'", sessionId );
			return;
		}
		for( var& [field,id] : request.Fields )
		{
			shared_ptr<string> pStr;
			if( field==EFields::Thread )
			{
				var pThreadStrings = _instanceThreads.Find( sessionId );
				if( !pThreadStrings )
					ERR( "Could not find thread strings for session '{}'.", sessionId );
				else
				{
					pStr = pThreadStrings->Find( id );
					if( !pStr )
						ERR( "Could not find thread string for id '{}'.", id );
				}
			}
			else
			{
				var pApplicationStrings = _applicationStrings.Find( pApplication->Name );
				if( !pApplicationStrings )
					ERR( "Could not find application strings for '{}'.", pApplication->Name );
				else
				{
					pStr = pApplicationStrings->Get( field, id );
					if( !pStr )
					{
						ERR( "Could not find 0x'{0:x}' string for id '{}' in applicaiton '{}'.", (uint)field, id, pApplication->Name );
						pStr = std::make_shared<string>( "Unknown" );
					}
				}
			}
			if( pStr )
			{
				auto pMap = pResult->Values.emplace( field, map<uint,string>{} ).first;
				pMap->second.emplace( id, *pStr );
			}
		}
		if( pResult->Values.size() )
			WebSocket::Instance().Push( sessionId, pResult );
	*/
	}

	void ApplicationStrings::Add( const Messages::Message& /*message*/ )noexcept
	{
/*		if( message.File.size() )
		{
			//DBG( "Adding file ({}){}", message.FileId, message.File );
			Files.Set( message.FileId, make_shared<string>(message.File) );
		}
		//else
//			DBG( "Not adding file ({})", message.FileId );
		if( message.Function.size() )
			Functions.Set( message.FunctionId, make_shared<string>(message.Function) );
		if( message.MessageView.size() )
			Messages.Set( message.MessageId, make_shared<string>(message.MessageView) );
		if( message.User.size() )
			Users.Set( message.UserId, make_shared<string>(message.User) );

		if( message.FileId && !Files.Find(message.FileId) )
			ERR( "Do not have file for id:  {}", message.FileId );
		if( message.FunctionId && !Functions.Find(message.FunctionId) )
			ERR( "Do not have function for id:  {}", message.FunctionId );
		if( message.MessageId && !Messages.Find(message.MessageId) )
			ERR( "Do not have message for id:  {}", message.MessageId );
		if( message.UserId && !Users.Find(message.UserId) )
			ERR( "Do not have user for id:  {}", message.UserId );
*/
	}

	shared_ptr<string> ApplicationStrings::Get( Logging::EFields field, uint id )noexcept
	{
		shared_ptr<string> pString;
		switch( field )
		{
		case Logging::EFields::Message:
			pString = MessagesPtr->Find( (uint32)id );
		break;
		case Logging::EFields::File:
			pString = FilesPtr->Find( (uint32)id );
		break;
		case Logging::EFields::Function:
			pString = FunctionsPtr->Find( (uint32)id );
		break;
		case Logging::EFields::User:
			pString = UsersPtr->Find( (uint32)id );
		break;
		default:
			//WARN( "requested string for field '{}'"sv, field );
			Logging::Log( Logging::MessageBase(ELogLevel::Warning, "requested string for field '{}'"sv, "C:\\Users\\duffyj\\source\\repos\\jde\\AppServer\\source\\Cache.cpp", __func__, 184), field );
		}
		return pString;
	}
}
