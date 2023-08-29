#include "../../Framework/source/io/sockets/RestServer.h"


namespace Jde::ApplicationServer::Rest
{
	using namespace IO::Rest;
	struct Session : ISession, std::enable_shared_from_this<Session>
	{
		Session( tcp::socket&& socket ): ISession{move(socket)}{}
		virtual ~Session(){}
		α HandleRequest( http::request<http::string_body>&& request, sp<ISession> s )ι->void override;
		α MakeShared()ι->sp<ISession> override{ return shared_from_this(); }
	};
	α Listener()ι->TListener<Session>&;
}
