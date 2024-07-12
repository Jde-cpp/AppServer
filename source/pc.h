#ifdef _MSC_VER
	#include <SDKDDKVer.h>
#endif

#include <filesystem>
#include <forward_list>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include "usings.h"

DISABLE_WARNINGS
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/exception/diagnostic_information.hpp>
ENABLE_WARNINGS

#include "../../Public/src/web/Exports.h"
#include <jde/log/Log.h>
#include <jde/Str.h>
#include "../../Ssl/source/Ssl.h"

#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/io/ProtoUtilities.h"
#include "../../Framework/source/threading/Mutex.h"