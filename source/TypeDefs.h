#pragma once
#include <jde/TypeDefs.h>
#include "../../Framework/source/io/proto/messages.pb.h"

#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
namespace Jde
{
	using ApplicationInstancePK=uint32;
	using ApplicationPK=uint32;
	using LogPK=uint;
	using SessionInfo=Logging::Proto::SessionInfo;
}