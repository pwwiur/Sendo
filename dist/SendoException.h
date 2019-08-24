#ifndef SENDO_SENDOEXCEPTION_H
#define SENDO_SENDOEXCEPTION_H

#include <exception>
SENDO_NAMESPACE_BEGIN

/*
 * Error codes {
 * 		0: Unknown error
 * 		1: Successful execution
 * 		2: Runtime error in the machine
 * 		3: Unknown request
 * 		4: Unknown property
 * 		5: Nothing to response
 * 		6: Incorrect data
 * 		7: Nothing to do
 *
 */
class SendoException : public std::exception {
private:
    int code;
    std::string msg;
public:
    SendoException(std::string msg, int code) {
        this->msg = msg;
        this->code = code;
    }

    SendoException(int code) : SendoException("Error" + code, code) {}

    SendoException() : SendoException(0) {}

    int getCode(){
        return code;
    }

    std::string what(){
        return msg;
    }
};
SENDO_NAMESPACE_END
#endif //SENDO_SENDOEXCEPTION_H
