//
// Created by pwwiur on 2019-08-21.
//

#ifndef SENDO_CONFIG_H
#define SENDO_CONFIG_H
SENDO_NAMESPACE_BEGIN
class Config {
public:
	static Config *basic;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map;
	Config() {
		rapidjson::Document conf;
		conf.Parse(Gadget::readFile("../dist/config.json").c_str());
		map["path"]["object"] = conf["path"]["object"].GetString();
		map["database"]["name"] = conf["database"]["name"].GetString();
		map["database"]["host"] = conf["database"]["host"].GetString();
		map["database"]["user"] = conf["database"]["username"].GetString();
		map["database"]["password"] = conf["database"]["password"].GetString();
	}
};
Config *Config::basic = new Config();

#ifndef CONF
#define CONF Config::basic->map
#endif

SENDO_NAMESPACE_END
#endif //SENDO_CONFIG_H
