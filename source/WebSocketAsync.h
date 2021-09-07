#pragma once
#pragma region Defines
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <jde/App.h>
#include "../../Framework/source/io/sockets/Socket.h"
#include "../../Framework/source/io/ProtoUtilities.h"
#include "../../Framework/source/collections/UnorderedMap.h"

namespace Jde::WebSocket
{
#define var const auto
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	//using SessionPK=IO::Sockets::SessionPK;
	using namespace Jde::IO::Sockets;
#pragma endregion
	// struct ISession
	// {};

	struct WebListener : IO::Sockets::IServerSocket, std::enable_shared_from_this<WebListener>
	{
		WebListener( PortType port, net::io_context& ioc )noexcept(false);
		static ELogLevel LogLevel()noexcept{ return _logLevel; }
		virtual sp<ISession> CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept=0;
	protected:
		tcp::acceptor _acceptor;
	private:
		void DoAccept()noexcept;
		void OnAccept( beast::error_code ec, tcp::socket socket )noexcept;
		atomic<bool> _shutdown{false};

		net::io_context& _ioc;
		static ELogLevel _logLevel;
	};

	template<class TFromServer, class TServerSession>
	struct TListener : WebListener, IShutdown
	{
		TListener( PortType port )noexcept: WebListener{ port, IOContextThread::GetContext() }
		{}

		void Push( IO::Sockets::SessionPK sessionId, TFromServer&& m )noexcept;
		sp<ISession> CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept override{ return make_shared<TServerSession>(server, id, move(socket)); }
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
		//Collections::UnorderedMap<SessionPK,TServerSession> _sessions;
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;
	private:
		//void Accept()noexcept override;
	};

	struct Session : IO::Sockets::ISession, std::enable_shared_from_this<Session>
	{
		Session( WebListener& server, SessionPK id, tcp::socket&& socket ):ISession{id}, _ws{std::move(socket)}, _server{server}{ Run(); }
		virtual void Close()noexcept{};
	protected:
		void Disconnect()noexcept{ /*_connected = false;*/ _server.RemoveSession( Id ); }

		websocket::stream<beast::tcp_stream> _ws;
		WebListener& _server;
	private:
		void Run()noexcept;
		void OnRun()noexcept;
		void OnAccept( beast::error_code ec )noexcept;
		void DoRead()noexcept;
		void OnNetRead( beast::error_code ec, std::size_t bytes_transferred )noexcept;
		virtual void OnRead( const char* p, uint size )noexcept=0;
		void OnWrite( beast::error_code ec, std::size_t bytes_transferred )noexcept;

		beast::flat_buffer _buffer;
	};
	template<class TFromServer, class TFromClient>
	struct TSession : Session//, public std::enable_shared_from_this<TSession<TFromServer,TFromClient>>
	{
		TSession( WebListener& server, SessionPK id, tcp::socket&& socket )noexcept(false):
			Session{ server, id, move(socket) }
		{}
		//virtual ~TSession()=default;

		void OnRead( const char* p, uint size )noexcept;
		virtual void OnRead( TFromClient transmission )noexcept=0;
		void Write( TFromServer&& message )noexcept(false);
		void Write( string data )noexcept;
		//void Read( sp<TFromClient> pTransmission )noexcept override;
	//private:
	//	void Run()noexcept override;
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
		LOG( LogLevel(), "WebSocket::Write '{}'"sv, data.size() );
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
			}
			Disconnect();
		} );
	}

#undef $
#undef var
}