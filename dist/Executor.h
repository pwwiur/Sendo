#ifndef SENDO_EXECUTOR_H
#define SENDO_EXECUTOR_H

#ifndef SENDO_NAMESPACE
#define SENDO_NAMESPACE sendo
#endif
#ifndef SENDO_NAMESPACE_BEGIN
#define SENDO_NAMESPACE_BEGIN namespace SENDO_NAMESPACE {
#endif
#ifndef SENDO_NAMESPACE_END
#define SENDO_NAMESPACE_END }
#endif

#include "Gadget.h"
#include "Config.h"
#include "SendoException.h"
#include "Request.h"
#include "Lexical.h"
#include "Semantic.h"

SENDO_NAMESPACE_BEGIN
class Executor {
public:
	Executor() {
		driver = get_driver_instance();
		con = driver->connect(CONF["database"]["host"], CONF["database"]["username"], CONF["database"]["password"]);
		con->setSchema(CONF["database"]["name"]);
	}
	Request query(std::string requestString) {
		Request request = lexicalAnalyzer.parse(requestString);
		if(semanticAnalyzer.analyze(request)){
			return request;
		}
		else{
			throw new SendoException(0);
		}
	}
	Request execute(std::string queryString){
		Request request = query(queryString);
		return execute(request);
	}
	Request execute(Request& request) {
		try {
			if (request.structure[0] == "SEND") {
				return executeSend(request);
			} else if (request.structure[0] == "GET") {
				return executeGet(request);
			} else if (request.structure[0] == "DO") {
				return executeDo(request);
			} else {
				throw SendoException(3);
			}
		}
		catch (SendoException &e) {
			return Request("GET STATUS " + std::to_string(e.getCode()));
		}
		catch (sql::SQLException e) {
			Gadget::logger(e.what());
			return Request("GET STATUS 2");
		}
		catch (...) {
			return Request("GET STATUS 2");
		}
    }
	~Executor() {
		delete con;
	}

private:
	Lexical lexicalAnalyzer;
	Semantic semanticAnalyzer;

	sql::Driver *driver;
	sql::Connection *con;

	int relator(std::string relationship, bool forgive = false) {
		int dotpos, parent = 0;
		std::string child;
		sql::ResultSet *res;
		sql::PreparedStatement *prpstmt;
		do {
			dotpos = relationship.find(".");
			child = relationship.substr(0, dotpos == std::string::npos ? relationship.length() : dotpos);
			relationship.erase(0, dotpos == std::string::npos ? relationship.length() : dotpos + 1); // pos + length of .
			prpstmt = con->prepareStatement("SELECT id FROM object t WHERE t.title = ? AND t.object = ?");
			prpstmt->setString(1, child);
			prpstmt->setInt(2, parent);
			res = prpstmt->executeQuery();
			if(res->next()) {
				parent = res->getInt("id");
			} else {
				return forgive && relationship.length() == 0 ? parent : 0;
			}
		} while (relationship.length() > 0);
		return parent;
	}

