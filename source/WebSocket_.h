#pragma once
/*
//#include "../framework/threading/Interrupt.h"
// #include "../framework/collections/Queue.h"
#include "types/proto/FromServer.pb.h"
#include "types/proto/FromClient.pb.h"
//#include <list>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/exception/diagnostic_information.hpp>
//https://www.boost.org/doc/libs/1_71_0/libs/beast/example/websocket/server/sync/websocket_server_sync.cpp
#include "../../Framework/source/collections/UnorderedMap.h"
#include "../../Framework/source/threading/Thread.h"
#include "../../Framework/source/threading/InterruptibleThread.h"
#include "../../Framework/source/io/sockets/Socket.h"
//------------------------------------------------------------------------------
namespace boost::asio::ip{ class tcp; }
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde::WebSocket
{
	struct Server;
	using SessionPK=IO::Sockets::SessionPK;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	struct Session
	{
		Session( Server& server, SessionPK id, tcp::socket&& socket )noexcept(false);//boost::system::system_error
		virtual ~Session()=default;

		//virtual void OnConnect()noexcept=0;
		const SessionPK Id;
		void Close()noexcept;
		static VectorPtr<google::protobuf::uint8> TryToBuffer( const google::protobuf::MessageLite& msg )noexcept;
	protected:
		virtual void Start()noexcept;
		void Disconnect()noexcept;
		typedef boost::beast::websocket::stream<tcp::socket> Stream;
		Stream _stream;
		sp<Threading::InterruptibleThread> _pThread;
		Server& _server;
	private:
		virtual void Run()noexcept=0;
		void SendAck()noexcept;
		std::atomic<bool> _connected{true};
	};

	template<typename TFromServer, typename TFromClient>
	struct TSession : Session, public std::enable_shared_from_this<TSession<TFromServer,TFromClient>>
	{
		TSession( Server& server, SessionPK id, tcp::socket&& socket )noexcept(false):
			Session{ server, id, move(socket) }
		{}
		virtual ~TSession()=default;
		void Write( const TFromServer& message )noexcept(false);
		void Write2( const vector<google::protobuf::uint8>& data )noexcept(false);
		virtual void OnRead( sp<TFromClient> pTransmission )noexcept=0;
	private:
		void Run()noexcept override;
	};
#define var const auto
	template<typename TFromServer, typename TFromClient>
	void TSession<TFromServer,TFromClient>::Write( const TFromServer& message )noexcept(false)
	{
		if( var p = TryToBuffer( message ); p )
			Write( *p );
	}
	template<typename TFromServer, typename TFromClient>
	void TSession<TFromServer,TFromClient>::Write2( const vector<google::protobuf::uint8>& data )noexcept(false)
	{
		try
		{
			TRACE( "WebSocket::Write '{}'"sv, data.size() );
			_stream.write( boost::asio::buffer(data.data(), data.size()) );
		}
		catch( boost::exception& e )
		{
			DBGX( "Error writing to Session:  '{}'"sv, boost::diagnostic_information(&e) );
			try
			{
				_stream.close( boost::beast::websocket::close_code::none );
			}
			catch( const boost::exception& e2 )
			{
				DBGX( "Error closing:  '{}'"sv, boost::diagnostic_information(&e2) );
			}
			Disconnect();
		}
	}

	template<typename TFromServer, typename TFromClient>
	void TSession<TFromServer,TFromClient>::Run()noexcept
	{
		//who calls this, should add thread at some point to application.  crashes if close on transmission.
		var pKeepAlive = this->shared_from_this();
		Threading::SetThreadDscrptn( fmt::format("Session( '{}' )", Id) );

		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			boost::beast::multi_buffer buffer;
			try
			{
				_stream.read( buffer );
				var str = boost::beast::buffers_to_string( buffer.data() );
				google::protobuf::io::CodedInputStream input( (const uint8*)str.data(), (int)str.size() );
				//IO::FileUtilities::Save( "/tmp/request.dat", str );

				auto pTransmission = make_shared<TFromClient>();
				if( !pTransmission->MergePartialFromCodedStream(&input) )
					ERR( "MergePartialFromCodedStream returned false, length={}"sv, str.size() );
				OnRead( pTransmission );
			}
			catch(boost::system::system_error const& se)
			{
				auto code = se.code();
				if( code == boost::beast::websocket::error::closed )
					DBG( "se.code()==websocket::error::closed"sv );
				else if( code.value()==104 )//reset by peer
				{
					DBG( "'{}' - closing '{}'"sv, se.code().message(), Id );
					Disconnect();
					break;
				}
				else
				{
					Disconnect();
					DBG( "system_error returned: '{}' - closing connection - {}"sv, se.code().message(), Id );
					//pSession->close( boost::beast::websocket::close_code::try_again_later, code );
					break;
				}
			}
			catch( std::exception const& e )
			{
				ERR( "std::exception returned: '{}'"sv, e.what() );
			}
		}
		IApplication::RemoveThread( _pThread );
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Server : std::enable_shared_from_this<Server>
	{
		Server( PortType port )noexcept;
		void StartAccepting()noexcept;
		virtual uint SessionCount()const noexcept=0;//{ return _sessions.size(); }
		virtual void RemoveSession( SessionPK id )noexcept=0;
	protected:
		PortType _port;
		std::atomic<SessionPK> _id{0};
	private:
		virtual void Accept()=0;
		//std::atomic<uint> _sessionId{0};
		shared_ptr<Threading::InterruptibleThread> _pAcceptor;
	};

	template<typename TFromServer, typename TFromClient, typename TServerSession>
	struct TServer : Server, public IShutdown
	{
		TServer( PortType port )noexcept:
			Server{port}
		{
			StartAccepting();
		}
		void Push( SessionPK sessionId, sp<TFromServer>& pItem )noexcept;
		uint SessionCount()const noexcept override{ return _sessions.size(); }
		void RemoveSession( SessionPK id )noexcept override;
		void Shutdown()noexcept override;
		sp<TServerSession> Find( SessionPK id )noexcept{ return _sessions.Find( id ); }
	protected:
		Collections::UnorderedMap<SessionPK,TServerSession> _sessions;
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;
	private:
		void Accept()noexcept override;
	};

	template<typename TFromServer, typename TFromClient, typename TServerSession>
	void TServer<TFromServer,TFromClient,TServerSession>::Push( uint sessionId, sp<TFromServer>& pItem )noexcept
	{
		auto pSession = _sessions.Find( sessionId );
		if( pSession )
			pSession->Push( pItem );
		else
			WARN( "Could not find session for {}", sessionId );
	}

	template<typename TFromServer, typename TFromClient, typename TServerSession>
	void TServer<TFromServer,TFromClient,TServerSession>::Shutdown()noexcept
	{
		_shutdown = true;
		std::function<void(const SessionPK&, TServerSession&)> fncn = []( const SessionPK&, TServerSession& session ){ session.Close(); };
		_sessions.ForEach( fncn );
		if( _pAcceptor )
			_pAcceptor->close();
	}
	template<typename TFromServer, typename TFromClient, typename TServerSession>
	void TServer<TFromServer,TFromClient,TServerSession>::RemoveSession( SessionPK id )noexcept
	{
		_sessions.erase(id);
		TRACE( "Removed session '{}'"sv, id );
	}

	template<typename TFromServer, typename TFromClient, typename TServerSession>
	void TServer<TFromServer,TFromClient,TServerSession>::Accept()noexcept
	{
		Threading::SetThreadDscrptn( "WebListener" );
		boost::asio::io_context ioc{1};
		try
		{
			_pAcceptor = sp<tcp::acceptor>( new tcp::acceptor{ioc, {tcp::v4(), (short unsigned int)_port}} );
		}
		catch( const boost::system::system_error& e )
		{
			LOGS( ELogLevel::Critical, e.code().message() );
			return;
		}
		//tcp::acceptor acceptor{ ioc, {tcp::v4(), (short unsigned int)_port} };
		INFO( "Accepting web sockets on port {}."sv, _port );
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			tcp::socket socket{ioc};
			try
			{
				_pAcceptor->accept( socket );// Blocking
				TRACE( "Accepted Connection."sv );
				var id = ++_id;
				sp<TFromServer> pServer = dynamic_pointer_cast<TFromServer>( shared_from_this() );
				auto pSession = make_shared<TServerSession>( pServer, id, move(socket) );//deadlock if included in _sessions.emplace
				pSession->Start();
				_sessions.emplace( id, pSession );
			}
			catch( const boost::system::system_error& e )
			{
				LOGS( ELogLevel::Debug, e.code().message() );
			}
		}
	}
}
namespace Jde::ApplicationServer::Web
{
	//typedef FromServer::Transmission MyFromServer;
	//typedef FromClient::Transmission MyFromClient;
	struct WebServer;
}
#undef var
*/