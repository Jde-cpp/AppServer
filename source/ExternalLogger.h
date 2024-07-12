#pragma once
#include <jde/log/Log.h>
#include <jde/appClient/History.h>

namespace Jde::App{

	//Log server messages to db
	struct ExternalLogger final : Logging::IExternalLogger{
		α Destroy(SRCE)ι->void override{};
		α Name()ι->string override{ return "DB"; }
		α Log( Logging::ExternalMessage&& m, SRCE )ι->void;
		α Log( const Logging::ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void override;
		α SetMinLevel( ELogLevel level )ι->void override;
	private:
		History _previouslySaved;
	};
}