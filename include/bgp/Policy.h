/* declares abstract base class that EVERY AS points to
 * defines interface (process announcement, store, send, etc) that both BGP and ROV must implement
 * lets you swap polcies per-AS without the graph logic caring whcih one its talking to*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "bgp/Announcement.h"

class Policy {
public:
    virtual ~Policy() = default;

    // add an announcement to the received queue for later processing
    virtual void receiveAnnouncement(const Announcement& ann) = 0;

    // process received queue - decide what goes into the local RIB
    virtual void processQueue() = 0;

    // check if the local RIB has an announcement for a given prefix
    virtual bool hasPrefix(const std::string& prefix) const = 0;

    // get the best announcement for a prefix from the local RIB
    virtual const Announcement* getAnnouncement(const std::string& prefix) const = 0;

    // get everything in the local RIB for propagation
    virtual const std::unordered_map<std::string, Announcement>& getRIB() const = 0;

    virtual void seedDirectly(const Announcement& ann) = 0;
};
