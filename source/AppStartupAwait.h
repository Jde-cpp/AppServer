#pragma once
#include <jde/framework/coroutine/Await.h>
#include "LogData.h"

namespace Jde::App{
	struct AppStartupAwait final : VoidAwait<>{
		using base = VoidAwait<>;
		AppStartupAwait( jobject webServerSettings, SRCE )ε:base{sl},_webServerSettings{move(webServerSettings)}{}
	private:
		α Suspend()ι->void{ Execute(); }
		α Execute()ι->VoidAwait<>::Task;
		jobject _webServerSettings;
	};
}