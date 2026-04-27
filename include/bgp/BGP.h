/*declares concrete BGP class inheriting from Policy
 * owns local RIB (hashmap of prefix -> best announcement) and received queue (hasmap of prefix -> list of incoming announcments)
 * contains tie breaker logic for conflict resolution
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "bgp/Policy.h"
#include "bgp/Announcement.h"

class BGP : public Policy {
public:
    BGP(uint32_t ownerAsn) : ownerAsn_(ownerAsn){}
    BGP() = default;


    void receiveAnnouncement(const Announcement& ann) override;
    void processQueue() override;
    bool hasPrefix(const std::string& prefix) const override;
    const Announcement* getAnnouncement(const std::string& prefix) const override;
    const std::unordered_map<std::string, Announcement>& getRIB() const override;

    void seedDirectly(const Announcement& ann) override;

protected:
    uint32_t ownerAsn_;
    // the best known announcement per prefix
    std::unordered_map<std::string, Announcement> localRIB_;

    // all received announcements waiting to be processed, grouped by prefix
    std::unordered_map<std::string, std::vector<Announcement>> recvQueue_;

    // returns true if newAnn is preferred over existing
    bool isBetter(const Announcement& newAnn, const Announcement& existing) const;
};
