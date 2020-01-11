// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifdef _WINDOWS
//	#pragma once
	#include <SDKDDKVer.h>
#endif

#include <filesystem>
#include <list>
#include <nlohmann/json.hpp>

#ifndef __INTELLISENSE__
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
#endif

#pragma warning( disable : 4245) 
#include <boost/crc.hpp> 
#pragma warning( default : 4245) 
#include <boost/system/error_code.hpp>

#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/asio/ts/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>


#include "TypeDefs.h"
#include "../../Framework/source/JdeAssert.h"
#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/StringUtilities.h"
#include "../../Framework/source/Diagnostics.h"
#include "../../Framework/source/Settings.h"
#include "../../Framework/source/log/Logging.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../Framework/source/db/Database.h"
#include "../../Framework/source/db/Row.h"
#include "../../Framework/source/threading/Thread.h"
#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include "../../Framework/source/io/sockets/ProtoServer.h"
#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/collections/Queue.h"
#include "../../Framework/source/db/DBQueue.h"
#include "../../Framework/source/threading/InterruptibleThread.h"
#include "../../Framework/source/application/Application.h"
#include "../../Framework/source/io/sockets/ProtoClient.h"
#include "../../Framework/source/io/File.h"


#pragma warning( push )
#pragma warning (disable: 4244)
#pragma warning (disable: 4267)
#include "../../Framework/source/log/server/proto/messages.pb.h"
#pragma warning( pop )

namespace Jde
{
	using std::list;
}
