/* implements core simulation logic
 * processing recieved queue
 * running tiebreaker comparison to decide whether to replace local RIb entry
 * sending announcement to neighbors (setting correct next_hop and prepending ASN to the path)
 */

#include "bgp/BGP.h"
#include <iostream>

void BGP::receiveAnnouncement(const Announcement& ann) {
    // add to the received queue under this prefix
    recvQueue_[ann.prefix_].push_back(ann);
}

void BGP::processQueue() {
    for (auto& [prefix, announcements] : recvQueue_) {
        for (const auto& ann : announcements) {
            Announcement toStore = ann;
            toStore.asPath_.insert(toStore.asPath_.begin(), ownerAsn_);

            if (localRIB_.find(prefix) == localRIB_.end()) {
                if(ownerAsn_ == 15830){
                    std::cout << "AS 15830 FIRST STORE: "
                              << " size =" << toStore.asPath_.size()
                              << " recvFrom=" << (int)toStore.recvFrom_
                              << " nextHope=" << toStore.nextHop_ << std::endl;
                }

                localRIB_.emplace(prefix, toStore);
            } else {
                // DEBUG - print when tiebreaker fires for AS 15830
                if (ownerAsn_ == 15830) {
                    const auto& existing = localRIB_.at(prefix);
                    std::cout << "AS 15830 tiebreaker:" << std::endl;
                    std::cout << "  existing path size=" << existing.asPath_.size()
                              << " recvFrom=" << (int)existing.recvFrom_
                              << " nextHop=" << existing.nextHop_ << std::endl;
                    std::cout << "  incoming path size=" << toStore.asPath_.size()
                              << " recvFrom=" << (int)toStore.recvFrom_
                              << " nextHop=" << toStore.nextHop_ << std::endl;
                    std::cout << "  isBetter=" << isBetter(toStore, existing) << std::endl;
                }
                if (isBetter(toStore, localRIB_.at(prefix))) {
                    localRIB_.at(prefix) = toStore;
                }
            }
        }
    }
    recvQueue_.clear();
}

bool BGP::hasPrefix(const std::string& prefix) const {
    return localRIB_.find(prefix) != localRIB_.end();
}

const Announcement* BGP::getAnnouncement(const std::string& prefix) const {
    auto it = localRIB_.find(prefix);
    if (it == localRIB_.end()) {
        return nullptr;
    }
    return &it->second;
}

const std::unordered_map<std::string, Announcement>& BGP::getRIB() const {
    return localRIB_;
}


bool BGP::isBetter(const Announcement& newAnn, const Announcement& existing) const {
    // step 1 - relationship priority: ORIGIN > CUSTOMER > PEER > PROVIDER
    auto relationshipRank = [](Relationship r) -> int {
        switch (r) {
            case Relationship::ORIGIN:   return 4;
            case Relationship::CUSTOMER: return 3;
            case Relationship::PEER:     return 2;
            case Relationship::PROVIDER: return 1;
            default:                     return 0;
        }
    };

    int newRank = relationshipRank(newAnn.recvFrom_);
    int existingRank = relationshipRank(existing.recvFrom_);

    if (newRank != existingRank) {
        return newRank > existingRank;
    }

    // step 2 - shorter AS path wins
    if (newAnn.asPath_.size() != existing.asPath_.size()) {
        return newAnn.asPath_.size() < existing.asPath_.size();
    }

    // step 3 - lower next-hop ASN wins
    return newAnn.nextHop_ < existing.nextHop_;
}

void BGP::seedDirectly(const Announcement& ann){
    localRIB_.emplace(ann.prefix_, ann);
}
