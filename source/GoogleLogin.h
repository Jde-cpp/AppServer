#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Google/source/TokenInfo.h"
namespace Jde::GoogleLogin
{
	using namespace Coroutine;
	struct GoogleLoginAwait : IAwait
	{
		GoogleLoginAwait( string&& token ):_token{ move(token) }{}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		variant<up<Google::TokenInfo>,up<IException>> _result;
		string _token;
	};

	Ξ Verify( string token )ι->GoogleLoginAwait{ return GoogleLoginAwait(move(token)); }
}