	Request executeSend(Request& request) {
		if(request.structure[1] == "OBJECT"){
			return sendObject(request);
		}
		else if(request.structure[1] == "DATA"){
			return sendData(request);

		}
		else if(request.structure[1] == "SKILL"){
			return sendSkill(request);
		}
		else {
			throw new SendoException(4);
		}
	}
	Request sendObject(Request& request){
		Request response;
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;
		rapidjson::StringBuffer buff;
		rapidjson::Writer<rapidjson::StringBuffer> write(buff);
		std::string property = request.structure[2];
		int parent = relator(property);
		if(parent > 0) {
			prpstmt = con->prepareStatement("SELECT title FROM object t WHERE object = ?");
			prpstmt->setInt(1, parent);
			res = prpstmt->executeQuery();
			response.setQueryString("GET OBJECT " + (request.structure[2]));
			if(res->rowsCount() > 0) {
				response.setProperty("BODY-TYPE", "json");
				write.StartArray();
				while(res->next()) {
					write.StartObject();
					write.Key("title");
					write.String(res->getString("title").c_str());
					write.EndObject();
				}
				write.EndArray();
				response.setBody(buff.GetString());
			}

			return response;
		}
		throw SendoException(5);
	}
	Request sendData(Request& request) {
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;
		rapidjson::StringBuffer buff;
		rapidjson::Writer<rapidjson::StringBuffer> write(buff);
		std::string property = request.structure[2];
		int ceperator = property.find_last_of('.');
		if(ceperator != std::string::npos) {
			int parent = relator(property.substr(0, ceperator));
			std::string key = property.substr(ceperator + 1, property.length());
			prpstmt = con->prepareStatement("SELECT t.value, COALESCE(tt.title, \"\") AS type FROM data t LEFT JOIN data_type tt ON t.type = tt.id WHERE t.title = ? AND t.object = ?");
			prpstmt->setString(1, key);
			prpstmt->setInt(2, parent);
			res = prpstmt->executeQuery();
			if(res->rowsCount() > 0) {
				Request response;
				response.setQueryString("GET DATA " + request.structure[2]);
				response.setProperty("BODY-TYPE", "json");
				write.StartArray();
				while(res->next()) {
					write.StartObject();
					write.Key("type");
					write.String(res->getString("type").c_str());
					write.Key("value");
					write.String(res->getString("value").c_str());
					write.EndObject();
				}
				write.EndArray();
				response.setBody(buff.GetString());
				return response;
			}
		}
		throw SendoException(5);
	}
	Request sendSkill(Request& request) {
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;
		rapidjson::StringBuffer buff;
		rapidjson::Writer<rapidjson::StringBuffer> write(buff);
		std::string property = request.structure[2];
		int ceperator = property.find_last_of('.');
		if(ceperator != std::string::npos) {
			std::string parent = property.substr(0, ceperator);
			std::string skill = property.substr(ceperator + 1, property.length());
			int parentID = relator(parent);
			prpstmt = con->prepareStatement("SELECT t.enforcer FROM skill t WHERE t.title = ? AND t.object = ?");
			prpstmt->setString(1, skill);
			prpstmt->setInt(2, parentID);
			res = prpstmt->executeQuery();
			if (res->next()) {
				Request response;
				std::string enforcer = res->getString("enforcer");
				response.setQueryString("GET SKILL " + property);
				Gadget::str_replace(parent, ".", "/");
				std::string filePath = CONF["path"]["object"] + parent + "/" + skill + "." + enforcer;
				response.setProperty("SKILL-ENFORCER", enforcer);
				response.setBody(Gadget::readFile(filePath));

				return response;
			}
		}
		throw SendoException(5);
	}

