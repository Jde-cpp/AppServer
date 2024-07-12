#pragma once
#include <jde/web/flex/IHttpRequestAwait.h>

namespace Jde::App{
	using namespace Jde::Web::Flex;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α await_suspend( base::Handle h )ε->void;
		α await_resume()ε->HttpTaskResult override;
	};
}
