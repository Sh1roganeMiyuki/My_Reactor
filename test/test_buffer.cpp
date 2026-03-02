// test_buffer.cpp
#include "Buffer.h"
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <string.h>

void testReadFd() {
    Buffer buf; // 初始大小 1024
    
    // 1. 创建管道 (模拟 socket)
    // pipes[0] 是读端，pipes[1] 是写端
    int pipes[2];
    if (pipe(pipes) == -1) {
        perror("pipe");
        return;
    }

    // 2. 往管道里写 2000 字节数据 (大于 Buffer 初始容量)
    // 预期：readFd 会先填满 Buffer (1024字节)，剩下的填入栈 extrabuf，然后 append 扩容
    std::string content(2000, 'X'); 
    write(pipes[1], content.data(), content.size());

    // 3. 从管道读数据
    int saveErrno = 0;
    ssize_t n = buf.readFd(pipes[0], &saveErrno);

    std::cout << "Read bytes: " << n << std::endl;
    std::cout << "Buffer readable bytes: " << buf.readableBytes() << std::endl;
    
    // 4. 验证
    if (n == 2000 && buf.readableBytes() == 2000) {
        std::string res = buf.retrieveAllAsString();
        if (res == content) {
            std::cout << "✅ SUCCESS: readFd handled large data correctly!" << std::endl;
        } else {
            std::cout << "❌ FAIL: Content mismatch!" << std::endl;
        }
    } else {
        std::cout << "❌ FAIL: Size mismatch. Expected 2000." << std::endl;
    }

    close(pipes[0]);
    close(pipes[1]);
}

int main() {
    testReadFd();
    return 0;
}