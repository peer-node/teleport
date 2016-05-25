#include "Calendar.h"
#include "CreditSystem.h"
#include <boost/range/adaptor/reversed.hpp>

Calendar::Calendar() { }

Calendar::Calendar(uint160 credit_hash, CreditSystem *credit_system)
{
    PopulateDiurn(credit_hash, credit_system);
    PopulateCalends(credit_hash, credit_system);
    PopulateTopUpWork(credit_hash, credit_system);
}

uint160 Calendar::CalendWork()
{
    uint160 calend_work = 0;
    for (auto calend : calends)
        calend_work += calend.mined_credit.network_state.diurnal_difficulty;
    return calend_work;
}

std::vector<uint160> Calendar::GetCalendCreditHashes()
{
    std::vector<uint160> calend_credit_hashes;
    for (auto calend : calends)
        calend_credit_hashes.push_back(calend.mined_credit.GetHash160());
    return calend_credit_hashes;
}

bool Calendar::ContainsDiurn(uint160 diurn_root)
{
    for (auto calend : calends)
        if (calend.Root() == diurn_root)
            return true;
    return false;
}

uint160 Calendar::LastMinedCreditHash()
{
    if (current_diurn.Size() > 0)
        return current_diurn.Last();
    if (calends.size() > 0)
        return calends.back().mined_credit.GetHash160();
    return 0;
}

MinedCredit Calendar::LastMinedCredit()
{
    if (current_diurn.Size() > 0)
        return current_diurn.credits_in_diurn.back().mined_credit;
    if (calends.size() > 0)
        return calends.back().mined_credit;
    return MinedCredit();
}

void Calendar::PopulateDiurn(uint160 credit_hash, CreditSystem* credit_system)
{
    current_diurn.credits_in_diurn.resize(0);
    current_diurn.diurnal_block = DiurnalBlock();

    std::vector<MinedCreditMessage> msgs;
    MinedCreditMessage msg = credit_system->creditdata[credit_hash]["msg"];

    msgs.push_back(msg);

    while (not credit_system->IsCalend(credit_hash) and credit_hash != 0)
    {
        credit_hash = msg.mined_credit.network_state.previous_mined_credit_hash;
        if (credit_hash == 0)
            break;
        msg = credit_system->creditdata[credit_hash]["msg"];
        msgs.push_back(msg);
    }
    std::reverse(msgs.begin(), msgs.end());
    for (auto msg_ : msgs)
        current_diurn.Add(msg_);
}

void Calendar::PopulateCalends(uint160 credit_hash, CreditSystem *credit_system)
{
    calends.resize(0);
    if (credit_system->IsCalend(credit_hash))
    {
        MinedCreditMessage msg = credit_system->creditdata[credit_hash]["msg"];
        calends.push_back(Calend(msg));
    }
    credit_hash = credit_system->PrecedingCalendCreditHash(credit_hash);
    while (credit_hash != 0)
    {
        MinedCreditMessage msg = credit_system->creditdata[credit_hash]["msg"];
        calends.push_back(Calend(msg));
        credit_hash = credit_system->PrecedingCalendCreditHash(credit_hash);
    }
    std::reverse(calends.begin(), calends.end());
}

void Calendar::PopulateTopUpWork(uint160 credit_hash, CreditSystem *credit_system)
{
    extra_work.resize(0);
    uint160 total_calendar_work = CalendWork() + current_diurn.Work();
    auto latest_network_state = LastMinedCredit().network_state;
    uint160 total_credit_work = latest_network_state.previous_total_work + latest_network_state.difficulty;
    if (calends.size() > 0)
        total_calendar_work -= calends.back().mined_credit.network_state.difficulty;

    uint160 previous_credit_work_so_far = 0;
    for (auto calend : boost::adaptors::reverse(calends))
    {
        uint160 credit_work_so_far = calend.TotalCreditWork();
        uint160 credit_work_in_diurn = credit_work_so_far - previous_credit_work_so_far;
        AddTopUpWorkInDiurn(credit_work_in_diurn, total_credit_work, total_calendar_work, calend, credit_system);
        previous_credit_work_so_far = credit_work_so_far;
    }
    std::reverse(extra_work.begin(), extra_work.end());
}

