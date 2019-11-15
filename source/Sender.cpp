#include "stdafx.h"
#include "Sender.h"

namespace Jde::Logging::Server
{
	unique_ptr<Sender> Sender::_pSender{nullptr};
	Sender::Sender():
		Interrupt( "Sender", 1s )
	{}
	
	Sender& Sender::Create()noexcept(false)
	{
		ASSRT_TR( !_pSender );
		_pSender = unique_ptr<Sender>( new Sender() );
		return *_pSender;
	}

	Sender& Sender::GetInstance()noexcept
	{
		ASSRT_TR( _pSender );
		return *_pSender;
	}

	void Sender::OnTimeout()noexcept
	{
		//std::function<void(sp<const Messages::Message>&)> function = []( sp<const Messages::Message>& message )
		//{

		//};
		//_messages.ForEach( function );
		//Collections::Queue<Messages::Message> _messages;
		
	}
}
