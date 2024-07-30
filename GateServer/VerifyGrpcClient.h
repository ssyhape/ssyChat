#pragma once
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"
#include "const.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

// getVarify grpc Client µ¥Àý
class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email) {
        ClientContext context;
        GetVarifyRsp reply;
        GetVarifyReq request;
        request.set_email(email);
        Status status = stub_->GetVarifyCode(&context, request, &reply);
        if (status.ok()) {
            return reply;
        }
        else {
            reply.set_error(ERRORCODE::ERROR_RPC);
            return reply;
        }
    }
private:
    VerifyGrpcClient() {
        std::cout << "varify service port is" << varifyServciePort << std::endl;
        std::shared_ptr<Channel> channel = grpc::CreateChannel(varifyServciePort, grpc::InsecureChannelCredentials());
        stub_ = VarifyService::NewStub(channel);
    }

    std::unique_ptr<VarifyService::Stub> stub_;
};

