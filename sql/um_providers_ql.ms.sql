create or alter VIEW um_providers_ql AS
	select um_providers.id, name
	from um_providers join um_provider_types on um_providers.provider_type_id = um_provider_types.id
	where um_provider_types.name != 'OpcServer'
	union
	select um_providers.id, target as name
	from um_providers join um_provider_types on um_providers.provider_type_id = um_provider_types.id
	where um_provider_types.name = 'OpcServer';
