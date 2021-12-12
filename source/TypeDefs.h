#pragma once
#include <jde/TypeDefs.h>
#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
namespace Jde
{
	typedef uint ApplicationInstancePK;
	typedef uint ApplicationPK;
	typedef uint LogPK;
}