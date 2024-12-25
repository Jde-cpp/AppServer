local common = import 'common-meta.libsonnet';

{
	local tables = self.tables,
	tables:{
		apps:{
			columns: {
				appId: common.smallSequenced,
				name: common.valuesColumns.name,
				attributes: common.targetColumns.attributes
			},
			customInsertProc: true
		},
		appInstances:{
			columns: {
				appInstanceId: common.pkSequenced,
				appId: tables.apps.columns.appId+{ pkTable: "apps", i:1 },
				endTime: common.types.dateTime+{ i:2 },
				hostId: tables.hosts.hostId+{ pkTable: "hosts", i:3 },
				pid: common.types.ulong+{ i:4 },
				startTime: common.types.dateTime+{ i:5 }
			},
			customInsertProc: true
		},
		hosts:{
			hostId: common.smallSequenced,
			name: common.valuesColumns.name,
			naturalKeys:[["name"]]
		},
	}
}