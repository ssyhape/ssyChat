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

class ConfigMgr;
extern ConfigMgr gCfgMgr;
// varify Service port 
extern std::string varifyServciePort;