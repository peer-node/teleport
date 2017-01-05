#include <src/vector_tools.h>
#include <src/credits/creditsign.h>
#include "RelayState.h"

#include "log.h"
#define LOG_CATEGORY "RelayState.cpp"



void RelayState::ProcessRelayJoinMessage(RelayJoinMessage relay_join_message)
{
    ThrowExceptionIfMinedCreditMessageHashWasAlreadyUsed(relay_join_message);
    Relay relay = GenerateRelayFromRelayJoinMessage(relay_join_message);
    relays.push_back(relay);
    maps_are_up_to_date = false;
}

void RelayState::ThrowExceptionIfMinedCreditMessageHashWasAlreadyUsed(RelayJoinMessage relay_join_message)
{
    if (MinedCreditMessageHashIsAlreadyBeingUsed(relay_join_message.mined_credit_message_hash))
        throw RelayStateException("mined_credit_message_hash already used");
}

bool RelayState::MinedCreditMessageHashIsAlreadyBeingUsed(uint160 mined_credit_message_hash)
{
    for (Relay& relay : relays)
        if (relay.mined_credit_message_hash == mined_credit_message_hash)
            return true;
    return false;
}

Relay RelayState::GenerateRelayFromRelayJoinMessage(RelayJoinMessage relay_join_message)
{
    Relay new_relay;
    if (relays.size() == 0)
        new_relay.number = 1;
    else
        new_relay.number = relays.back().number + 1;

    new_relay.join_message_hash = relay_join_message.GetHash160();
    new_relay.mined_credit_message_hash = relay_join_message.mined_credit_message_hash;
    new_relay.public_signing_key = relay_join_message.PublicSigningKey();
    new_relay.public_key_set = relay_join_message.public_key_set;
    new_relay.public_key_sixteenths = relay_join_message.public_key_sixteenths;
    return new_relay;
}

void RelayState::AssignRemainingQuarterKeyHoldersToRelay(Relay &relay, uint160 encoding_message_hash)
{
    uint64_t tries = 0;
    while (relay.key_quarter_holders.size() < 4)
    {
        uint64_t quarter_key_holder = SelectKeyHolderWhichIsntAlreadyBeingUsed(relay,
                                                                               encoding_message_hash + (tries++));

        bool candidate_quarter_key_holder_has_this_relay_as_quarter_key_holder
                = VectorContainsEntry(GetRelayByNumber(quarter_key_holder)->key_quarter_holders, relay.number);

        if (candidate_quarter_key_holder_has_this_relay_as_quarter_key_holder)
            continue;
        relay.key_quarter_holders.push_back(quarter_key_holder);
    }
}

uint64_t RelayState::KeyPartHolderWhoJoinedLaterAndIsntAlreadyBeingUsed(Relay &relay, uint160 random_seed)
{
    uint64_t chosen_relay_number = SelectKeyHolderWhichIsntAlreadyBeingUsed(relay, random_seed);
    while (chosen_relay_number <= relay.number)
    {
        random_seed = HashUint160(random_seed);
        chosen_relay_number = SelectKeyHolderWhichIsntAlreadyBeingUsed(relay, random_seed);
    }
    return chosen_relay_number;
}

uint64_t RelayState::SelectKeyHolderWhichIsntAlreadyBeingUsed(Relay &relay, uint160 random_seed)
{
    bool found = false;
    uint64_t relay_position = 0;
    while (not found)
    {
        random_seed = HashUint160(random_seed);
        relay_position = (CBigNum(random_seed) % CBigNum(relays.size())).getulong();
        if (relays[relay_position].number != relay.number and not KeyPartHolderIsAlreadyBeingUsed(relay, relays[relay_position]))
        {
            found = true;
        }
    }
    return relays[relay_position].number;
}

bool RelayState::KeyPartHolderIsAlreadyBeingUsed(Relay &key_distributor, Relay &key_part_holder)
{
    for (auto key_part_holder_group : key_distributor.KeyPartHolderGroups())
    {
        for (auto number_of_existing_key_part_holder : key_part_holder_group)
            if (number_of_existing_key_part_holder == key_part_holder.number)
            {
                return true;
            }
    }
    return false;
}

