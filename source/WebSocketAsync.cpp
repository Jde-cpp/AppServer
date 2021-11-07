#include "WebSocketAsync.h"
#include <jde/Exception.h>

#define var const auto
namespace Jde::WebSocket
{
	ELogLevel WebListener::_logLevel{ ELogLevel::Debug };

	WebListener::WebListener( PortType port )noexcept(false):
		IServerSocket{ port },
		_pContextThread{ IOContextThread::Instance() },
		_acceptor{ _pContextThread->Context() }
	{
		try
		{
			const tcp::endpoint ep{ boost::asio::ip::tcp::v4(), ISocket::Port };
			_acceptor.open( ep.protocol() );
			_acceptor.set_option( net::socket_base::reuse_address(true) );
			_acceptor.bind( ep );
			_acceptor.listen( net::socket_base::max_listen_connections );
			DoAccept();
		}
		catch( boost::system::system_error& e )
		{
			throw CodeException( e.what(), e.code() );
		}
	}

#define CHECK_EC(ec, ...) if( ec ){ CodeException x(ec __VA_OPT__(,) __VA_ARGS__); return; }
	void WebListener::DoAccept()noexcept
	{
		_acceptor.async_accept( net::make_strand(_pContextThread->Context()), [this]( beast::error_code ec, tcp::socket socket )noexcept
		{
			if( /*ec.value() == 125 &&*/ IApplication::ShuttingDown() )
				return INFO("Webserver shutdown");
			CHECK_EC( ec );
			var id = ++_id;
			sp<ISession> pSession = CreateSession( *this, id, move(socket) );//deadlock if included in _sessions.emplace
			unique_lock l{ _sessionMutex };
			_sessions.emplace( id, pSession );
			DoAccept();
		} );
	}

	void Session::Run()noexcept
	{
		DBG( "Session::Run()" );
		net::dispatch( _ws.get_executor(), beast::bind_front_handler(&Session::OnRun, shared_from_this()) );
	}

	void Session::OnRun()noexcept
	{
		DBG( "Session::OnRun()" );
		_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
		_ws.set_option( websocket::stream_base::decorator([]( websocket::response_type& res )
		{
			res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async" );
		}) );
		_ws.async_accept( beast::bind_front_handler(&Session::OnAccept,shared_from_this()) );
	}
	void Session::OnAccept( beast::error_code ec )noexcept
	{
		DBG( "Session::OnAccept()" );
		CHECK_EC( ec );
		DoRead();
	}
	void Session::DoRead()noexcept
	{
		DBG( "Session::DoRead()" );
		 _ws.async_read( _buffer, [this]( beast::error_code ec, uint bytes_transferred )noexcept
		{
			boost::ignore_unused( bytes_transferred );
			CHECK_EC( ec, ec == websocket::error::closed ? LogLevel().Level : ELogLevel::Error );
			OnRead( (char*)_buffer.data().data(), _buffer.size() );
			DoRead();
		} );
	}


	void Session::OnWrite( beast::error_code ec, std::size_t bytes_transferred )noexcept
	{
		boost::ignore_unused(bytes_transferred);
		try
		{
			THROW_IFX( ec, CodeException(ec, ec == websocket::error::closed ? LogLevel().Level : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}
}
