#include "stdafx.h"
#include "WebSocket.h"
//#include "../framework/log/server/ServerSink.h"
//#include "../framework/threading/Thread.h"
//#include "../framework/io/Buffer.h"
//#include "../framework/io/sockets/ProtoClient.h"
#include "Listener.h"
#include "Cache.h"
#define var const auto

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

namespace Jde::WebSocket	
{
	Server::Server( uint16 port )noexcept:
		_port{ port }
	{
	}
	void Server::StartAccepting()noexcept
	{
		TRACE0( "WebSocket::StartAccepting" );
		_pAcceptor = make_shared<Threading::InterruptibleThread>( "WebSocketServer", [&](){Accept();} );
		IApplication::AddThread( _pAcceptor );
	}

//auto pSession = make_shared<websocket::stream<tcp::socket>>( std::move(socket) );
	Session::Session( sp<Server> pServer, uint id, tcp::socket& socket )noexcept(false):
		Id{ id },
		_stream{ websocket::stream<tcp::socket>{std::move(socket)} },
		_pServer{ pServer }
	{
		_stream.binary( true );
		_stream.accept();
	}

	void Session::Close()noexcept
	{
		boost::beast::websocket::close_reason cr{ boost::beast::websocket::close_code::service_restart, "Server Shutdown" };
		boost::system::error_code ec;
		_stream.close( cr, ec );
		if( ec )
			ERR( "_stream.close returned:  '{}'", ec.message() );
	}

	void Session::Start()noexcept
	{
		_pThread = make_shared<Threading::InterruptibleThread>( fmt::format("Web Session '{}'", Id), [&]()
		{
			Run();
		});
		IApplication::AddThread( _pThread );
	}

	void Session::Disconnect()noexcept
	{
		_connected = false;
		_pServer->RemoveSession( Id );
	}

	VectorPtr<google::protobuf::uint8> Session::TryToBuffer( const google::protobuf::MessageLite& msg )noexcept
	{
		auto pData = make_shared<vector<google::protobuf::uint8>>( msg.ByteSizeLong() );
		var result = msg.SerializeToArray( pData->data(), (int)pData->size() );
		if( !result )
		{
			WARN0( "Could not serialize to an array" );
			pData = nullptr;
		}
		return pData;
	}
}
//////////////////////////////////////////////////////////////