void RelayState::AssignRemainingKeySixteenthHoldersToRelay(Relay &relay, uint160 encoding_message_hash)
{
    while (relay.first_set_of_key_sixteenth_holders.size() < 16)
        relay.first_set_of_key_sixteenth_holders.push_back(SelectKeyHolderWhichIsntAlreadyBeingUsed(relay, encoding_message_hash));
    while (relay.second_set_of_key_sixteenth_holders.size() < 16)
        relay.second_set_of_key_sixteenth_holders.push_back(SelectKeyHolderWhichIsntAlreadyBeingUsed(relay, encoding_message_hash));
}

Relay *RelayState::GetRelayByNumber(uint64_t number)
{
    EnsureMapsAreUpToDate();
    return relays_by_number.count(number) ? relays_by_number[number] : NULL;
}

Relay *RelayState::GetRelayByJoinHash(uint160 join_message_hash)
{
    EnsureMapsAreUpToDate();
    return relays_by_join_hash.count(join_message_hash) ? relays_by_join_hash[join_message_hash] : NULL;
}

bool RelayState::ThereAreEnoughRelaysToAssignKeyPartHolders(Relay &relay)
{
    uint64_t number_of_relays_that_joined_later = NumberOfRelaysThatJoinedLaterThan(relay);
    uint64_t number_of_relays_that_joined_before = NumberOfRelaysThatJoinedBefore(relay);

    return number_of_relays_that_joined_later >= 3 and
            number_of_relays_that_joined_before + number_of_relays_that_joined_later >= 36;
}

void RelayState::RemoveKeyPartHolders(Relay &relay)
{
    relay.key_quarter_holders.resize(0);
    relay.first_set_of_key_sixteenth_holders.resize(0);
    relay.second_set_of_key_sixteenth_holders.resize(0);
}

bool RelayState::AssignKeyPartHoldersToRelay(Relay &relay, uint160 encoding_message_hash)
{
    if (not ThereAreEnoughRelaysToAssignKeyPartHolders(relay))
        return false;
    RemoveKeyPartHolders(relay);
    AssignKeySixteenthAndQuarterHoldersWhoJoinedLater(relay, encoding_message_hash);
    AssignRemainingQuarterKeyHoldersToRelay(relay, encoding_message_hash);
    AssignRemainingKeySixteenthHoldersToRelay(relay, encoding_message_hash);
    return true;
}

void RelayState::AssignKeySixteenthAndQuarterHoldersWhoJoinedLater(Relay &relay, uint160 encoding_message_hash)
{
    uint64_t key_part_holder = KeyPartHolderWhoJoinedLaterAndIsntAlreadyBeingUsed(relay, encoding_message_hash);
    relay.key_quarter_holders.push_back(key_part_holder);
    key_part_holder = KeyPartHolderWhoJoinedLaterAndIsntAlreadyBeingUsed(relay, encoding_message_hash);
    relay.first_set_of_key_sixteenth_holders.push_back(key_part_holder);
    key_part_holder = KeyPartHolderWhoJoinedLaterAndIsntAlreadyBeingUsed(relay, encoding_message_hash);
    relay.second_set_of_key_sixteenth_holders.push_back(key_part_holder);
}

uint64_t RelayState::NumberOfRelaysThatJoinedLaterThan(Relay &relay)
{
    uint64_t count = 0;
    for (uint64_t n = relays.size(); n > 0 and relays[n - 1].number > relay.number; n--)
        count++;
    return count;
}

uint64_t RelayState::NumberOfRelaysThatJoinedBefore(Relay &relay)
{
    uint64_t count = 0;
    for (uint64_t n = 0; n < relays.size() and relays[n].number < relay.number; n++)
        count++;
    return count;
}

