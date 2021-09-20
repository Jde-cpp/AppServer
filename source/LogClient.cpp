#include "LogClient.h"
#include "LogData.h"
#ifndef TESTING
	#include "WebServer.h"
#endif

#define var const auto
namespace Jde::Logging
{
	void LogClient::CreateInstance()noexcept(false)
	{
		ASSERT( !_pServerSink );
		var [applicationId, applicationInstanceId, dbLogLevel, fileLogLevel] = Logging::Data::AddInstance( "Main", IApplication::HostName(), OSApp::ProcessId() );
		auto p = make_unique<LogClient>( applicationId, applicationInstanceId, dbLogLevel );
		SetServerSink( move(p) );
	}

	LogClient::LogClient( ApplicationPK id, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false):
		InstanceId{applicationInstanceId},
		ApplicationId{id}
	{
		SetServerLevel( serverLevel );
		auto addMessages =[]( var& map, auto& set )
		{
			std::function<void(const uint32&, const string&)> fnctn = [&set](const uint32& key, const string&) {set.emplace(key);};
			map.ForEach( fnctn );
		};
		addMessages( Data::LoadMessages(id), _messagesSent );
		addMessages( Data::LoadFiles(id), _filesSent );
		addMessages( Data::LoadFunctions(id), _functionsSent );


	}
	void LogClient::Log( Logging::Messages::Message& message )noexcept
	{
		Log( dynamic_cast<Logging::MessageBase&>(message), message.Variables );
	}
	void LogClient::Log( const Logging::MessageBase& msg )noexcept
	{
		vector<string> v;
		Log( msg, v );
	}
	mutex _messageMutex;//if 1st function save, 2nd will skip to insert and get fk error.
	void LogClient::Log( const Logging::MessageBase& msg, vector<string>& values )noexcept
	{
#ifndef TESTING
		if( msg.Level>=_webLevel )
			ApplicationServer::Web::Server().PushMessage( 0, ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, vector<string>{values} );
#endif
		unique_lock<mutex> l{ _messageMutex };
		if( ShouldSendMessage(msg.MessageId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::MessageId, (uint32)msg.MessageId, make_shared<string>(msg.MessageView) );
		if( ShouldSendFile(msg.FileId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::FileId, (uint32)msg.FileId, make_shared<string>(msg.File) );
		if( ShouldSendFunction(msg.FunctionId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::FunctionId, (uint32)msg.FunctionId, make_shared<string>(msg.Function) );
		Logging::Data::PushMessage( ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, move(values) );
	}
}