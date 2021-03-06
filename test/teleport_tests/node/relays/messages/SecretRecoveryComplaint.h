#ifndef TELEPORT_SECRETRECOVERYCOMPLAINT_H
#define TELEPORT_SECRETRECOVERYCOMPLAINT_H


#include <src/crypto/signature.h>
#include "test/teleport_tests/node/Data.h"
#include "SecretRecoveryMessage.h"


class Relay;

class SecretRecoveryComplaint
{
public:
    uint64_t successor_number;
    uint160 secret_recovery_message_hash{0};
    uint32_t position_of_key_sharer{0};
    uint32_t position_of_bad_encrypted_secret{0};
    uint32_t position_of_quarter_holder{0};
    uint256 private_receiving_key{0};
    Signature signature;

    static std::string Type() { return "secret_recovery_complaint"; }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(successor_number);
        READWRITE(secret_recovery_message_hash);
        READWRITE(position_of_key_sharer);
        READWRITE(position_of_bad_encrypted_secret);
        READWRITE(position_of_quarter_holder);
        READWRITE(private_receiving_key);
        READWRITE(signature);
    );

    JSON(successor_number,
         secret_recovery_message_hash, position_of_key_sharer, position_of_bad_encrypted_secret,
         position_of_quarter_holder, private_receiving_key, signature);

    DEPENDENCIES(secret_recovery_message_hash);

    IMPLEMENT_HASH_SIGN_VERIFY();

    void Populate(SecretRecoveryMessage recovery_message, uint32_t key_sharer_position, uint32_t key_part_position,
                  Data data);

    Point VerificationKey(Data data);

    Relay *GetComplainer(Data data);

    Relay *GetSecretSender(Data data);

    SecretRecoveryMessage GetSecretRecoveryMessage(Data data);

    bool IsValid(Data data);

    bool EncryptedSecretIsOk(Data data);

    CBigNum RecoverSecret(Data data);

    uint256 GetEncryptedSecret(Data data);

    Point GetPointCorrespondingToSecret(Data data);

    bool PrivateReceivingKeyIsCorrect(Data data);

    Relay *GetDeadRelay(Data data);

    bool SuccessionCompletedMessageHasBeenReceived(Data data);
};


#endif //TELEPORT_SECRETRECOVERYCOMPLAINT_H