std::vector<uint64_t> RelayState::GetKeyQuarterHoldersAsListOf16RelayNumbers(uint64_t relay_number)
{
    Relay *relay = GetRelayByNumber(relay_number);
    if (relay == NULL) throw RelayStateException("no such relay");
    std::vector<uint64_t> recipient_relay_numbers;
    for (auto quarter_key_holder : relay->key_quarter_holders)
    {
        for (uint64_t i = 0; i < 4; i++)
            recipient_relay_numbers.push_back(quarter_key_holder);
    }
    return recipient_relay_numbers;
}

std::vector<uint64_t>
RelayState::GetKeySixteenthHoldersAsListOf16RelayNumbers(uint64_t relay_number, uint64_t first_or_second_set)
{
    Relay *relay = GetRelayByNumber(relay_number);
    if (relay == NULL) throw RelayStateException("no such relay");

    return (first_or_second_set == 1) ? relay->first_set_of_key_sixteenth_holders
                                                                   : relay->second_set_of_key_sixteenth_holders;
}

void RelayState::ProcessKeyDistributionMessage(KeyDistributionMessage key_distribution_message)
{
    Relay *relay = GetRelayByJoinHash(key_distribution_message.relay_join_hash);
    if (relay == NULL) throw RelayStateException("no such relay");
    if (relay->key_distribution_message_hash != 0)
        throw RelayStateException("key distribution message for relay has already been processed");
    relay->key_distribution_message_hash = key_distribution_message.GetHash160();
}

void RelayState::EnsureMapsAreUpToDate()
{
    if (maps_are_up_to_date)
        return;
    relays_by_number.clear();
    relays_by_join_hash.clear();
    for (Relay &relay : relays)
    {
        relays_by_number[relay.number] = &relay;
        relays_by_join_hash[relay.join_message_hash] = &relay;
    }
    maps_are_up_to_date = true;
}

void RelayState::UnprocessRelayJoinMessage(RelayJoinMessage relay_join_message)
{
    if (relays.size() == 0 or relays.back().join_message_hash != relay_join_message.GetHash160())
        throw RelayStateException("trying to unprocess join message out of sequence");
    relays.pop_back();
    maps_are_up_to_date = false;
}

void RelayState::UnprocessKeyDistributionMessage(KeyDistributionMessage key_distribution_message)
{
    Relay *relay = GetRelayByJoinHash(key_distribution_message.relay_join_hash);
    if (relay == NULL or relay->key_distribution_message_hash != key_distribution_message.GetHash160())
        throw RelayStateException("trying to unprocess a key distribution message which was not processed");
    relay->key_distribution_message_hash = 0;
}

uint64_t RelayState::AssignSuccessorToRelay(Relay *relay)
{
    uint160 random_seed = relay->GetSeedForDeterminingSuccessor();
    Relay *chosen_successor{NULL};
    bool found = false;

    while (not found)
    {
        random_seed = HashUint160(random_seed);
        chosen_successor = &relays[(CBigNum(random_seed) % CBigNum(relays.size())).getulong()];
        if (SuccessorToRelayIsSuitable(chosen_successor, relay))
            found = true;
    }
    return chosen_successor->number;
}

bool RelayState::SuccessorToRelayIsSuitable(Relay *chosen_successor, Relay *relay)
{
    if (chosen_successor->number == relay->number)
        return false;
    if (VectorContainsEntry(relay->key_quarter_holders, chosen_successor->number))
        return false;
    std::vector<uint64_t> key_quarter_sharers_of_relay = RelayNumbersOfRelaysWhoseKeyQuartersAreHeldByGivenRelay(relay);
    if (VectorContainsEntry(key_quarter_sharers_of_relay, chosen_successor->number))
        return false;
    for (auto key_quarter_sharer_number : key_quarter_sharers_of_relay)
        if (VectorContainsEntry(chosen_successor->key_quarter_holders, key_quarter_sharer_number))
            return false;
    return true;
}

