//
// Created by pwwiur on 23/06/19.
//

#ifndef SENDO_REQUEST_H
#define SENDO_REQUEST_H
SENDO_NAMESPACE_BEGIN
class Request {
public:
	int querylen;
	std::string request;
	std::string query;
	std::string body;
	std::unordered_map<std::string, std::string> properties;
    std::string structure[4];

    Request() {}
	Request(std::string request) : request(request) {
		parse(request);
	}
	void parse(std::string request) {
		int pos;
		bool qr = true;
		std::string token;
		do {
			pos = request.find("\r\n");
			token = request.substr(0, pos == std::string::npos ? request.length() : pos);
			request.erase(0, pos == std::string::npos ? request.length() : pos + 2); // pos + length of \r\n
			if(qr) {
				setQueryString(token);
				qr = false;
			} else {
				if(pos > 0) {
					int tpos = token.find_first_of(":");
					std::string key = token.substr(0, tpos);
					std::string value = token.substr(tpos + 1, token.length());
					Gadget::trim(key);
					Gadget::toupper(key);
					Gadget::trim(value);
					setProperty(key, value);
				} else {
					break;
				}
			}

		} while (pos != std::string::npos);
		if(request.length() > 0){
			setBody(request);
		}
	}
	std::string serialize() {
		std::string str = query;
		if(properties.size() > 0){
			str.append("\r\n");
			for(auto const& [key, value] : properties) {
				str.append(key + ": " + value);
				str.append("\r\n");
			}
		}
		if(body.length() > 0) {
			str.append("\r\n");
			str.append(body);
		}
		return str;
    }
	void setQueryString(std::string queryString) {
    	this->query = queryString;
    	this->querylen = queryString.length();
    }
	void setBody(std::string body) {
		this->body = body;
		setProperty("BODY-SIZE", std::to_string(body.size()));
	}
	void setProperties(std::unordered_map<std::string, std::string> properties) {
    	this->properties = properties;
    }
    void setProperty(std::string key, std::string value) {
		properties[key] = value;
	}
	std::string getProperty(std::string key) {
		return properties[key];
	}
	std::string getBody() {
		return body;
    }
	bool hasProperty(std::string key) {
		return properties.size() != 0 && properties.find(key) != properties.end();
	}
	bool hasBody() {
		return hasProperty("BODY-SIZE") && std::stoi(getProperty("BODY-SIZE")) > 0 && body.length() > 0;
    }
};
SENDO_NAMESPACE_END
#endif //SENDO_REQUEST_H
