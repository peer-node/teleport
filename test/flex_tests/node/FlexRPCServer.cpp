#include "FlexRPCServer.h"
#include "FlexLocalServer.h"

void FlexRPCServer::BindMethod(const char* method_name,
                               void (FlexRPCServer::*method)(const Json::Value &,Json::Value &))
{
    this->bindAndAddMethod(
            jsonrpc::Procedure(method_name, jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_STRING, NULL),
            method);
}

void FlexRPCServer::Help(const Json::Value& request, Json::Value& response)
{
    response = "help";
}

void FlexRPCServer::GetInfo(const Json::Value& request, Json::Value& response)
{
    response["network_id"] = network_id.ToString();
}

void FlexRPCServer::SetNetworkID(const Json::Value& request, Json::Value& response)
{
    auto network_id_string = request[0].asString();
    network_id = uint256(network_id_string);
    response = "ok";
}

void FlexRPCServer::NewProof(const Json::Value &request, Json::Value &response)
{
    flexnode->latest_proof_of_work = NetworkSpecificProofOfWork(request["proof_base64"].asString());
}

void FlexRPCServer::SetFlexNode(FlexLocalServer *flexnode_)
{
    flexnode = flexnode_;
}