std::vector<uint64_t> RelayState::RelayNumbersOfRelaysWhoseKeyQuartersAreHeldByGivenRelay(Relay *given_relay)
{
    std::vector<uint64_t> relays_with_key_quarters_held;

    for (Relay &relay : relays)
        if (VectorContainsEntry(relay.key_quarter_holders, given_relay->number))
            relays_with_key_quarters_held.push_back(relay.number);

    return relays_with_key_quarters_held;
}

Obituary RelayState::GenerateObituary(Relay *relay, uint32_t reason_for_leaving)
{
    Obituary obituary;
    obituary.relay = *relay;
    obituary.reason_for_leaving = reason_for_leaving;
    obituary.relay_state_hash = GetHash160();
    obituary.in_good_standing =
            reason_for_leaving == OBITUARY_SAID_GOODBYE and RelayIsOldEnoughToLeaveInGoodStanding(relay);
    obituary.successor_number = AssignSuccessorToRelay(relay);
    return obituary;
}

bool RelayState::RelayIsOldEnoughToLeaveInGoodStanding(Relay *relay)
{
    uint64_t latest_relay_number = relays.back().number;
    return latest_relay_number > relay->number + 1000;
}

void RelayState::ProcessObituary(Obituary obituary)
{
    Relay *relay = GetRelayByNumber(obituary.relay.number);
    if (relay == NULL) throw RelayStateException("no such relay");
    relay->obituary_hash = obituary.GetHash160();

    if (obituary.reason_for_leaving != OBITUARY_SAID_GOODBYE)
        for (auto key_quarter_holder_relay_number : relay->key_quarter_holders)
        {
            Relay *key_quarter_holder = GetRelayByNumber(key_quarter_holder_relay_number);
            key_quarter_holder->tasks.push_back(relay->obituary_hash);
        }
}

void RelayState::UnprocessObituary(Obituary obituary)
{
    Relay *relay = GetRelayByNumber(obituary.relay.number);
    if (relay == NULL) throw RelayStateException("no such relay");

    if (obituary.reason_for_leaving != OBITUARY_SAID_GOODBYE)
        for (auto key_quarter_holder_relay_number : relay->key_quarter_holders)
        {
            Relay *key_quarter_holder = GetRelayByNumber(key_quarter_holder_relay_number);
            EraseEntryFromVector(relay->obituary_hash, key_quarter_holder->tasks);
        }
    relay->obituary_hash = 0;
}

RelayExit RelayState::GenerateRelayExit(Relay *relay)
{
    RelayExit relay_exit;
    relay_exit.obituary_hash = relay->obituary_hash;
    relay_exit.secret_recovery_message_hashes = relay->secret_recovery_message_hashes;
    return relay_exit;
}

void RelayState::ProcessRelayExit(RelayExit relay_exit, Data data)
{
    Obituary obituary = data.msgdata[relay_exit.obituary_hash]["obituary"];
    if (not data.msgdata[relay_exit.obituary_hash].HasProperty("obituary"))
        throw RelayStateException("no record of obituary specified in relay exit");
    if (GetRelayByNumber(obituary.relay.number) == NULL)
        throw RelayStateException("no relay with specified number to remove");
    TransferTasks(obituary.relay.number, obituary.successor_number);
    RemoveRelay(obituary.relay.number, obituary.successor_number);
}

void RelayState::UnprocessRelayExit(RelayExit relay_exit, Data data)
{
    Obituary obituary = data.msgdata[relay_exit.obituary_hash]["obituary"];
    if (not data.msgdata[relay_exit.obituary_hash].HasProperty("obituary"))
        throw RelayStateException("no record of obituary specified in relay exit");
    ReaddRelay(obituary, obituary.successor_number);
    TransferTasksBackFromSuccessorToRelay(obituary);
}

void RelayState::TransferTasks(uint64_t dead_relay_number, uint64_t successor_number)
{
    Relay *successor = GetRelayByNumber(successor_number);
    Relay *dead_relay = GetRelayByNumber(dead_relay_number);
    if (dead_relay == NULL or successor == NULL) throw RelayStateException("no such relay");

    successor->tasks = ConcatenateVectors(successor->tasks, dead_relay->tasks);
}

