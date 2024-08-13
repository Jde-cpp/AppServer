drop procedure if exists um_provider_purge;
go

#DELIMITER $$
create procedure um_provider_purge( _provider_id int unsigned )
begin
	delete from um_users where entity_id in ( select id from um_entities where provider_id = _provider_id );
	delete from um_entities where provider_id = _provider_id;
	delete from um_providers where id = _provider_id;
end
#$$
#DELIMITER ;