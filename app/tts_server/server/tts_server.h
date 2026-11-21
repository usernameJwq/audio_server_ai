#ifndef TTS_SERVER_HTTP_H_
#define TTS_SERVER_HTTP_H_

#include "httplib.h"

class Server {
public:
    Server(const int& port);
    ~Server();

public:
    void run();

private:
    httplib::Server server_;
    int port_;
};

#endif // TTS_SERVER_HTTP_H_