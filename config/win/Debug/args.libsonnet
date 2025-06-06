{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/apps/AppServer/bin/$(JDE_BUILD_TYPE)/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=jde",
	catalogs: {
		jde: {
			schemas:{
				dbo:{
					access:{  //test debug with schema, debug with default schema ie dbo.
						meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
						ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
						prefix: "access_"  //test with null prefix, debug with prefix
					},
					log:{
						meta: "$(JDE_DIR)/AppServer/config/log-meta.jsonnet",
						prefix: "log_"  //test with null prefix, debug with prefix
					},
				}
			}
		}
	}
}