//
// Created by pwwiur on 2019-08-20.
//

#ifndef SENDO_GADGET_H
#define SENDO_GADGET_H

#include <string>
#include <unordered_map>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <fstream>

#include "../include/rapidjson/stringbuffer.h"
#include "../include/rapidjson/writer.h"

#include "../include/rapidjson/document.h"

SENDO_NAMESPACE_BEGIN
class Gadget {
public:
	static void ltrim(std::string& str, char c = ' ') {
		int n = 0;
		for (int i = 0; i < str.length(); i++) {
			if (str[i] == c){
				n++;
			} else {
				break;
			}
		}
		str = str.substr(n);
	}
	static void rtrim(std::string& str, char c = ' ') {
		int n = 0;
		for (int i = str.length() - 1; i > 0; i--) {
			if (str[i] == c){
				n++;
			} else {
				break;
			}
		}
		str = str.substr(0, str.length() - n);
	}
	static void trim(std::string& str, char c = ' ') {
		ltrim(str, c);
		rtrim(str, c);
	}
	static void toupper(std::string& str) {
		for (int i = 0; i < str.length(); i++) {
			if(str[i] > 96 && str[i] < 123) {
				str[i] = str[i] - 32;
			}
		}
	}
	static void tolower(std::string& str) {
		for (int i = 0; i < str.length(); i++) {
			if(str[i] > 64 && str[i] < 91) {
				str[i] = str[i] + 32;
			}
		}
	}
	static void str_replace(std::string& str, std::string from, std::string to) {
		int pos;
		do {
			pos = str.find(from);
			if(pos != std::string::npos)
				str.replace(pos, from.length(), to);
		} while(pos != std::string::npos);
	}
	static std::string readFile(std::string filePath) {
		char* buffer;
		FILE* fp = fopen(filePath.c_str(), "r");
		if (fp != NULL){
			fseek (fp, 0, SEEK_END);
			long length = ftell (fp);
			fseek (fp, 0, SEEK_SET);
			buffer = (char*) malloc(length + 1);
			fread(buffer, 1, length, fp);
			buffer[length] = '\0';
			fclose(fp);
			return buffer;
		}
		throw std::runtime_error("File not found");
	}
	static void editFile(std::string filePath, std::string body) {
		std::ofstream file(filePath.c_str());
		logger(filePath);
		file << body;
		file.close();

	}
	static std::string enforce(std::string enforcer, std::string path, std::string args) {
		char buffer[128];
		std::string result = "";
		std::string cmd = "python " + enforcer + " " + path + " " + args;
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe) throw std::runtime_error("popen() failed!");
		try {
			while (fgets(buffer, sizeof buffer, pipe) != NULL) {
				result += buffer;
			}
		} catch (...) {
			pclose(pipe);
			throw;
		}
		pclose(pipe);
		return result;
	}
	static void logger(const char* txt) {
		std::cout << txt << std::endl;
	}
	static void logger(std::string txt) {
		std::cout << txt << std::endl;
	}
	template <class T> static void logger(T txt) {
		logger(std::to_string(txt));
	}
};

SENDO_NAMESPACE_END
#endif //SENDO_GADGET_H
