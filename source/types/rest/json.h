#pragma once

namespace Jde::App{

	Ξ to_json( nlohmann::json& j, const Proto::FromClient::Instance& x )ε->void{
		j = json{ {"application", x.application()}, {"host", x.host()}, {"pid", x.pid()}, {"serverLogLevel", (uint32)x.server_log_level()}, {"clientLogLevel", (uint32)x.client_log_level()}, {"startTime", x.start_time()}, {"webPort", x.web_port()}, {"instanceName", x.instance_name()} };
	}
}
