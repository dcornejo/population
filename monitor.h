//
// Created by dave on 3/4/2024.
//

#ifndef HOSTMON_MONITOR_H
#define HOSTMON_MONITOR_H

#include <chrono>

class participant;

enum ParticipantStatus {
    PEXISTS,
    PADDED
};

std::uint64_t get_timestamp ();
ParticipantStatus report_participant (const participant &p);

#endif //HOSTMON_MONITOR_H
