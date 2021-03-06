#ifndef TELEPORT_TELEPORTRPCSERVER_H
#define TELEPORT_TELEPORTRPCSERVER_H


#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/server/abstractserver.h>
#include <src/crypto/uint256.h>
#include <test/teleport_tests/node/Data.h>
#include "HttpAuthServer.h"


class TeleportLocalServer;
class TeleportNetworkNode;


class TeleportRPCServer : public jsonrpc::AbstractServer<TeleportRPCServer>
{
public:
    uint256 network_id;
    std::map<std::string,std::string> headers;
    std::string response{"a response"};
    TeleportLocalServer *teleport_local_server{nullptr};
    TeleportNetworkNode *node{nullptr};
    std::vector<std::string> methods;
    Data *data{nullptr};

    explicit TeleportRPCServer(jsonrpc::HttpAuthServer &server) :
            jsonrpc::AbstractServer<TeleportRPCServer>(server)
    {
        BindMethod("help", &TeleportRPCServer::Help);
        BindMethod("getinfo", &TeleportRPCServer::GetInfo);
        BindMethod("setnetworkid", &TeleportRPCServer::SetNetworkID);
        BindMethod("new_proof", &TeleportRPCServer::NewProof);
        BindMethod("balance", &TeleportRPCServer::Balance);
        BindMethod("start_mining", &TeleportRPCServer::StartMining);
        BindMethod("stop_mining", &TeleportRPCServer::StopMining);
        BindMethod("start_mining_asynchronously", &TeleportRPCServer::StartMiningAsynchronously);
        BindMethod("keep_mining_asynchronously", &TeleportRPCServer::KeepMiningAsynchronously);
        BindMethod("sendtopublickey", &TeleportRPCServer::SendToPublicKey);
        BindMethod("getnewaddress", &TeleportRPCServer::GetNewAddress);
        BindMethod("sendtoaddress", &TeleportRPCServer::SendToAddress);
        BindMethod("getknownaddressbalance", &TeleportRPCServer::GetKnownAddressBalance);
        BindMethod("addnode", &TeleportRPCServer::AddNode);
        BindMethod("requesttips", &TeleportRPCServer::RequestTips);
        BindMethod("getcalendar", &TeleportRPCServer::GetCalendar);
        BindMethod("getminedcredit", &TeleportRPCServer::GetMinedCredit);
        BindMethod("getminedcreditmessage", &TeleportRPCServer::GetMinedCreditMessage);
        BindMethod("getbatch", &TeleportRPCServer::GetBatch);
        BindMethod("listunspent", &TeleportRPCServer::ListUnspent);
        BindMethod("gettransaction", &TeleportRPCServer::GetTransaction);

        BindMethod("requestdepositaddress", &TeleportRPCServer::RequestDepositAddress);
        BindMethod("listdepositaddresses", &TeleportRPCServer::ListDepositAddresses);
        BindMethod("withdrawdepositaddress", &TeleportRPCServer::WithdrawDepositAddress);
        BindMethod("transferdepositaddress", &TeleportRPCServer::TransferDepositAddress);

        BindMethod("listwithdrawals", &TeleportRPCServer::ListWithdrawals);

        BindMethod("getunspentcreditsfromstubs", &TeleportRPCServer::GetUnspentCreditsFromStubs);

        BindMethod("getprivatekeyfromcurrencysecret", &TeleportRPCServer::GetPrivateKeyFromCurrencySecret);
        BindMethod("getcurrencysecretfromprivatekey", &TeleportRPCServer::GetCurrencySecretFromPrivateKey);
        BindMethod("getcurrencyaddressfromprivatekey", &TeleportRPCServer::GetCurrencyAddressFromPrivateKey);

        BindMethod("listcurrencies", &TeleportRPCServer::ListCurrencies);
    }

    void SetTeleportLocalServer(TeleportLocalServer *teleport_local_server_);

    void Help(const Json::Value& request, Json::Value& response);

    void GetInfo(const Json::Value& request, Json::Value& response);

    void SetNetworkID(const Json::Value& request, Json::Value& response);

    void NewProof(const Json::Value& request, Json::Value& response);

    void Balance(const Json::Value& request, Json::Value& response);

    void StartMining(const Json::Value& request, Json::Value& response);

    void StopMining(const Json::Value& request, Json::Value& response);

    void StartMiningAsynchronously(const Json::Value& request, Json::Value& response);

    void KeepMiningAsynchronously(const Json::Value& request, Json::Value& response);

    void BindMethod(const char* method_name,
                    void (TeleportRPCServer::*method)(const Json::Value &,Json::Value &));

    void SendToPublicKey(const Json::Value &request, Json::Value &response);

    void GetNewAddress(const Json::Value &request, Json::Value &response);

    void SendToAddress(const Json::Value &request, Json::Value &response);

    void GetKnownAddressBalance(const Json::Value &request, Json::Value &response);

    void AddNode(const Json::Value &request, Json::Value &response);

    void RequestTips(const Json::Value &request, Json::Value &response);

    void GetCalendar(const Json::Value &request, Json::Value &response);

    void GetMinedCredit(const Json::Value &request, Json::Value &response);

    void GetMinedCreditMessage(const Json::Value &request, Json::Value &response);

    void GetBatch(const Json::Value &request, Json::Value &response);

    void ListUnspent(const Json::Value &request, Json::Value &response);

    void GetTransaction(const Json::Value &request, Json::Value &response);

    void RequestDepositAddress(const Json::Value &request, Json::Value &response);

    void ListDepositAddresses(const Json::Value &request, Json::Value &response);

    void WithdrawDepositAddress(const Json::Value &request, Json::Value &response);

    void TransferDepositAddress(const Json::Value &request, Json::Value &response);

    void ListWithdrawals(const Json::Value &request, Json::Value &response);

    void GetUnspentCreditsFromStubs(const Json::Value &request, Json::Value &response);

    void GetCurrencySecretFromPrivateKey(const Json::Value &request, Json::Value &response);

    void GetPrivateKeyFromCurrencySecret(const Json::Value &request, Json::Value &response);

    void GetCurrencyAddressFromPrivateKey(const Json::Value &request, Json::Value &response);

    void ListCurrencies(const Json::Value &request, Json::Value &response);
};


#endif //TELEPORT_TELEPORTRPCSERVER_H