void RelayState::TransferTasksBackFromSuccessorToRelay(Obituary obituary)
{
    Relay *successor = GetRelayByNumber(obituary.successor_number);
    Relay *dead_relay = GetRelayByNumber(obituary.relay.number);
    if (dead_relay == NULL or successor == NULL) throw RelayStateException("no such relay");

    for (auto task_hash : obituary.relay.tasks)
    {
        if (VectorContainsEntry(successor->tasks, task_hash))
            EraseEntryFromVector(task_hash, successor->tasks);
        if (not VectorContainsEntry(dead_relay->tasks, task_hash))
            dead_relay->tasks.push_back(task_hash);
    }
}

void RelayState::RemoveRelay(uint64_t exiting_relay_number, uint64_t successor_relay_number)
{
    for (uint32_t i = 0; i < relays.size(); i++)
    {
        if (relays[i].number == exiting_relay_number)
            relays.erase(relays.begin() + i);
        for (uint32_t j = 0; j < relays[i].key_quarter_holders.size(); j++)
            if (relays[i].key_quarter_holders[j] == exiting_relay_number)
                relays[i].key_quarter_holders[j] = successor_relay_number;
    }
    maps_are_up_to_date = false;
}

void RelayState::ReaddRelay(Obituary obituary, uint64_t successor_relay_number)
{
    for (uint32_t i = 0; i < relays.size(); i++)
        if (relays[i].number > obituary.relay.number)
        {
            relays.insert(relays.begin() + i, obituary.relay);
            break;
        }
    maps_are_up_to_date = false;
}

void RelayState::ProcessKeyDistributionComplaint(KeyDistributionComplaint complaint, Data data)
{
    Relay *secret_sender = complaint.GetSecretSender(data);
    if (secret_sender == NULL) throw RelayStateException("no such relay");

    if (secret_sender->key_distribution_message_accepted)
        throw RelayStateException("too late to process complaint");

    RecordRelayDeath(secret_sender, data, OBITUARY_COMPLAINT);
}

void RelayState::UnprocessKeyDistributionComplaint(KeyDistributionComplaint complaint, Data data)
{
    Relay *secret_sender = complaint.GetSecretSender(data);
    if (secret_sender == NULL) throw RelayStateException("no such relay");

    UnrecordRelayDeath(secret_sender, data, OBITUARY_COMPLAINT);
}

void RelayState::ProcessDurationWithoutResponse(DurationWithoutResponse duration, Data data)
{
    std::string message_type = data.msgdata[duration.message_hash]["type"];

    if (message_type == "key_distribution")
        ProcessDurationAfterKeyDistributionMessage(data.msgdata[duration.message_hash][message_type], data);

    else if (message_type == "goodbye")
        ProcessDurationAfterGoodbyeMessage(data.msgdata[duration.message_hash][message_type], data);

    else if (message_type == "secret_recovery")
        ProcessDurationAfterSecretRecoveryMessage(data.msgdata[duration.message_hash][message_type], data);
}

void RelayState::UnprocessDurationWithoutResponse(DurationWithoutResponse duration, Data data)
{
    std::string message_type = data.msgdata[duration.message_hash]["type"];

    if (message_type == "key_distribution")
        UnprocessDurationAfterKeyDistributionMessage(data.msgdata[duration.message_hash][message_type], data);

    else if (message_type == "goodbye")
        UnprocessDurationAfterGoodbyeMessage(data.msgdata[duration.message_hash][message_type], data);

    else if (message_type == "secret_recovery")
        UnprocessDurationAfterSecretRecoveryMessage(data.msgdata[duration.message_hash][message_type], data);
}

void RelayState::ProcessDurationAfterKeyDistributionMessage(KeyDistributionMessage key_distribution_message, Data data)
{
    Relay *relay = GetRelayByJoinHash(key_distribution_message.relay_join_hash);
    if (relay == NULL or relay->obituary_hash != 0) throw RelayStateException("non-existent or dead relay");
    relay->key_distribution_message_accepted = true;
}

