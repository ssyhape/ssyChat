#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <string>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ERRORCODE {
	SUCCESS = 0,
	ERROR_JSON = 1001,
	ERROR_RPC = 1002
};

class Defer {
public:
	// 接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	// 析构函数中执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};