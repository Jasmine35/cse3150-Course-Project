/* implement ROV override
 * checks rov_invalid on each incoming announcement and drops it immediately if true
 * otherwise it delegates to the parent BGP logic
 */

#include "bgp/ROV.h"

void ROV::receiveAnnouncement(const Announcement& ann) {
    // drop the announcement entirely if it is marked as invalid
    if (ann.rovInvalid_) {
        return;
    }
    // otherwise delegate to normal BGP behavior
    BGP::receiveAnnouncement(ann);
}
