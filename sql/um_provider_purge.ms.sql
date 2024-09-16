create or alter proc um_provider_purge( @provider_id int ) as
begin
	delete from um_users where entity_id in ( select id from um_entities where provider_id = @provider_id );
	delete from um_entities where provider_id = @provider_id;
	delete from um_providers where id = @provider_id;
end