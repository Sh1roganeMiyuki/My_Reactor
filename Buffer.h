#include "vector"
#include "string"
#include "algorithm"
class Buffer{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024; 

    explicit Buffer(size_t initialSize = kInitialSize);

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    const char* peek() const { return &buffer_[readIndex_]; }

    void retrieveAll() { readIndex_ = kCheapPrepend; writeIndex_ = kCheapPrepend; }

    void retrieve(size_t len);

    std::string retrieveAllAsString() ;
    void append(const void* data, size_t len);
    void ensureWritableBytes(size_t len);
    char* beginWrite() { return &buffer_[writeIndex_]; }
    ssize_t readFd(int fd, int* savedErrno);
private:
    void makeSpace(size_t len) ;
    //std::vector<char>::iterator begin() { return buffer_.begin(); }
    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};