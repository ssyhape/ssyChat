#pragma once
#include "Singleton.h"
#include "const.h"
#include "VerifyGrpcClient.h"
#include <functional>
#include <map>



class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem: public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
    void RegGet(std::string, HttpHandler handler);
    void RegPost(std::string, HttpHandler handler);
private:
    LogicSystem();
    std::map<std::string, HttpHandler> _post_handlers;
    std::map<std::string, HttpHandler> _get_handlers;
};

