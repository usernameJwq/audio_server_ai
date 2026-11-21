#include <iostream>

#include "tts_server.h"
#include "logs.h"

int main(int argc, char* argv[]) {
    // 初始化log
    init_logger();

    int port = 8080;
    Server serevr(port);
    serevr.run();
}