	Request executeGet(Request &request) {
		if(request.structure[1] == "OBJECT"){
			return getObject(request);
		}
		else if(request.structure[1] == "DATA"){
			return getData(request);

		}
		else if(request.structure[1] == "SKILL"){
			return getSkill(request);
		}
		else {
			throw new SendoException(4);
		}

	}
	Request getObject(Request& request){
		int parent = 0;
		std::string property = request.structure[2];
		Gadget::trim(property, '.');
		bool absolute = property.find('.') == std::string::npos;
		int propsize = request.structure[2].length();
		bool success = false;
		bool hasobject = false;
		std::string title;
		sql::Statement *stmt;
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;

		if(request.hasProperty("BODY-SIZE")){
			int bodySize = std::stoi(request.getProperty("BODY-SIZE"));
			if(bodySize > 0) {
				std::string bodyType = request.getProperty("BODY-TYPE");
				if(bodyType == "json") {
					if(propsize > 0) {
						parent = relator(request.structure[2]);
					}
					if(propsize == 0 || (propsize > 0 && parent > 0)) {
						rapidjson::Document doc;
						doc.Parse(request.body.c_str());
						if(doc.HasParseError()) {
							throw new SendoException(0);
						}
						for (auto& v : doc.GetArray()){
							if(v.HasMember("title")){
								title =  v["title"].GetString();
								std::string query = "SELECT EXISTS(SELECT 1 FROM object t WHERE t.title = ? AND t.object = ?) as has";
								prpstmt = con->prepareStatement(query);
								prpstmt->setString(1, title);
								prpstmt->setInt(2, parent);
								res = prpstmt->executeQuery();
								res->next();
								if(res->getInt("has") == 0) {
									prpstmt = con->prepareStatement("INSERT INTO object(title, object) VALUES(?, ?)");
									prpstmt->setString(1, title);
									prpstmt->setInt(2, parent);
									int result = prpstmt->executeUpdate();
								}
							}
						}
						return Request("GET STATUS 1");
					}
				}
			}
		}
		else {
			if(propsize > 0) {
				parent = relator(request.structure[2], true);
				int itself = relator(request.structure[2]);
				if(propsize > 0 && (absolute || parent > 0) && itself < 1) {
					int lastdot = request.structure[2].find_last_of(".");
					title = request.structure[2].substr(lastdot == std::string::npos ? 0 : lastdot + 1, request.structure[2].length());
					prpstmt = con->prepareStatement("SELECT EXISTS(SELECT 1 FROM object t WHERE t.title = ? AND t.object = ?) has");
					prpstmt->setString(1, title);
					prpstmt->setInt(2, parent);
					res = prpstmt->executeQuery();
					res->next();
					if(res->getInt("has") == 0) {
						prpstmt = con->prepareStatement("INSERT INTO object(title, object) VALUES(?, ?)");
						prpstmt->setString(1, title);
						prpstmt->setInt(2, parent);
						int result = prpstmt->executeUpdate();
					}
					return Request("GET STATUS 1");
				}

			}
		}
		throw new SendoException(6);
	}
	Request getData(Request& request){
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;
		std::string property = request.structure[2];
		int ceperator = property.find_last_of('.');
		if(ceperator != std::string::npos) {
			int parent = relator(property.substr(0, ceperator));
			std::string key = property.substr(ceperator + 1, property.length());
			std::string value;
			if(request.structure[3].length() > 0) {
				value = request.structure[3];
			}
			else {
				if(request.hasBody()) {
					if(request.getProperty("BODY-TYPE") == "json") {
						rapidjson::Document doc;
						doc.Parse(request.getBody().c_str());
						if(!doc.HasParseError()) {
							value = doc["value"].GetString();
						}
					}
				}
			}
			if(value.length() > 0) {
				prpstmt = con->prepareStatement("SELECT id FROM data t WHERE title = ? AND object = ?");
				prpstmt->setString(1, key);
				prpstmt->setInt(2, parent);
				res = prpstmt->executeQuery();
				if(res->next()) {
					prpstmt = con->prepareStatement("UPDATE data SET value = ? WHERE id = ?");
					prpstmt->setString(1, value);
					prpstmt->setInt(2, res->getInt("id"));
					res = prpstmt->executeQuery();
				}
				else {
					prpstmt = con->prepareStatement("INSERT INTO data(title, object, value) VALUES(?, ?, ?)");
					prpstmt->setString(1, key);
					prpstmt->setInt(2, parent);
					prpstmt->setString(3, value);
					res = prpstmt->executeQuery();
				}
				return Request("GET STATUS 1");
			}
		}
		throw SendoException(6);
	}
	Request getSkill(Request& request) {
		sql::PreparedStatement *prpstmt;
		sql::ResultSet *res;
		std::string property = request.structure[2];
		int ceperator = property.find_last_of('.');
		if (ceperator != std::string::npos) {
			std::string parent = property.substr(0, ceperator);
			int parentID = relator(parent);
			std::string skill = property.substr(ceperator + 1, property.length());
			if (request.hasBody()) {
				if (request.hasProperty("SKILL-ENFORCER")) {
					std::string enforcer = request.getProperty("SKILL-ENFORCER");
					prpstmt = con->prepareStatement("SELECT id FROM skill t WHERE title = ? AND object = ?");
					prpstmt->setString(1, skill);
					prpstmt->setInt(2, parentID);
					res = prpstmt->executeQuery();
					if (!res->next()) {
						prpstmt = con->prepareStatement("INSERT INTO skill(title, object, enforcer) VALUES(?, ?, ?)");
						prpstmt->setString(1, skill);
						prpstmt->setInt(2, parentID);
						prpstmt->setString(3, enforcer);
						res = prpstmt->executeQuery();
					}
					Gadget::str_replace(parent, ".", "/");
					std::string path = CONF["path"]["object"] + parent + "/" + skill + "." + enforcer;
					Gadget::editFile(path, request.getBody());
					return Request("GET STATUS 1");
				}
			}
		}
		throw SendoException(6);
	}

	Request executeDo(Request &request) {
		std::string property = request.structure[2];
		int ceperator = property.find_last_of('.');
		if(ceperator != std::string::npos) {
			std::string parent = property.substr(0, ceperator);
			std::string skill = property.substr(ceperator + 1);
			int parentID = relator(parent);
			if(parentID > 0) {
				sql::PreparedStatement *preparedStatement = con->prepareStatement("SELECT title, enforcer FROM skill t WHERE title = ? AND object = ?");
				preparedStatement->setString(1, skill);
				preparedStatement->setInt(2, parentID);
				sql::ResultSet *res = preparedStatement->executeQuery();
				if(res->next()) {
					std::string skill = res->getString("title");
					std::string enforcer = res->getString("enforcer");
					Gadget::str_replace(parent, ".", "/");
					std::string path = CONF["path"]["object"] + parent + "/" + skill + "." + enforcer;
					std::string enforcerPath = CONF["path"]["object"] + "root/enforcer/" + enforcer + ".py";
					std::string result = Gadget::enforce(enforcerPath, path, request.structure[3]);
					Request response;
					response.setQueryString("GET RESULT " + property);
					response.setProperty("BODY-TYPE", "plainText");
					response.setProperty("BODY-SIZE", std::to_string(result.length()));
					response.setBody(result);
					return response;
				}
			}
		}
		throw SendoException(7);
	}
};
SENDO_NAMESPACE_END
#endif //SENDO_EXECUTOR_H
