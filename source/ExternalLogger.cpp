#include "ExternalLogger.h"
#include <jde/appClient/AppClient.h>
#include <jde/appClient/proto/App.FromClient.h>
#include "Server.h"
#include "LogData.h"

namespace Jde::App{
	α ExternalLogger::Log( Logging::ExternalMessage&& m, SL sl )ι->void{
		Log( m );
	}
	α ExternalLogger::Log( const Logging::ExternalMessage& m, const vector<string>* args, SL sl )ι->void{
		if( _minLevel==ELogLevel::NoLog || m.Level<_minLevel )
			return;
		auto logEntry = FromClient::ToLogEntry(m);
		BroadcastLogEntry( 0, AppId(), InstanceId(), logEntry, *args );
		try{
			if( _previouslySaved.AddMessage(m.MessageId) )
				SaveString( AppId(), Proto::FromClient::EFields::MessageId, (uint32)m.MessageId, string{m.MessageView} );
			if( _previouslySaved.AddFile(m.FileId) )
				SaveString( AppId(), Proto::FromClient::EFields::FileId, (uint32)m.FileId, m.File );
			if( _previouslySaved.AddFunction(m.FunctionId) )
				SaveString( AppId(), Proto::FromClient::EFields::FunctionId, (uint32)m.FunctionId, m.Function );
			SaveMessage( AppId(), InstanceId(), logEntry, args );
		}
		catch( const IException& ){}
	}
	α ExternalLogger::SetMinLevel( ELogLevel level )ι->void{
		_minLevel = level;
		BroadcastStatus();
	}
}