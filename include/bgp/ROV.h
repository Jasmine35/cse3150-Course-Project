/* declares ROV - inherits from BGP and overrides only the part where announcements are accepted
 * drops any with rov_invalud == true before it reaches the local RIB
 */

# pragma once
# include "bgp/BGP.h"

class ROV : public BGP {
public:
    ROV(uint32_t ownerAsn) : BGP(ownerAsn) {}

    // only override receiveAnnouncement - drop rov_invalid announcements
    void receiveAnnouncement(const Announcement& ann) override;
};
