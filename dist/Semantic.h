#ifndef SENDO_SEMANTIC_H
#define SENDO_SEMANTIC_H

SENDO_NAMESPACE_BEGIN
class Semantic{
public:
    bool analyze(Request &request){
		Gadget::toupper(request.structure[0]);
		for(int i = 0; i < tokens->length(); i++){
			if(tokens[i] == request.structure[0]){
				if(i < 2){
					Gadget::toupper(request.structure[1]);
					for (int j = 0; j < j < ontologies->length(); ++j) {
						if(ontologies[j] == request.structure[1]){
							return true;
						}
					}
					throw new SendoException("Unknown untology " + request.structure[1], 1002);
				}
				return true;
			}
		}
		throw new SendoException("Unknown command " + request.structure[0], 1001);
    }

private:
    std::string tokens[3] = {
         "SEND",
         "GET",
         "DO"
    };

    std::string ontologies[3] = {
    		"OBJECT",
    		"DATA",
    		"SKILL"
    };
};

SENDO_NAMESPACE_END
#endif //SENDO_SEMANTIC_H
