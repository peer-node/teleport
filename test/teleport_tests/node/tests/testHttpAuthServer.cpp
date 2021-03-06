#include "gmock/gmock.h"
#include "test/teleport_tests/node/server/HttpAuthServer.h"
#include <jsonrpccpp/client.h>
#include "test/teleport_tests/mining/server_key.h"

#define private public // to allow access to HttpClient.curl
#include <jsonrpccpp/client/connectors/httpclient.h>
#undef private
#include <jsonrpccpp/server/abstractserver.h>
#include <fstream>

using namespace ::testing;
using namespace jsonrpc;
using namespace std;


class TestAuthRPCServer : public jsonrpc::AbstractServer<TestAuthRPCServer>
{
public:
    std::map<std::string, std::string> headers;

    TestAuthRPCServer(jsonrpc::HttpAuthServer &server) :
            jsonrpc::AbstractServer<TestAuthRPCServer>(server)
    {
        BindMethod("help", &TestAuthRPCServer::Help);
    }

    void BindMethod(const char* method_name,
                    void (TestAuthRPCServer::*method)(const Json::Value &,Json::Value &))
    {
        this->bindAndAddMethod(
                jsonrpc::Procedure(method_name, jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_STRING, NULL),
                method);
    }

    void Help(const Json::Value& request, Json::Value& response)
    {
        response = "help";
    }
};


class AHttpAuthServer : public Test
{
public:
    HttpAuthServer *http_auth_server;
    TestAuthRPCServer *server;
    HttpClient *http_client;
    Client *client;

    virtual void SetUp()
    {
        http_auth_server = new HttpAuthServer(8338, "rpcuser", "rpcpassword");
        server = new TestAuthRPCServer(*http_auth_server);
        server->StartListening();
        http_client = new HttpClient("http://localhost:8338");
        client = new Client(*http_client);
    }

    virtual void TearDown()
    {
        delete client;
        delete http_client;
        server->StopListening();
        delete server;
        delete http_auth_server;
    }
};

TEST_F(AHttpAuthServer, StoresHeadersFromRequests)
{
    http_client->AddHeader("x", "y");
    auto response = client->CallMethod("help", Json::Value());
    ASSERT_THAT(http_auth_server->headers["x"], Eq("y"));
}

TEST_F(AHttpAuthServer, ForgetsHeadersFromPreviousRequests)
{
    http_client->AddHeader("x", "y");
    auto response = client->CallMethod("help", Json::Value());
    ASSERT_THAT(http_auth_server->headers["x"], Eq("y"));

    http_client->RemoveHeader("x");
    client->CallMethod("help", Json::Value());
    ASSERT_THAT(http_auth_server->headers.count("x"), Eq(0));
}

TEST_F(AHttpAuthServer, RequiresCorrectAuthorization)
{
    http_auth_server->RequireAuthentication();
    ASSERT_ANY_THROW(client->CallMethod("help", Json::Value()));

    http_client->AddHeader("Authorization", "Basic rpcuser:rpcpassword");
    auto result = client->CallMethod("help", Json::Value());
    ASSERT_THAT(result.asString(), Ne(""));
}
