#include "RedisMgr.h"
#include "const.h"
#include "ConfigMgr.h"
#include <queue>
namespace {
	class RedisConPool {
	public:
		RedisConPool(size_t poolSize, const char* host, int port, const char* pwd) :
			poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd), counter_(0) {
			for (size_t i = 0; i < poolSize_; ++i) {
				auto* context = redisConnect(host, port);
				if (context == nullptr || context->err != 0) {
					if (context != nullptr)
						redisFree(context);
					continue;
				}
				auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
				if (reply->type == REDIS_REPLY_ERROR) {
					std::cout << "认证失败" << std::endl;
					freeReplyObject(reply);
					continue;
				}
				freeReplyObject(reply);
				std::cout << "认证成功" << std::endl;
				connections_.push(context);
			}

			check_thread_ = std::thread([this]() {
				while (!b_stop_) {
					counter_++;
					if (counter_ >= 60) {
						checkThread();
						counter_ = 0;
					}
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
				});
		}
		~RedisConPool() {

		}
		void ClearConnections() {
			std::lock_guard<std::mutex> lock(mutex_);
			while (!connections_.empty()) {
				auto* context = connections_.front();
				redisFree(context);
				connections_.pop();
			}
		}

		redisContext* getConnection() {
			std::unique_lock<std::mutex> lock(mutex_);
			cond_.wait(lock, [this] {
				if (b_stop_) {
					return true;
				}
				return !connections_.empty();
				});
			if (b_stop_) {
				return nullptr;
			}
			auto* context = connections_.front();
			connections_.pop();
			return context;
		}

		void returnConnection(redisContext* context) {
			std::lock_guard<std::mutex> lock(mutex_);
			if (b_stop_) {
				return;
			}
			connections_.push(context);
			cond_.notify_one();
		}

		void Close() {
			b_stop_ = true;
			cond_.notify_all();
			check_thread_.join();
		}
	private:
		std::atomic<bool> b_stop_;
		size_t poolSize_;
		const char* host_;
		const char* pwd_;
		int port_;
		std::queue<redisContext*> connections_;
		std::mutex mutex_;
		std::condition_variable cond_;
		std::thread check_thread_;
		int counter_;
	};
} // 

RedisMgr::RedisMgr() {
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	_con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}
bool RedisMgr::Connect(const std::string& host, int port) {
	this->_connect = redisConnect(host.c_str(), port);
	if (this->_connect != NULL && this->_connect->err) {
		std::cout << "connect error " << this->_connect->errstr << std::endl;
		return false;
	}
	return true;
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "GET %s", key.c_str());

	if (this->_reply == NULL) {
		std::cout << "[ GET " << key << "] failed" << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}

	if (this->_reply->type != REDIS_REPLY_STRING) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}

	value = this->_reply->str;
	freeReplyObject(this->_reply);
	std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;

	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "SET %s %s", key.c_str(), value.c_str());
	if (NULL == this->_reply)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	if (!(this->_reply->type == REDIS_REPLY_STATUS && (strcmp(this->_reply->str, "OK") == 0 || strcmp(this->_reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	freeReplyObject(this->_reply);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::Auth(const std::string& password) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "AUTH %s", password.c_str());
	if (this->_reply->type == REDIS_REPLY_ERROR) {
		std::cout << "认证失败" << std::endl;
		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(this->_reply);
		return false;
	}
	else {
		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(this->_reply);
		std::cout << "认证成功" << std::endl;
		return true;
	}
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == this->_reply)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	if (this->_reply->type != REDIS_REPLY_INTEGER || this->_reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "LPOP %s ", key.c_str());
	if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	value = _reply->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == this->_reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	if (this->_reply->type != REDIS_REPLY_INTEGER || this->_reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "RPOP %s ", key.c_str());
	if (_reply == nullptr || _reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	value = _reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	this->_reply = (redisReply*)redisCommand(this->_connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}
bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;
	this->_reply = (redisReply*)redisCommandArgv(this->_connect, 4, argv, argvlen);
	if (_reply == nullptr || _reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	this->_reply = (redisReply*)redisCommandArgv(this->_connect, 3, argv, argvlen);
	if (this->_reply == nullptr || this->_reply->type == REDIS_REPLY_NIL) {
		freeReplyObject(this->_reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}
	std::string value = this->_reply->str;
	freeReplyObject(this->_reply);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisMgr::Del(const std::string& key)
{
	this->_reply = (redisReply*)redisCommand(this->_connect, "DEL %s", key.c_str());
	if (this->_reply == nullptr || this->_reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	this->_reply = (redisReply*)redisCommand(this->_connect, "exists %s", key.c_str());
	if (this->_reply == nullptr || this->_reply->type != REDIS_REPLY_INTEGER || this->_reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(this->_reply);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(this->_reply);
	return true;
}

void RedisMgr::Close() {
	redisFree(_connect);
}