void RelayState::UnprocessDurationAfterKeyDistributionMessage(KeyDistributionMessage key_distribution_message, Data data)
{
    Relay *relay = GetRelayByJoinHash(key_distribution_message.relay_join_hash);
    if (relay == NULL) throw RelayStateException("no such relay");
    relay->key_distribution_message_accepted = false;
}

void RelayState::ProcessGoodbyeMessage(GoodbyeMessage goodbye)
{
    Relay *relay = GetRelayByNumber(goodbye.dead_relay_number);
    if (relay == NULL) throw RelayStateException("no such relay");
    relay->goodbye_message_hash = goodbye.GetHash160();
}

void RelayState::UnprocessGoodbyeMessage(GoodbyeMessage goodbye)
{
    Relay *relay = GetRelayByNumber(goodbye.dead_relay_number);
    if (relay == NULL) throw RelayStateException("no such relay");
    relay->goodbye_message_hash = 0;
}

void RelayState::ProcessGoodbyeComplaint(GoodbyeComplaint complaint, Data data)
{
    Relay *dead_relay = complaint.GetSecretSender(data);

    if (dead_relay == NULL) throw RelayStateException("no such relay");

    RecordRelayDeath(dead_relay, data, OBITUARY_COMPLAINT);
}

void RelayState::UnprocessGoodbyeComplaint(GoodbyeComplaint complaint, Data data)
{
    Relay *dead_relay = complaint.GetSecretSender(data);

    if (dead_relay == NULL) throw RelayStateException("no such relay");

    UnrecordRelayDeath(dead_relay, data, OBITUARY_COMPLAINT);
}

void RelayState::ProcessDurationAfterGoodbyeMessage(GoodbyeMessage goodbye, Data data)
{
    Relay *dead_relay = data.relay_state->GetRelayByNumber(goodbye.dead_relay_number);
    if (dead_relay == NULL) throw RelayStateException("no such relay");
    RecordRelayDeath(dead_relay, data, OBITUARY_SAID_GOODBYE);
}

void RelayState::UnprocessDurationAfterGoodbyeMessage(GoodbyeMessage goodbye, Data data)
{
    Relay *dead_relay = data.relay_state->GetRelayByNumber(goodbye.dead_relay_number);
    if (dead_relay == NULL) throw RelayStateException("no such relay");
    UnrecordRelayDeath(dead_relay, data, OBITUARY_SAID_GOODBYE);
}

void RelayState::ProcessSecretRecoveryMessage(SecretRecoveryMessage secret_recovery_message)
{
    Relay *dead_relay = GetRelayByNumber(secret_recovery_message.dead_relay_number);
    Relay *quarter_holder = GetRelayByNumber(secret_recovery_message.quarter_holder_number);
    Relay *recipient = GetRelayByNumber(secret_recovery_message.successor_number);
    if (dead_relay == NULL  or quarter_holder == NULL or recipient == NULL)
        throw RelayStateException("secret recovery message refers to non-existent relay");

    dead_relay->secret_recovery_message_hashes.push_back(secret_recovery_message.GetHash160());
}

void RelayState::UnprocessSecretRecoveryMessage(SecretRecoveryMessage secret_recovery_message)
{
    Relay *dead_relay = GetRelayByNumber(secret_recovery_message.dead_relay_number);
    Relay *quarter_holder = GetRelayByNumber(secret_recovery_message.quarter_holder_number);
    Relay *recipient = GetRelayByNumber(secret_recovery_message.successor_number);
    if (dead_relay == NULL  or quarter_holder == NULL or recipient == NULL)
        throw RelayStateException("secret recovery message refers to non-existent relay");
    if (not VectorContainsEntry(dead_relay->secret_recovery_message_hashes, secret_recovery_message.GetHash160()))
        throw RelayStateException("attempt to unprocess secret recovery message that wasn't processed");
    EraseEntryFromVector(secret_recovery_message.GetHash160(), dead_relay->secret_recovery_message_hashes);
}

