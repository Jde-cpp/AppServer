
DISABLE_WARNINGS
#include <filesystem>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
ENABLE_WARNINGS

#include <jde/web/client/exports.h>
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include <jde/app/shared/exports.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/Common.pb.h>
