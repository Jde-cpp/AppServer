#pragma once

//#include "../../Framework/source/collections/UnorderedMap.h"
//#include "../../Framework/source/collections/UnorderedSet.h"

namespace Jde::App{
	using boost::concurrent_flat_map;

	struct ApplicationStrings{
		ApplicationStrings()
		{}
		α Add( const Proto::FromServer::Message& message )ι->void;
		α Get( Logging::EFields field, LogPK id )ι->string;

		concurrent_flat_map<StringPK,string> Files;
		concurrent_flat_map<StringPK,string> Functions;
		concurrent_flat_map<StringPK,string> Messages;
		concurrent_flat_map<StringPK,string> Threads;
		concurrent_flat_map<StringPK,string> Users;
	};
}
namespace Jde::App::Cache{
	α Add( AppPK applicationId, Proto::FromClient::EFields field, StringPK id, string value )ι->void;
	α AddMessageStrings( uint sessionId, const Proto::FromServer::Message& message )ι->void;
	α AddSession( uint id, Proto::FromServer::Application app )ι->void;
	α AddThread( SessionPK sessionId, ThreadPK threadId, string thread )ι->void;
	α AppStrings()ι->ApplicationStrings&;
	α FetchStrings( uint sessionId, const Proto::FromClient::RequestString& request )ι->void;
	α ForEachApplication( function<void(SessionPK,const Proto::FromServer::Application&)> func )ι->uint;
	α Load()ε->ApplicationStrings&;
	α Messages()ι->const concurrent_flat_set<StringPK>&;
}