void RelayState::ProcessDurationAfterSecretRecoveryMessage(SecretRecoveryMessage secret_recovery_message, Data data)
{
    Relay *key_quarter_holder = secret_recovery_message.GetKeyQuarterHolder(data);
    if (key_quarter_holder == NULL)
        throw RelayStateException("no such relay");
    EraseEntryFromVector(secret_recovery_message.obituary_hash, key_quarter_holder->tasks);
}

void RelayState::UnprocessDurationAfterSecretRecoveryMessage(SecretRecoveryMessage secret_recovery_message, Data data)
{
    Relay *key_quarter_holder = secret_recovery_message.GetKeyQuarterHolder(data);
    if (key_quarter_holder == NULL)
        throw RelayStateException("no such relay");
    key_quarter_holder->tasks.push_back(secret_recovery_message.obituary_hash);
}

void RelayState::ProcessSecretRecoveryComplaint(SecretRecoveryComplaint complaint, Data data)
{
    Relay *key_quarter_holder = complaint.GetSecretSender(data);
    RecordRelayDeath(key_quarter_holder, data, OBITUARY_COMPLAINT);
}

void RelayState::UnprocessSecretRecoveryComplaint(SecretRecoveryComplaint complaint, Data data)
{
    Relay *key_quarter_holder = complaint.GetSecretSender(data);
    UnrecordRelayDeath(key_quarter_holder, data, OBITUARY_COMPLAINT);
}

void RelayState::ProcessDurationAfterSecretRecoveryComplaint(SecretRecoveryComplaint complaint, Data data)
{
    Relay *key_quarter_holder = complaint.GetSecretSender(data);
}

void RelayState::UnprocessDurationAfterSecretRecoveryComplaint(SecretRecoveryComplaint complaint, Data data)
{
    Relay *key_quarter_holder = complaint.GetSecretSender(data);
}

void RelayState::RecordRelayDeath(Relay *dead_relay, Data data, uint32_t reason)
{
    Obituary obituary = GenerateObituary(dead_relay, reason);
    data.StoreMessage(obituary);
    ProcessObituary(obituary);
}

void RelayState::UnrecordRelayDeath(Relay *dead_relay, Data data, uint32_t reason)
{
    Obituary obituary = data.GetMessage(dead_relay->obituary_hash);
    UnprocessObituary(obituary);
}

void RelayState::ProcessDurationWithoutResponseFromRelay(DurationWithoutResponseFromRelay duration, Data data)
{
    std::string message_type = data.msgdata[duration.message_hash]["type"];

    if (message_type == "obituary")
    {
        Obituary obituary = data.msgdata[duration.message_hash][message_type];
        ProcessDurationWithoutRelayResponseAfterObituary(obituary, duration.relay_number, data);
    }
}

void RelayState::UnprocessDurationWithoutResponseFromRelay(DurationWithoutResponseFromRelay duration, Data data)
{
    std::string message_type = data.msgdata[duration.message_hash]["type"];

    if (message_type == "obituary")
    {
        Obituary obituary = data.msgdata[duration.message_hash][message_type];
        UnprocessDurationWithoutRelayResponseAfterObituary(obituary, duration.relay_number, data);
    }
}

void RelayState::ProcessDurationWithoutRelayResponseAfterObituary(Obituary obituary, uint64_t relay_number, Data data)
{
    Relay *key_quarter_holder = GetRelayByNumber(relay_number);
    RecordRelayDeath(key_quarter_holder, data, OBITUARY_NOT_RESPONDING);
}

void RelayState::UnprocessDurationWithoutRelayResponseAfterObituary(Obituary obituary, uint64_t relay_number, Data data)
{
    Relay *key_quarter_holder = GetRelayByNumber(relay_number);
    UnrecordRelayDeath(key_quarter_holder, data, OBITUARY_NOT_RESPONDING);
}
