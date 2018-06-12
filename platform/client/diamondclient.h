// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// vim: set ts=4 sw=4:
/***********************************************************************
 *
 * client/diamondclient.h:
 *   Diamond transactional store interface
 *
 * Copyright 2015 Irene Zhang  <iyzhang@cs.washington.edu>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************************/
 
#ifndef _DIAMOND_CLIENT_H_
#define _DIAMOND_CLIENT_H_

#include "tapir/lib/assert.h"
#include "tapir/lib/message.h"
#include "tapir/lib/configuration.h"
#include "tapir/lib/tcptransport.h"
#include "frontend/client.h"
#include "tapir/store/common/frontend/client.h"
#include "store/common/frontend/cacheclient.h"
#include "tapir/store/common/notification.h"
#include "tapir/store/common/increment.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <set>
#include <thread>
#include <functional>

namespace diamond {

class DiamondClient : public Client
{
public:
    DiamondClient(std::string configPath);
    DiamondClient(const std::string &hostname, const std::string &port);
    ~DiamondClient();

    // Overriding functions from ::Client
    void Begin();
    void BeginRO();
    int Get(const std::string &key, std::string &value);
    int Put(const std::string &key, const std::string &value);
    int Increment(const std::string &key, const Increment inc);
    bool Commit();
    void Abort();

    void BeginReactive(uint64_t reactive_id);
    uint64_t GetNextNotification(bool blocking);
    void Unsubscribe(uint64_t reactive_id);

    void SetIsolationLevel(int isolationLevel);
    void SetCaching(bool cachingEnabled);
    void Notify(std::function<void (void)> callback,
                const uint64_t reactive_id,
                const Timestamp timestamp,
                const std::map<std::string, VersionedValue> &values);
    void NotificationInit(std::function<void (void)> callback);
private:    
    /* Private helper functions. */
    void run_client(); // Runs the transport event loop.

    // helper methods for the constructors (to avoid duplicated code)
    void initState();
    void startTransport(frontend::Client *frontendclient);

    static void * startHelper(void * arg); // method to be called by pthread_create()

    // Unique ID for this client.
    uint64_t client_id;

    // transaction id counter
    uint64_t txnid_counter;
    // its lock
    std::mutex txnid_lock;
    
    // Transport used by storage client proxies.
    TCPTransport transport;
    
    // Thread running the transport event loop.
    pthread_t clientTransport;

    // Caching client for the store
    CacheClient *client;

    // reactive_id to registration set map
    std::unordered_map<uint64_t, std::set<std::string> > regMap; 
    
    // map reactive_id to timestamp of last notification received
    std::map<uint64_t, Timestamp> timestamp_map;
    // Timestamp at which the last notification was received
    Timestamp last_notification_ts; 

    // Notification client state
    std::queue<uint64_t> pending;
    std::mutex lock;
    std::condition_variable cv;

};

} // namespace diamond

#endif /* _DIAMOND_CLIENT_H_ */
