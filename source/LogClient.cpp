#include "LogClient.h"
#include "LogData.h"
#ifndef TESTING
	#include "WebServer.h"
#endif

#define var const auto
namespace Jde::Logging
{
	α LogClient::CreateInstance()noexcept(false)->void
	{
		ASSERT( !Server() );
		var [applicationId, applicationInstanceId, dbLogLevel, fileLogLevel] = Data::AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
		auto p = make_unique<LogClient>( applicationId, applicationInstanceId, dbLogLevel );
		SetServer( move(p) );
	}

	LogClient::LogClient( ApplicationPK id, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false):
		IServerSink{ Data::LoadMessageIds() },
		InstanceId{applicationInstanceId},
		ApplicationId{id}
	{
		SetServerLevel( serverLevel );
		auto addMessages =[]( var& map, auto& set )
		{
			std::function<void(const uint32&, const string&)> fnctn = [&set](const uint32& key, const string&) {set.emplace(key);};
			map.ForEach( fnctn );
		};
		addMessages( Data::LoadFiles(), _filesSent );
		addMessages( Data::LoadFunctions(), _functionsSent );


	}
	α LogClient::Log( Messages::ServerMessage& message )noexcept->void
	{
		Log( dynamic_cast<MessageBase&>(message), message.Variables );
	}
	α LogClient::Log( const MessageBase& msg )noexcept->void
	{
		vector<string> v;
		Log( msg, v );
	}
	mutex _messageMutex;//if 1st function save, 2nd will skip to insert and get fk error.
	α LogClient::Log( const MessageBase& msg, vector<string>& values )noexcept->void
	{
#ifndef TESTING
		if( msg.Level>=_webLevel )
			ApplicationServer::Web::Server().PushMessage( 0, ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, vector<string>{values} );
#endif
		try
		{
			unique_lock<mutex> l{ _messageMutex };
			if( ShouldSendMessage(msg.MessageId) )//2030045667
				Data::SaveString( ApplicationId, Proto::EFields::MessageId, (uint32)msg.MessageId, make_shared<string>(msg.MessageView) );
			if( ShouldSendFile(msg.FileId) )
				Data::SaveString( ApplicationId, Proto::EFields::FileId, (uint32)msg.FileId, make_shared<string>(msg.File) );
			if( ShouldSendFunction(msg.FunctionId) )
				Data::SaveString( ApplicationId, Proto::EFields::FunctionId, (uint32)msg.FunctionId, make_shared<string>(msg.Function) );
			Data::PushMessage( ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, move(values) );
		}
		catch( ... )
		{
			try
			{
				std::rethrow_exception( std::current_exception() );
			}
			catch( const std::exception& e )
			{
				CRITICAL( "{}", e.what() );
			}
		}
	}
}