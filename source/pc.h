#ifdef _MSC_VER
	#include <SDKDDKVer.h>
#endif

#include <filesystem>
#include <forward_list>
#include <set>
#include <shared_mutex>
#include <unordered_map>

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "TypeDefs.h"
#include "types/proto/AppFromServer.pb.h"
#include "types/proto/AppFromClient.pb.h"

#include <jde/Log.h>
#include <jde/Str.h>
#include <jde/log/types/proto/FromServer.pb.h>
#include "../../Ssl/source/Ssl.h"

#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/io/ProtoUtilities.h"
#include "../../Framework/source/io/ServerSink.h"
#include "../../Framework/source/io/Socket.h"
#include "../../Framework/source/threading/Mutex.h"