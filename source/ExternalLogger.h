#pragma once
//#include <jde/log/Log.h>

namespace Jde::App{
	//Log server messages to db
	struct ExternalLogger final : Logging::IExternalLogger{
		α Destroy( SL )ι->void override{};
		α Name()ι->string override{ return "db"; }
		α Log( Logging::ExternalMessage&& m, SRCE )ι->void;
		α Log( const Logging::ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void override;
		α SetMinLevel( ELogLevel level )ι->void override;
	};
}