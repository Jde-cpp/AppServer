#pragma once
#include "../../Framework/source/threading/Interrupt.h"
#include "../../Framework/source/collections/Queue.h"

namespace Jde
{
	namespace IO
	{
		class IncomingMessage;
		namespace Sockets{ class Client; }
	}
namespace Logging
{
	namespace Messages{struct Message;}
namespace Server
{
	struct Sender : Threading::Interrupt
	{
		static Sender& Create()noexcept(false);
		static Sender& GetInstance()noexcept;
		void OnTimeout()noexcept;
		void OnAwake()noexcept{OnTimeout();}//not expecting...
		//static void OnIncoming( IO::IncomingMessage& returnMessage );
		void Push( sp<const Messages::Message> pMessage ){ _messages.Push(pMessage); }
	private:
		Sender();
		Queue<const Messages::Message> _messages;
		static unique_ptr<Sender> _pSender;
	};
}}}