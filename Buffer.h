#include "vector"
#include "string"
#include "algorithm"
class Buffer{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024; 

    explicit Buffer(size_t initialSize = kInitialSize) : 
        buffer_(kCheapPrepend + initialSize), readIndex_(kCheapPrepend), writeIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    const char* peek() const { return &buffer_[readIndex_]; }

    void retrieveAll() { readIndex_ = kCheapPrepend; writeIndex_ = kCheapPrepend; }

    void retrieve(size_t len) {
        if (len < readableBytes()) { //说明只读了一部分,移动索引
            readIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    std::string retrieveAllAsString() {
        std::string result(peek(), readableBytes());
        retrieveAll();
        return result;
    }
    void append(const void* data, size_t len){
        ensureWritableBytes(len);
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, buffer_.begin() + writeIndex_);
        writeIndex_ += len;
    }
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }
    char* beginWrite() { return &buffer_[writeIndex_]; }
private:
    void makeSpace(size_t len) {
        // 策略 A：writable < len
        // 策略 B：writable + prepend > len (内部腾挪)
        // 稍后实现
        if (writableBytes() + kCheapPrepend > len) { //严重bug，明天修
            buffer_.resize(len + kCheapPrepend);
        }else{
            size_t readable = readableBytes();
            std::copy(buffer_.begin() + readIndex_, buffer_.begin() + writeIndex_, buffer_.begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
    }
    std::vector<char>::iterator begin() { return buffer_.begin(); }
    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};