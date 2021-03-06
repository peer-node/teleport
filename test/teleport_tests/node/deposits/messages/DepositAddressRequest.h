#ifndef TELEPORT_DEPOSITADDRESSREQUEST_H
#define TELEPORT_DEPOSITADDRESSREQUEST_H


#include <string>
#include <src/mining/work.h>
#include <src/crypto/signature.h>
#include <test/teleport_tests/node/Data.h>
#include <src/crypto/secp256k1point.h>

#define DEPOSIT_NUM_RELAYS 5
#define DEPOSIT_DIFFICULTY 100000


class DepositAddressRequest
{
public:
    Point depositor_key;
    uint8_t curve;
    vch_t currency_code;
    SimpleWorkProof proof_of_work;
    Signature signature;

    DepositAddressRequest() { }

    DepositAddressRequest(uint8_t curve, vch_t &currency_code, Data data):
            curve(curve),
            currency_code(currency_code)
    {
        SetDepositorKey(data);
    }

    void SetDepositorKey(Data data)
    {
        CBigNum depositor_privkey;
        depositor_privkey.Randomize(Secp256k1Point::Modulus());
        depositor_key = Point(SECP256K1, depositor_privkey);
        data.keydata[depositor_key]["privkey"] = depositor_privkey;
    }

    static string_t Type() { return string_t("deposit_request"); }

    DEPENDENCIES();

    uint256 PreWorkHash()
    {
        SimpleWorkProof original_proof = proof_of_work;
        Signature original_signature = signature;
        signature = Signature();
        proof_of_work = SimpleWorkProof();
        uint256 hash = GetHash();
        proof_of_work = original_proof;
        signature = original_signature;
        return hash;
    }

    void DoWork(Data data)
    {
        bool done = false;
        while (!done)
        {
            proof_of_work = SimpleWorkProof(PreWorkHash(), DEPOSIT_DIFFICULTY);
            uint8_t keep_working = 1;
            proof_of_work.DoWork(&keep_working);
            if (proof_of_work.WorkDone() > 0)
                done = true;
            else
                SetDepositorKey(data);
        }
    }

    bool CheckWork()
    {
        return proof_of_work.target == SimpleWorkProof::TargetFromDifficulty(DEPOSIT_DIFFICULTY) 
               and proof_of_work.IsValidProofOfWork();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(depositor_key);
        READWRITE(curve);
        READWRITE(currency_code);
        READWRITE(proof_of_work);
        READWRITE(signature);
    )

    Point VerificationKey(Data data)
    {
        return depositor_key;
    }

    IMPLEMENT_HASH_SIGN_VERIFY();
};


#endif //TELEPORT_DEPOSITADDRESSREQUEST_H
