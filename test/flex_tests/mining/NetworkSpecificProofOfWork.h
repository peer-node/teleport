#ifndef FLEX_NETWORKSPECIFICPROOFOFWORK_H
#define FLEX_NETWORKSPECIFICPROOFOFWORK_H


#include <src/crypto/uint256.h>
#include <src/mining/work.h>

class NetworkSpecificProofOfWork
{
public:
    NetworkSpecificProofOfWork() { }
    NetworkSpecificProofOfWork(std::vector<uint256> branch, TwistWorkProof proof);

    NetworkSpecificProofOfWork(std::string base64_string);

    std::vector<uint256> branch;
    TwistWorkProof proof;

    std::string GetBase64String();

    bool operator==(const NetworkSpecificProofOfWork& other) const
    {
        return branch == other.branch && proof.GetHash() == other.proof.GetHash();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(branch);
        READWRITE(proof);
    )

    bool IsValid();
};


#endif //FLEX_NETWORKSPECIFICPROOFOFWORK_H
