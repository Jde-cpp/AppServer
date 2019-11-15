#include "stdafx.h"
#include "LogClient.h"
#include "LogData.h"
#include "WebServer.h"
//#include "../framework/Collections.h"


#define var const auto
namespace Jde::Logging
{
	//using namespace Logging::Data;
	sp<LogClient> LogClient::_pInstance;
	void LogClient::CreateInstance()noexcept(false)
	{
		ASSERT( !_pInstance );
		var [applicationId, applicationInstanceId, dbLogLevel, fileLogLevel] = Logging::Data::AddInstance( "Main", Diagnostics::HostName(), Diagnostics::ProcessId() );
		_pInstance = sp<LogClient>{ new LogClient(applicationId, applicationInstanceId,dbLogLevel) };
		SetServerSink( _pInstance.get() );
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

	void LogClient::Log( const Logging::MessageBase& msg )noexcept
	{
		const static vector<string> values;
		Log( msg, values );
	}
	void LogClient::Log( const Logging::MessageBase& msg, const vector<string>& values )noexcept
	{
		if( ShouldSendMessage(msg.MessageId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::MessageId, msg.MessageId, make_shared<string>(msg.MessageView) );
		if( ShouldSendFile(msg.FileId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::FileId, msg.FileId, make_shared<string>(msg.File) );
		if( ShouldSendFunction(msg.FunctionId) )
			Logging::Data::SaveString( ApplicationId, Proto::EFields::FunctionId, msg.FunctionId, make_shared<string>(msg.Function) );

		Logging::Data::PushMessage( ApplicationId, InstanceId, Clock::now(), msg.Level, msg.MessageId, msg.FileId, msg.FunctionId, msg.LineNumber, msg.UserId, msg.ThreadId, values );
		if( msg.Level>=_webLevel )
			ApplicationServer::Web::MyServer::GetInstance()->PushMessage( ApplicationId, InstanceId, Clock::now(), msg.Level, msg.MessageId, msg.FileId, msg.FunctionId, msg.LineNumber, msg.UserId, msg.ThreadId, values );
	}
}