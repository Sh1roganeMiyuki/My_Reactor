#include "Buffer.h"
#include <errno.h>      // 错误码 errno
#include <sys/uio.h>    // readv, struct iovec
#include <unistd.h>     // read, write
// 去掉了 explicit，去掉了 = kInitialSize
Buffer::Buffer(size_t initialSize) : 
        buffer_(kCheapPrepend + initialSize), 
        readIndex_(kCheapPrepend), 
        writeIndex_(kCheapPrepend) {}

void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) { //说明只读了一部分,移动索引
        readIndex_ += len;
    } else {
        retrieveAll();
    }
}
std::string Buffer::retrieveAllAsString() {
    std::string result(peek(), readableBytes());
    retrieveAll();
    return result;
}

void Buffer::append(const void* data, size_t len){
    ensureWritableBytes(len);
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, buffer_.begin() + writeIndex_);
    writeIndex_ += len;
}

void Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
}

void Buffer::makeSpace(size_t len) {
    // 1. 判断：剩余空闲 + 头部垃圾 < len？
    if (writableBytes() + readIndex_ - kCheapPrepend < len) { 
        // 真的不够，硬扩容
        buffer_.resize(writeIndex_ + len);
    } else {
        // 2. 够用，内部搬运
        size_t readable = readableBytes();
        std::copy(buffer_.begin() + readIndex_, 
                    buffer_.begin() + writeIndex_, 
                    buffer_.begin() + kCheapPrepend);
        // 重置指针
        readIndex_ = kCheapPrepend;
        writeIndex_ = readIndex_ + readable;
    }
}
/**
 * 从 socket 读数据
 * @param fd      socket 文件描述符
 * @param saveErrno (输出参数) 如果出错，把 errno 存到这里返回给上层
 * @return        实际读取到的字节数
 */
ssize_t Buffer::readFd(int fd, int* savedErrno){
    char extrabuf[65536];
    const size_t writable = writableBytes();
    struct iovec iov[2];
    
    iov[0].iov_base = beginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;

    const ssize_t n = readv(fd, iov, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    }else if (static_cast<size_t>(n) <= writable) {
        writeIndex_ += n;
    }else{
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}