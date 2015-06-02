#ifndef UVPEER_H
#define UVPEER_H

#include <string.h>

class UVBuf {
    int buf_size;
public:
	char* data;
	int data_size;
	UVBuf() {
        data = nullptr;
        data_size = buf_size = 0;
    }
	~UVBuf() {
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
			} else return nullptr;
		} else return data + data_size;
    }
    
    void clear() {
        data_size = 0;
    }
    
    void append(int size) {
        data_size += size;
    }
};

#include <uv.h>
class UVPeer : public UVBuf{
protected:
	uv_loop_t* _loop;
public:
	std::string remote;
	bool connected = false;
	uv_tcp_t peer;
	UVPeer() {}

	void init(uv_loop_t* loop = nullptr) {
		if(loop == nullptr) _loop = uv_default_loop();
		else _loop = loop;
		uv_tcp_init(_loop, &peer);
		peer.data = (void*)this;
	}

	virtual ~UVPeer() {
		if(_loop != uv_default_loop()) {
			uv_loop_close(_loop);
			free(_loop);
		}
	}
	virtual void onConnect() {}
	virtual void onRead() {}
	virtual void onWrite() {}
	virtual void onClose(){}

	static void close_callback(uv_handle_t *handle){
		UVPeer* peer = (UVPeer*)handle->data;
		peer->onClose();
	}

	void close() {
		if(connected) {
			connected = false;
			uv_close((uv_handle_t*)&peer, close_callback);
		}
	}

	static void malloc_func(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		UVPeer* peer = (UVPeer*)handle->data;
		*buf = uv_buf_init(peer->get((int)suggested_size), (int)suggested_size);
	}

	static void read_callback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){
		UVPeer* peer = (UVPeer*)stream->data;
		if(nread < 0 && nread != UV_EOF) peer->close();
		else if(nread > 0) {
			peer->append(nread);
			peer->onRead();
			uv_read_stop(stream);
		}
	}

	void read() {
		uv_read_start((uv_stream_t*)&peer, malloc_func, read_callback);
	}

	typedef struct{
		uv_write_t req;
		uv_buf_t buf;
	} write_t;

	static void write_callback(uv_write_t *req, int status) {
		UVPeer* peer = (UVPeer*)req->handle->data;
		peer->onWrite();
		write_t *wr = (write_t*)req;
		free(wr->buf.base);
		free(wr);
	}

	void write(const char* data, int len) {
		write_t *req = (write_t*)malloc(sizeof(write_t));
		req->buf = uv_buf_init((char*)malloc(len), len);
		memcpy(req->buf.base, data, len);
		uv_write((uv_write_t*)req, (uv_stream_t*)&peer, &req->buf, 1, write_callback);
	}
};

#include <string>
template<typename T> class UVServer : public UVPeer {
public:
	static void __connect_callback(uv_stream_t* server, int status){
		if(status >= 0) {
			auto peer = new T;
			peer->init();
			if(uv_accept(server, (uv_stream_t*)&peer->peer) == 0) {
				struct sockaddr addr;
				int addr_len = sizeof(addr);
				uv_tcp_getpeername((uv_tcp_t*)&peer->peer, &addr, &addr_len);
				sockaddr_in sin;
				memcpy(&sin, &addr, sizeof(sin));
				peer->connected = true;
				peer->remote = inet_ntoa(sin.sin_addr);
				peer->onConnect();
			} else peer->close();
		}
	}

	bool start(const char* local, unsigned short port) {
		init();
		sockaddr_in addr;
		uv_ip4_addr(local, port, &addr);
		uv_tcp_bind(&peer, (const struct sockaddr*)&addr, 0);
		return uv_listen((uv_stream_t*)&peer, 200, __connect_callback) == 0;
	}
};

class UVClient : public UVPeer {
public:
	UVClient() {
		uv_loop_t* loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
		uv_loop_init(loop);
		init(loop);
	}

	static void connect_callback(uv_connect_t* req, int status) {
		if(status >= 0) {
			UVClient* peer = (UVClient*)req->handle->data;
			peer->connected = true;
		}
		free(req);
	}

	bool connect(const char* host, unsigned short port) {
		uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
		struct hostent* hptr = gethostbyname(host);
		if(hptr != nullptr && hptr->h_length > 0) {
			struct sockaddr_in addr;
			uv_ip4_addr(inet_ntoa(*(in_addr*)hptr->h_addr), port, &addr);
			remote = host;
			if(uv_tcp_connect(req, &peer, (const struct sockaddr*)&addr, connect_callback) == 0) {
				uv_run(_loop, UV_RUN_DEFAULT);
				return connected;
			}
		}
		return false;
	}

	void write(const char* data, int len) {
		UVPeer::write(data, len);
		uv_run(_loop, UV_RUN_DEFAULT);
	}

	void read() {
		UVPeer::read();
		uv_run(_loop, UV_RUN_DEFAULT);
	}

	void close() {
		UVPeer::close();
		uv_run(_loop, UV_RUN_DEFAULT);
	}
};

#endif
