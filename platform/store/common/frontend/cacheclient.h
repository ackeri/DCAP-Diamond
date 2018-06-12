// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// vim: set ts=4 sw=4:
/***********************************************************************
 *
 * store/common/frontend/cacheclient.h:
 *   Single shard caching client implementation.
 *
 * Copyright 2015 Irene Zhang <iyzhang@cs.washington.edu>
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

#ifndef _CACHE_CLIENT_H_
#define _CACHE_CLIENT_H_

#include "includes/error.h"
#include "tapir/lib/assert.h"
#include "tapir/lib/message.h"
#include "tapir/store/common/transaction.h"
#include "tapir/store/common/promise.h"
#include "tapir/store/common/frontend/txnclient.h"
#include "tapir/store/common/backend/versionstore.h"

#include <string>

class CacheClient : public TxnClient {

public:
    CacheClient(TxnClient *txnclient);
    virtual ~CacheClient();

    virtual void Begin(const uint64_t tid);
    virtual void BeginRO(const uint64_t tid,
                 const Timestamp timestamp = Timestamp());
	
	virtual void Get(uint64_t tid,
                     const std::string &key,
	                 Promise *promise = NULL);

	virtual void Get(uint64_t tid,
                     const std::string &key,
					 const Timestamp &t = Timestamp(),
	                 Promise *promise = NULL);

    virtual void MultiGet(const uint64_t tid,
                          const std::set<std::string> &key,
                          const Timestamp timestamp = Timestamp(),
                          Promise *promise = NULL);

    // Set the value for the given key.
    virtual void Put(const uint64_t tid,
                     const std::string &key,
                     const std::string &value,
                     Promise *promise = NULL);

    // Prepare the transaction.
    virtual void Prepare(const uint64_t tid,
                         const Transaction &txn = Transaction(),
	                     const Timestamp &t = Timestamp(),
                         Promise *promise = NULL);

    // Commit all Get(s) and Put(s) since Begin().
    virtual void Commit(const uint64_t tid,
                        const Transaction &txn = Transaction(),
	                    const Timestamp &t = Timestamp(),
                        Promise *promise = NULL);
    
    // Abort all Get(s) and Put(s) since Begin().
    virtual void Abort(const uint64_t tid,
                       const Transaction &txn = Transaction(),
                       Promise *promise = NULL);

    // Subscribe to notifications
    virtual void Subscribe(const uint64_t reactive_id,
                           const std::set<std::string> &keys,
                           const Timestamp timestamp,
                           Promise *promise = NULL);

    virtual void Unsubscribe(const uint64_t reactive_id,
                             const std::set<std::string> &keys,
                             Promise *promise = NULL);

    virtual void Ack(const uint64_t reactive_id,
                     const std::set<std::string> &keys,
                     const Timestamp timestamp,
                     Promise *promise = NULL);

    virtual void SetCaching(bool cachingEnabled);

    virtual void SetNotify(notification_handler_t notify);

    virtual void Notify(notification_handler_t notify,
                        const uint64_t reactive_id,                
                        const Timestamp timestamp,
                        const std::map<std::string, VersionedValue> &values);

protected:
    // Underlying single shard transaction client implementation.
    TxnClient* txnclient;

     // Read cache
    VersionedKVStore cache;
    std::mutex cache_lock;
    std::map<uint64_t, Transaction> prepared;

    // Flag controlling whether caching is used on Get and Multiget
    bool cachingEnabled;


};

#endif /* _CACHE_CLIENT_H_ */
