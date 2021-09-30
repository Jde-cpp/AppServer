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
		static ELogLevel LogLevel()noexcept{ return _logLevel; }
		virtual sp<ISession> CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept=0;
	private:
		void DoAccept()noexcept;
		void OnAccept( beast::error_code ec, tcp::socket socket )noexcept;
		atomic<bool> _shutdown{false};
		sp<IOContextThread> _pContextThread;
		static ELogLevel _logLevel;
	protected:
		tcp::acceptor _acceptor;
	};

	template<class TFromServer, class TServerSession>
	struct TListener : WebListener, IShutdown
	{
		TListener( PortType port )noexcept: WebListener{ port }
		{}

		void Push( IO::Sockets::SessionPK sessionId, TFromServer&& m )noexcept;
		sp<ISession> CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept override{ auto p = make_shared<TServerSession>(server, id, move(socket)); p->Run(); return p; }
		auto Shutdown()noexcept->void override;
		sp<TServerSession> Find( IO::Sockets::SessionPK id )noexcept
		{
			shared_lock l{_sessionMutex};
			if( auto p=_sessions.find(id); p!=_sessions.end() )
			{
				sp<ISession> p2 = p->second;
				return static_pointer_cast<TServerSession>( p2 );
			}
			else
				return {};
		}
	protected:
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;

	};

	struct Session : IO::Sockets::ISession, std::enable_shared_from_this<Session>
	{
		Session( WebListener& server, SessionPK id, tcp::socket&& socket ):ISession{id}, _ws{std::move(socket)}, _server{server}{}
		virtual α Close()noexcept->void{};
		virtual α Run()noexcept->void;
	protected:
		α Disconnect()noexcept{ /*_connected = false;*/ _server.RemoveSession( Id ); }

		websocket::stream<beast::tcp_stream> _ws;
		WebListener& _server;
	private:
		α OnRun()noexcept->void;
		virtual α OnAccept( beast::error_code ec )noexcept->void;
		α DoRead()noexcept->void;
		α OnNetRead( beast::error_code ec, std::size_t bytes_transferred )noexcept->void;
		virtual α OnRead( const char* p, uint size )noexcept->void=0;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )noexcept->void;

		beast::flat_buffer _buffer;
	};
	template<class TFromServer, class TFromClient>
	struct TSession : Session//, public std::enable_shared_from_this<TSession<TFromServer,TFromClient>>
	{
		TSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept(false):
			Session{ server, id, move(socket) }
		{}


		void OnRead( const char* p, uint size )noexcept;
		virtual void OnRead( TFromClient transmission )noexcept=0;
		void Write( TFromServer&& message )noexcept(false);
		void Write( string data )noexcept;

	};

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
		Try( [&]{ OnRead( IO::Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, size) ); } );
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