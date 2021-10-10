#pragma once
#pragma region Defines
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <jde/App.h>
#include "../../Framework/source/io/sockets/Socket.h"
#include "../../Framework/source/io/ProtoUtilities.h"
#include "../../Framework/source/collections/UnorderedMap.h"
#define var const auto

namespace Jde::WebSocket
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	using namespace Jde::IO::Sockets;
#pragma endregion

	struct WebListener /*abstract*/ : IO::Sockets::IServerSocket, std::enable_shared_from_this<WebListener>
	{
		WebListener( PortType port )noexcept(false);
		~WebListener(){ _acceptor.close(); DBG("~WebListener"); }
		Ω LogLevel()noexcept{ return _logLevel; }
		β CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept->sp<ISession> =0;
	private:
		α DoAccept()noexcept->void;
		α OnAccept( beast::error_code ec, tcp::socket socket )noexcept->void;
		atomic<bool> _shutdown{false};
		sp<IOContextThread> _pContextThread;
		static ELogLevel _logLevel;
	protected:
		tcp::acceptor _acceptor;
	};

	template<class TFromServer, class TServerSession>
	struct TListener /*abstract*/ : WebListener, IShutdown
	{
		TListener( PortType port )noexcept: WebListener{ port } {}
		~TListener()=0;
		α Push( IO::Sockets::SessionPK sessionId, TFromServer&& m )noexcept->void;
		α CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept->sp<ISession> override{ auto p = make_shared<TServerSession>(server, id, move(socket)); p->Run(); return p; }
		α Shutdown()noexcept->void override;
		α Find( IO::Sockets::SessionPK id )noexcept->sp<TServerSession>
		{
			shared_lock l{ _sessionMutex };
			return _sessions.find(id)==_sessions.end() ? sp<TServerSession>{} : static_pointer_cast<TServerSession>( _sessions.find(id)->second );
		}
	protected:
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;
	};

	struct Session /*abstract*/: IO::Sockets::ISession, std::enable_shared_from_this<Session>
	{
		Session( WebListener& server, SessionPK id, tcp::socket&& socket ):ISession{id}, _ws{std::move(socket)}, _server{server}{}
		β Close()noexcept->void{};
		β Run()noexcept->void;
	protected:
		α Disconnect()noexcept{ /*_connected = false;*/ _server.RemoveSession( Id ); }

		websocket::stream<beast::tcp_stream> _ws;
		WebListener& _server;
	private:
		α OnRun()noexcept->void;
		β OnAccept( beast::error_code ec )noexcept->void;
		α DoRead()noexcept->void;
		α OnNetRead( beast::error_code ec, std::size_t bytes_transferred )noexcept->void;
		β OnRead( const char* p, uint size )noexcept->void=0;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )noexcept->void;

		beast::flat_buffer _buffer;
	};
	template<class TFromServer, class TFromClient>
	struct TSession /*abstract*/ : Session//, public std::enable_shared_from_this<TSession<TFromServer,TFromClient>>
	{
		TSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept(false) : Session{ server, id, move(socket) }{}

		α OnRead( const char* p, uint size )noexcept->void;
		β OnRead( TFromClient transmission )noexcept->void = 0;
		α Write( TFromServer&& message )noexcept(false)->void;;
		α Write( string data )noexcept->void;;
	};

	template<class TFromServer, class TServerSession>
	TListener<TFromServer,TServerSession>::~TListener()
	{}

#define $ template<class TFromServer, class TServerSession> auto TListener<TFromServer,TServerSession>
	$::Shutdown()noexcept->void
	{
		_shutdown = true;
		shared_lock l{_sessionMutex};
		for_each( _sessions.begin(), _sessions.end(), []( auto& pair ){ static_pointer_cast<TServerSession>(pair.second)->Close();} );
		_acceptor.close();
	}

#undef $
#define $ template<class TFromServer, class TFromClient> auto TSession<TFromServer,TFromClient>
	$::OnRead( const char* p, uint size )noexcept->void
	{
		Try( [&]{ OnRead( IO::Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size) ); } );
	}
	$::Write( TFromServer&& message )noexcept(false)->void
	{
		Write( IO::Proto::ToString(message) );
	}
	$::Write( string data )noexcept->void
	{
		LOG( LogLevel(), "TSession::Write '{}'"sv, data.size() );
		_ws.async_write( boost::asio::buffer((const void*)data.data(), data.size()), [ this, d=move(data) ]( beast::error_code ec, uint bytes_transferred )
		{
			if( ec || d.size()!=bytes_transferred )
			{
				DBGX( "Error writing to Session:  '{}'"sv, boost::diagnostic_information(ec) );
				try
				{
					_ws.close( websocket::close_code::none );
				}
				catch( const boost::exception& e2 )
				{
					DBGX( "Error closing:  '{}'"sv, boost::diagnostic_information(&e2) );
				}
				Disconnect();
			}
		} );
	}

#undef $
#undef var
}