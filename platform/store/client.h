// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// vim: set ts=4 sw=4:
/***********************************************************************
 *
 * store/strongstore/client.h:
 *   Transactional client interface.
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
 
#ifndef _STRONG_CLIENT_H_
#define _STRONG_CLIENT_H_

#include "tapir/lib/assert.h"
#include "tapir/lib/message.h"
#include "tapir/lib/configuration.h"
#include "tapir/lib/tcptransport.h"
#include "tapir/store/common/frontend/txnclient.h"
#include "tapir/store/common/frontend/client.h"
#include "tapir/replication/ir/client.h"
#include "store/common/frontend/cacheclient.h"
#include "store-proto.pb.h"
#include "tapir/store/tapirstore/shardclient.h"
#include "tapir/timeserver/timeserver.h"
#include "store/common/frontend/asyncclient.h"

#include <string>
#include <set>
#include <thread>
#include <vector>

namespace strongstore {

class Client : public AsyncClient
{
public:
    Client(const string configPath,
           const int nshards,
           const int closestReplica,
           Transport *transport);
    ~Client();

    virtual void Get(const uint64_t tid,
                          const std::string &key,
                          callback_t callback,
                          const Timestamp timestamp = Timestamp());

    // Callback based async call
    virtual void Commit(const uint64_t tid,
                        callback_t callback,
                        const Transaction &txn);

    virtual void Abort(const uint64_t tid,
                       callback_t callback);
    
    std::vector<int> Stats();

    virtual void Subscribe(const uint64_t reactive_id,
                           const std::set<std::string> &keys,
                           const Timestamp timestamp,
                           callback_t callback);

    virtual void Unsubscribe(const uint64_t reactive_id,
                             const std::set<std::string> &keys,
                             callback_t callback);

    virtual void Ack(const uint64_t reactive_id,
                     const std::set<std::string> &keys,
                     const Timestamp timestamp,
                     callback_t callback);

    virtual void SetPublish(publish_handler_t publish);
private:
    // Callback functions
    void GetCallback(callback_t callback,
                          Promise &promise);
    void PrepareCallback(callback_t callback,
                         const size_t total,
			 std::vector<int> *results,
                         const uint64_t *ts,
                         const bool isTSS,
			 Promise &promise);
    void tssCallback(callback_t callback,
                     size_t total,
		     std::vector<int> *results,
                     uint64_t *ts,
		     const string &request,
                     const string &reply);
    void CommitCallback(uint64_t tid,
			callback_t callback,
			std::map<int, Transaction> participants,
			Promise &promise);
    // local Abort function
    void AbortInternal(const uint64_t tid,
		       const std::map<int, Transaction> &participants);

    // local Prepare function
    void PrepareInternal(const uint64_t tid,
			 const std::map<int, Transaction> &participants,
			 callback_t callback);

    // Sharding logic: Given key, generates a number b/w 0 to nshards-1
    uint64_t key_to_shard(const std::string &key,
                          const uint64_t nshards);

    // Unique ID for this client.
    uint64_t client_id;

    // index of closest replica to read 
    int closestReplica = 0;
    
    // Number of shards in SpanStore.
    long nshards;

    // Transport used by paxos client proxies.
    Transport *transport;

    // Caching client for each shard.
    std::vector<AsyncClient *> cclient;

    // Timestamp server shard.
	TimeStampServer *tss; 

    // Synchronization variables.
    std::condition_variable cv;
    std::mutex cv_m;
    string replica_reply;
};

} // namespace strongstore

#endif /* _STRONG_CLIENT_H_ */
