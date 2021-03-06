#ifndef TELEPORT_WALLET_H
#define TELEPORT_WALLET_H


#include <test/teleport_tests/teleport_data/MemoryDataStore.h>
#include <src/crypto/point.h>
#include <src/credits/SignedTransaction.h>
#include <src/credits/creditsign.h>
#include <src/crypto/key.h>
#include <src/base/BitcoinAddress.h>
#include "test/teleport_tests/node/credit/messages/MinedCreditMessage.h"
#include "test/teleport_tests/node/credit/structures/CreditSystem.h"

class CreditTracker;

class Wallet
{
public:
    MemoryDataStore& keydata;
    std::vector<CreditInBatch> credits;
    CreditSystem *credit_system{nullptr};
    Calendar *calendar{nullptr};
    BitChain *spent_chain{nullptr};

    explicit Wallet(MemoryDataStore& keydata_):
        keydata(keydata_)
    { }

    void SetCreditSystem(CreditSystem *credit_system_);

    void SetCalendar(Calendar *calendar_);

    void SetSpentChain(BitChain *spent_chain_);

    std::vector<CreditInBatch> GetCredits();

    Point GetNewPublicKey();

    void HandleCreditInBatch(CreditInBatch credit_in_batch);

    bool PrivateKeyIsKnown(CreditInBatch credit_in_batch);

    bool HaveCreditInBatchAlready(CreditInBatch credit_in_batch);

    SignedTransaction GetSignedTransaction(Point pubkey, uint64_t amount);

    UnsignedTransaction GetUnsignedTransaction(vch_t key_data, uint64_t amount);

    void AddBatchToTip(MinedCreditMessage msg, CreditSystem *credit_system);

    void RemoveTransactionInputsSpentInBatchFromCredits(MinedCreditMessage &msg, CreditSystem *credit_system);

    void AddNewCreditsFromBatch(MinedCreditMessage &msg, CreditSystem *credit_system);

    uint64_t Balance();

    void RemoveBatchFromTip(MinedCreditMessage msg, CreditSystem *credit_system);

    void AddTransactionInputsSpentInRemovedBatchToCredits(MinedCreditMessage &msg, CreditSystem *credit_system);

    void RemoveCreditsFromRemovedBatch(MinedCreditMessage &msg, CreditSystem *credit_system);

    bool PrivateKeyIsKnown(Point public_key);

    void SwitchAcrossFork(uint160 old_tip, uint160 new_tip, CreditSystem *credit_system);

    void ImportPrivateKey(const CBigNum private_key);

    void SortCredits();

    UnsignedTransaction GetUnsignedTransaction(Point pubkey, uint64_t amount);
};

uint160 GetKeyHashFromAddress(std::string address_string);

std::string GetAddressFromPublicKey(Point public_key);

#endif //TELEPORT_WALLET_H
