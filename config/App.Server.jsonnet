{
	GoogleAuthClientId:"445012155442-1v8ntaa22konm0boge6hj5mfs15o9lvd.apps.googleusercontent.com",
	http:{
		address: null,
		port: 1967,
		threads: 1,
		timeout: "PT30M",
		maxLogLength: 255,
		accessControl: {
			allowOrigin: "*",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		ssl: {
			_certificate: "{ApplicationDataFolder}/ssl/certs/cert.pem",
			certificateAltName: "DNS:localhost,IP:127.0.0.1",
			_certficateCompany: "Jde-Cpp",
			_certficateCountry: "US",
			_certficateDomain: "localhost",
			_privateKey: "{ApplicationDataFolder}/ssl/private/private.pem",
			_publicKey: "{ApplicationDataFolder}/ssl/public/public.pem",
			_dh: "{ApplicationDataFolder}/certs/dh.pem",
			_passcode: "$(JDE_PASSCODE)"
		}
	},
	dbServers: {
		dataPath: "$(JDE_DIR)/bin/config",
		scriptPath: "$(JDE_DIR)/bin/config/sql/mysql/appServer",
		sync: false,
		localhost:{
			driver: "$(JDE_DIR)/bin/asan/libJde.MySql.so",
			connectionString: "mysqlx://$(JDE_MYSQL_CREDS)@127.0.0.1:33060/jde?ssl-mode=disabled",
			catalogs: {
				jde: {
					schemas:{
						jde:{ //for sqlserver, test with schema, debug with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
							log:{
								meta: "$(JDE_DIR)/AppServer/config/log-meta.jsonnet",
								prefix: "log_"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	},
	logging:{
		defaultLevel: "Information",
		tags: {
			trace:["sql", "parsing", "test", "ql",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read"
			],
			debug:["sessions", "settings"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{}
		},
		file:{ path: "$(ProgramData)/Jde-cpp/AppServer/logs", md: false },
		breakLevel: "Warning"
	},
	workers:{
		drive: {threads: 1},
		alarm: {threads: 1},
		executor: 1
	}
}