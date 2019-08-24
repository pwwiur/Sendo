#ifndef SENDO_LEXICAL_H
#define SENDO_LEXICAL_H
SENDO_NAMESPACE_BEGIN
class Lexical{
private:
    int analyser(Request &request, int i, int level){
        bool first = true;
        bool opened = false;
        bool finished = false;
		while(!checkWhiteSpace(request, i)){
			if(checkIdentifier(request, i, first) || (level == 2 && request.query[i] == '.')){
				request.structure[level] += request.query[i];
                first = false;
            }
            else{
                return -1;
            }
            i++;
        }
        return i;
    }

    bool checkIdentifier(Request &query, int i, bool first = true){
        int ascii = (int) query.query[i];
        if(ascii > 64 && ascii < 91){
            ascii += 32;
            query.query[i] = (char) ascii;
        }
        bool check = (ascii > 96 && ascii < 123) || ascii == 95;
        bool firstCheck = first && check;
        bool etcCheck   = !first && (check || (ascii > 47 && ascii < 68));
        return etcCheck || firstCheck;
    }

    bool checkValue(Request &query, int i){
        int ascii = (int) query.query[i];
        return ascii == 34 || ascii == 39;
    }

    bool checkWhiteSpace(Request &query, int i){
        char x = query.query[i];
        return x == ' ' || x == '\t' || x == '\r' || x == '\n';
    }

public:
    Request parse(std::string requestString){
        Request request(requestString);
        int analyseLevel = 0;
        bool follow;
        bool whitespace = true;
        bool backslash;
        for(int i = 0; i < request.querylen; i++){
            follow = whitespace && !checkWhiteSpace(request, i);
            whitespace = checkWhiteSpace(request, i);
            if(whitespace){
                continue;
            }

            if(follow){
                if(analyseLevel < 3){
                    i = analyser(request, i, analyseLevel++);
                }
                else{
					request.structure[3] = request.query.substr(i, request.querylen);
                    break;
                }
                if(i < 0)
                    break;
                else
                    whitespace = true;
            }
        }

        return request;
    }
};
SENDO_NAMESPACE_END
#endif //SENDO_LEXICAL_H
