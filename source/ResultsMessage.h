#pragma once

#define _ Web::FromServer;
#define $ noexcept->Web::FromServer::MessageUnion
#define var const auto
#define SET(x) Web::FromServer::MessageUnion m; m.x( p ); return m
namespace Jde::ApplicationServer
{
	using ClientId=uint32;
	Ξ ToMessage( Web::FromServer::EResults t, int v )${ auto p = new Web::FromServer::MessageValue{}; p->set_type( t ); p->set_int_value( v ); SET(set_allocated_message); }
	Ξ ToMessage( string&& v, ClientId id )${ auto p = new Web::FromServer::StringValue(); p->set_request_id( id ); p->set_value( move(v) ); SET(set_allocated_string_value); }
	Ξ ToMessageQL( string&& v, ClientId id )${ auto p = new Web::FromServer::GraphQL(); p->set_request_id( id ); p->set_json( move(v) ); SET(set_allocated_graph_ql); }
	Ξ ToException( string v, ClientId id )${ auto p = new Jde::Web::FromServer::Exception(); p->set_request_id( id ); p->set_message( move(v) ); SET(set_allocated_exception); }
	Ξ ToComplete( ClientId id )${ auto p = new Jde::Web::FromServer::Complete(); p->set_request_id( id ); SET(set_allocated_complete); }
}
#undef $
#undef SET
#undef _
#undef var