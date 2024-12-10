local types = {
	binary: {type: "Binary"},
	bit: {type:"Bit", length: 1},
	blob: {type: "Blob"},
	char: {type: "Char"},
	cursor: {type: "Cursor"},
	dateTime: {type: "DateTime", length:: 64},
	decimal: {type: "Decimal"},
	float: {type:"Float", length:: 64},
	guid: {type: "Guid", length:: 128},
	image: {type: "Image"},
	int: {type:"Int", length:: 32},
	int8: {type: "Int8", length:: 8},
	int16: {type:"Int16", length:: 16},
	long: {type: "Long", length:: 64},
	money: {type: "Money", length:: 64},
	ntext: {type: "NText"},
	numeric: {type: "Numeric"},
	refCursor: {type: "RefCursor", length:: 64},
	smallDateTime: {type: "SmallDateTime", length:: 32},
	smallFloat: {type:"SmallFloat", length:: 32},
	tchar: {type: "TChar"},
	text: {type: "Text"},
	timeSpan: {type: "TimeSpan", length:: 64},
	uint: {type:"UInt", length:: 32},
	uint8: {type: "UInt8", length:: 8},
	uint16: {type: "UInt16", length:: 16},
	ulong: {type: "ULong", length:: 64},
	uri: {type: "Uri"},
	varbinary: {type: "VarBinary"},
	varchar: {type: "Varchar"},
	varTChar: {type: "VarTChar"},
	varWChar: {type: "VarWChar"},
	wchar: {type: "WChar"},
};
local sqlFunctions = {
	now: { name: "$now" }
};

local smallSequenced = types.uint16+{ sequence: true, sk:0, i:0 };
local pkSequenced = types.uint+{ sequence: true, sk:0, i:0 };
local longSequenced = types.ulong+{ sequence: true, sk:0, i:0 };
local valuesColumns = { name: types.varchar+{ length: 256, i:10 } };

local valuesNK = ["name"];

local targetColumns = valuesColumns+{
	target:types.varchar+{ length: valuesColumns.name.length, i:20 },
	attributes: types.uint16+{ nullable: true, i:30 },
	created: types.dateTime+{ insertable: false, updateable: false, default: sqlFunctions.now.name, i:40 },
	updated: types.dateTime+{ nullable: true, insertable: false, updateable: false, i:50 },
	deleted:types.dateTime+{ nullable: true, insertable: false, updateable: false, i:60 },
	description: types.varchar+{ length: 2048, nullable: true, i:70 }
};

local targetNKs = [valuesNK, ["target"]];
local crc = types.uint+{ i:0 };
local crc64 = types.ulong+{ i:0 };
{
	local tables = self.tables,
	tables:{
		apps:{
			columns: {
				appId: smallSequenced,
				name: valuesColumns.name,
				attributes: targetColumns.attributes
			},
			customInsertProc: true
		},
		appInstances:{
			columns: {
				appInstanceId: pkSequenced,
				appId: tables.apps.columns.appId+{ pkTable: "apps", i:1 },
				endTime: types.dateTime+{ i:2 },
				hostId: tables.hosts.hostId+{ pkTable: "hosts", i:3 },
				pid: types.ulong+{ i:4 },
				startTime: types.dateTime+{ i:5 }
			},
			customInsertProc: true
		},
		sourceFiles:{
			columns: {
				sourceFileId: crc+{sk:0},
				path: valuesColumns.name
			},
			naturalKeys:[["path"]]
		},
		sourceFunctions:{
			columns: {
				sourceFunctionId: crc+{sk:0},
				name: valuesColumns.name
			},
			naturalKeys:[["name"]]
		},
		hosts:{
			hostId: smallSequenced,
			name: valuesColumns.name,
			naturalKeys:[["name"]]
		},
		formats:{
			columns: {
				formatId: crc64+{sk:0},
				text: types.varchar+{ length: 4096, i:1 },
				naturalKeys:[["text"]]
			}
		},
		args:{
			columns: {
				argId: crc64+{sk:0},
				text: types.varchar+{ length: 4096, i:1 },
			},
			naturalKeys:[["text"]]
		},
		logArgs:{
			columns: {
				logId: tables.logs.columns.logId+{ pkTable: "logs", sk:0, i:0 },
				argId: tables.args.columns.argId+{ pkTable: "args", sk:1, i:1 },
				index: types.uint8+{ i:3 }
			},
			map: {parentId:"log_id", childId:"arg_id"}
		},
		logs:{
			columns: {
				logId: longSequenced,
				appInstanceId: tables.appInstances.columns.appInstanceId+{ nullable:true, pkTable: "app_instances", i:1 },
				formatId: tables.formats.columns.formatId+{ pkTable: "formats", nullable: true, i:2 },
				lineNumber: types.uint16+{ nullable:true, i:3 },
				severity: tables.severities.columns.severityId+{ i:4 },
				sourceFileId: tables.sourceFiles.columns.sourceFileId+{ nullable:true, pkTable: "source_files", i:5 },
				sourceFunctionId: tables.sourceFunctions.columns.sourceFunctionId+{ nullable:true, pkTable: "source_functions", i:6 },
				tags: tables.tags.columns.tagId+{nullable:true, i:7 },
				time: types.dateTime+{ i:8 },
				threadId: types.ulong+{ i:9 },
				userId: types.uint+{ i:10 }
			}
		},
		tags:{
			columns: {
				tagId: types.long+{ sk:0, i:0 },
				name: valuesColumns.name
			},
			flagsData: ["None", "Access", "App", "Cache", "Client", "Exception", "ExternalLogger", "Http", "IO",
			"Locks", "Parsing", "Pedantic", "QL", "Read", "Scheduler", "Server", "Sessions",
			"Settings", "Shutdown", "Socket", "Sql", "Startup", "Subscription", "Test", "Threads",
			"Write"]
		},
		severities:{
			columns: {
				severityId: types.uint8+{ i:0 },
				name: types.varchar+{ length: 7, i:1 }
			},
			data: ["Trace", "Debug", "Information", "Warning", "Error", "Critical"]
		}
	}
}