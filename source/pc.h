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
#include <jde/Log.h>
#include <jde/Str.h>
#include <jde/log/types/proto/FromServer.pb.h>
#include "../../Ssl/source/Ssl.h"

#include "../../Framework/source/log/server/proto/messages.pb.h"
#include "../../Framework/source/db/Database.h"