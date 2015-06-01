#ifndef UVPEER_H
#define UVPEER_H

class ZBuf {
    char* data;
    int data_size;
    int buf_size;
public:
    ZBuf() {
        data = nullptr;
        size = buf_size = 0;
    }
    ~ZBuf() {
        if(data != nullptr) delete[] data;
    }
    
    char* get(int size) {
        if(buf_size - data_size < size) {
            buf_size = data_size + size;
            char* ptr = new char[buf_size];
            if(ptr != nullptr) {
                if(data != nullptr) {
                    memmove(ptr, data, data_size);
                    delete[] data;
                }
                data = ptr;
                return data + data_size;
            }
        }
        return nullptr;
    }
    
    void clear() {
        data_size = 0;
    }
    
    void append(int size) {
        data_size += size;
    }
};

#endif