void Calendar::AddTopUpWorkInDiurn(uint160 credit_work_in_diurn,
                                   uint160 total_credit_work,
                                   uint160& total_calendar_work,
                                   Calend& calend,
                                   CreditSystem* credit_system)
{
    std::vector<MinedCreditMessage> extra_work_in_diurn;
    uint160 calend_work = calend.mined_credit.network_state.diurnal_difficulty;
    EncodedNetworkState network_state = calend.mined_credit.network_state;

    uint160 previous_hash, remaining_credit_work_in_diurn = credit_work_in_diurn;

    while (remaining_credit_work_in_diurn > calend_work && total_credit_work > total_calendar_work)
    {
        previous_hash = network_state.previous_mined_credit_hash;
        if (previous_hash == 0)
            break;
        MinedCreditMessage msg = credit_system->creditdata[previous_hash]["msg"];
        network_state = msg.mined_credit.network_state;
        extra_work_in_diurn.push_back(msg);
        remaining_credit_work_in_diurn -= network_state.difficulty;
        total_calendar_work += network_state.difficulty;
    }
    extra_work.push_back(extra_work_in_diurn);
}

uint160 Calendar::TopUpWork()
{
    uint160 top_up_work = 0;
    uint64_t diurn_number = 0;
    for (auto mined_credit_messages : extra_work)
    {
        for (auto msg : mined_credit_messages)
            top_up_work += msg.mined_credit.network_state.difficulty;
        diurn_number++;
    }
    return top_up_work;
}

uint160 Calendar::TotalWork()
{
    uint160 total_work = CalendWork();
    total_work += TopUpWork();
    total_work += current_diurn.Work();

    if (calends.size() > 0)
    {
        uint160 double_counted_work = calends.back().mined_credit.network_state.difficulty;
        total_work -= double_counted_work;
    }

    return total_work;
}

bool Calendar::CheckExtraWorkRoots()
{
    return false;
}

bool Calendar::CheckCurrentDiurnRoots()
{
    return false;
}

bool Calendar::CheckCalendRoots()
{
    return false;
}

bool Calendar::CheckRoots()
{
    return CheckCalendRoots() and CheckCurrentDiurnRoots() and CheckExtraWorkRoots();
}

bool Calendar::CheckExtraWorkDifficulties()
{
    return false;
}

bool Calendar::CheckCurrentDiurnDifficulties()
{
    return false;
}

bool Calendar::CheckCalendDifficulties()
{
    uint64_t calend_number = 0, previous_timestamp = 0, preceding_timestamp = 0;
    uint160 previous_difficulty = 0;

    for (auto calend : calends)
    {
        uint160 diurnal_difficulty = calend.mined_credit.network_state.diurnal_difficulty;
        std::cout << "calend " << calend_number << " difficulty = " << diurnal_difficulty.ToString() << "\n";

        if (calend_number < 2)
        {
            if (diurnal_difficulty != INITIAL_DIURNAL_DIFFICULTY)
                return false;
        }
        else
        {
            uint64_t diurn_duration = previous_timestamp - preceding_timestamp;
            uint160 correct_difficulty = AdjustDiurnalDifficultyAfterDiurnDuration(previous_difficulty, diurn_duration);
            if (diurnal_difficulty != correct_difficulty)
                return false;
        }

        previous_difficulty = diurnal_difficulty;
        preceding_timestamp = previous_timestamp;
        previous_timestamp = calend.mined_credit.network_state.timestamp;
        calend_number++;
    }
    return true;
}

bool Calendar::CheckDifficulties()
{
    return CheckCalendDifficulties() and CheckCurrentDiurnDifficulties() and CheckExtraWorkDifficulties();
}

bool Calendar::CheckRootsAndDifficulties()
{
    return CheckRoots() and CheckDifficulties();
}


