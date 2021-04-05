#include "LogClient.h"
#include "LogData.h"
#ifndef TESTING
	#include "WebServer.h"
#endif

#define var const auto
namespace Jde::Logging
{
	//using namespace Logging::Data;
	sp<LogClient> LogClient::_pInstance;
	void LogClient::CreateInstance()noexcept(false)
	{
		ASSERT( !_pInstance );
		var [applicationId, applicationInstanceId, dbLogLevel, fileLogLevel] = Logging::Data::AddInstance( "Main", IApplication::HostName(), IApplication::ProcessId() );
		_pInstance = sp<LogClient>{ new LogClient(applicationId, applicationInstanceId,dbLogLevel) };
		SetServerSink( _pInstance.get() );
		IApplication::AddShutdown( _pInstance );
	}

	LogClient::LogClient( ApplicationPK applicationId, ApplicationInstancePK applicationInstanceId, ELogLevel serverLevel )noexcept(false):
		IServerSink{serverLevel},
		InstanceId{applicationInstanceId},
		ApplicationId{applicationId}
	{
		auto addMessages =[]( const auto& map, auto& set )
		{
			std::function<void(const uint32&, const string&)> fnctn = [&set](const uint32& key, const string&) {set.emplace(key);};
			map.ForEach( fnctn );
		};
		addMessages( *Data::LoadMessages(applicationId), _messagesSent );
		addMessages( *Data::LoadFiles(applicationId), _filesSent );
		addMessages( *Data::LoadFunctions(applicationId), _functionsSent );

		//   = *LoadFiles(applicationId);
		//  = *(applicationId);
	}
	void LogClient::Log( const Logging::Messages::Message& message )noexcept
	{
		Log( dynamic_cast<const Logging::MessageBase&>(message), message.Variables );
	}
	void LogClient::Log( const Logging::MessageBase& msg )noexcept
	{
		const static vector<string> values;
		Log( msg, values );
	}
	mutex _messageMutex;//if 1st function save, 2nd will skip to insert and get fk error.
	void LogClient::Log( const Logging::MessageBase& msg, const vector<string>& values )noexcept
	{
		{
			unique_lock<mutex> l{ _messageMutex };
			if( ShouldSendMessage(msg.MessageId) )
				Logging::Data::SaveString( ApplicationId, Proto::EFields::MessageId, (uint32)msg.MessageId, make_shared<string>(msg.MessageView) );
			if( ShouldSendFile(msg.FileId) )
				Logging::Data::SaveString( ApplicationId, Proto::EFields::FileId, (uint32)msg.FileId, make_shared<string>(msg.File) );
			if( ShouldSendFunction(msg.FunctionId) )
				Logging::Data::SaveString( ApplicationId, Proto::EFields::FunctionId, (uint32)msg.FunctionId, make_shared<string>(msg.Function) );

			Logging::Data::PushMessage( ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, values );
		}
#ifndef TESTING
		if( msg.Level>=_webLevel )
			ApplicationServer::Web::MyServer::GetInstance()->PushMessage( 0, ApplicationId, InstanceId, Clock::now(), msg.Level, (uint32)msg.MessageId, (uint32)msg.FileId, (uint32)msg.FunctionId, (uint32)msg.LineNumber, (uint32)msg.UserId, msg.ThreadId, values );
#endif
	}
}