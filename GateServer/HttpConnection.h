#pragma once

#include "const.h"
#include <iostream>

// http管理类
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    void Start();
private:
    void CheckDeadline();
    void WriteResponse();
    void HandleReq();
    tcp::socket  _socket;

    // The buffer for performing reads.
    beast::flat_buffer  _buffer{ 8192 }; // 存放数据

    // The request message.
    http::request<http::dynamic_body> _request;// 用来解析请求
    
    // The response message.
    http::response<http::dynamic_body> _response;// 生成reponse
    
    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60) }; // 定时器
};

