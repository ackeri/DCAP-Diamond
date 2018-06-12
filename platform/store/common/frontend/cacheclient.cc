// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// vim: set ts=4 sw=4:
/***********************************************************************
 *
 * store/common/frontend/cacheclient.cc:
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

#include "cacheclient.h"

using namespace std;

CacheClient::CacheClient(TxnClient* txnclient)
    : txnclient(txnclient)
{
    this->cachingEnabled = true;
}

CacheClient::~CacheClient() { }

/* Begins a transaction. */
void
CacheClient::Begin(const uint64_t tid)
{
    Debug("BEGIN [%lu]", tid);
    txnclient->Begin(tid);
}

void
CacheClient::BeginRO(const uint64_t tid,
                     const Timestamp timestamp)
{
    Debug("BEGIN [%lu]", tid);
    txnclient->BeginRO(tid, timestamp);
}

void
CacheClient::SetCaching(bool cachingEnabled) {
    this->cachingEnabled = cachingEnabled;
}

void
CacheClient::SetNotify(notification_handler_t notify)
{
    txnclient->SetNotify(bind(&CacheClient::Notify,
                              this,
                              notify,
                              placeholders::_1,
                              placeholders::_2,
                              placeholders::_3));
}

void
CacheClient::Get(const uint64_t tid,
                 const string &key,
                 const Timestamp timestamp,
                 Promise *Promise)
{
	Get(tid, key, promise);
}

void
CacheClient::Get(const uint64_t tid,
                      const string &key,
                      Promise *promise)
{
    if (cachingEnabled) {
        Debug("GET %lu", keys.size());

        cache_lock.lock();

        VersionedValue value;

        if (cache.Get(key, timestamp, value)) {
			if(promise != NULL) {
				promise->Reply(REPLY_OK, value);
				return;
			}
		}

        cache_lock.unlock();
    } else { //!cachingEnabled
        txnclient->Get(tid, key, promise);
        return;
    } 

    Promise p(GET_TIMEOUT);
    Promise *pp = (promise != NULL) ? promise : &p;

    txnclient->Get(tid, key, pp);

    if (pp->GetReply() == REPLY_OK){
        string value = pp->GetValue();
        
        cache_lock.lock();
		Debug("Adding [%s] with to the cache",
				value.c_str());
		cache.Put(key, value);
        cache_lock.unlock();
    }

    if (promise != NULL) {
        promise->Reply(pp->GetReply(), value);
    }
}

/* Set value for a key. (Always succeeds).
 * Returns 0 on success, else -1. */
void
CacheClient::Put(const uint64_t tid,
                 const string &key,
                 const string &value,
                 Promise *promise)
{
    Debug("PUT %s %s", key.c_str(), value.c_str());
    txnclient->Put(tid, key, value, promise);
}

/* Prepare the transaction. */
void
CacheClient::Prepare(const uint64_t tid,
                     const Transaction &txn,
					 const Timestamp &t,
                     Promise *promise)
{
    if (!cachingEnabled) {
        txnclient->Prepare(tid, txn, promise);
        return;
    }
    
    Debug("PREPARE [%lu]", tid);
    Promise p(COMMIT_TIMEOUT);
    Promise *pp =
        (promise != NULL) ? promise : &p;
    
    txnclient->Prepare(tid, txn, pp);
    
    //save the transactions for later
    if (pp->GetReply() == REPLY_OK) {
        cache_lock.lock();
        if (prepared.find(tid) == prepared.end()) {
            prepared[tid] = txn;
        }
        cache_lock.unlock();
    }
}

void
CacheClient::Commit(const uint64_t tid,
                    const Transaction &txn,
					const Timestamp &t,
                    Promise *promise)
{

    if (!cachingEnabled) {
        txnclient->Commit(tid, txn, promise);
        return;
    }
    
    Debug("COMMIT [%lu]", tid);

    Promise p(COMMIT_TIMEOUT);
    Promise *pp = (promise != NULL) ? promise : &p;
    txnclient->Commit(tid, txn, pp);

    if (!cachingEnabled) return;
    // update the cache
    auto it = prepared.find(tid);
    const Transaction t = (it != prepared.end()) ? it->second : txn;
    prepared.erase(tid);
        
    // // update the cache
    cache_lock.lock();
    if (pp->GetReply() == REPLY_OK) {
        for (auto &write : txn.getWriteSet()) {
    	    Debug("Adding write [%s] with ts %lu to the cache",
                  write.first.c_str(),
                  pp->GetTimestamp());
            cache.Put(write.first,
                      write.second,
                      pp->GetTimestamp());
        }

    } else if (pp->GetReply() == REPLY_FAIL) {
        for (auto &read : txn.getReadSet()) {
            Debug("Removing stale [%s] from the cache",
                  read.first.c_str());
            cache.Remove(read.first);
        }
    }
    cache_lock.unlock();
    // for (auto &inc : t.getIncrementSet()) {
    //     Debug("Removing [%s] from the cache", inc.first.c_str());
    //     cache.Remove(inc.first);
    // }
    
}

/* Aborts the ongoing transaction. */
void
CacheClient::Abort(const uint64_t tid,
                   const Transaction &txn,
                   Promise *promise)
{
    if (!cachingEnabled) {
        txnclient->Abort(tid, promise);
    }
    
    Debug("ABORT [%lu]", tid);

    Promise p(COMMIT_TIMEOUT);
    Promise *pp = (promise != NULL) ? promise : &p;

    txnclient->Abort(tid, pp);
    pp->GetReply();

    cache_lock.lock();
    auto it = prepared.find(tid); 
    if (it != prepared.end()) {
        // clear out the read set
        for (auto &read : it->second.getReadSet()) {
                Debug("Removing [%s] from the cache",
                      read.first.c_str());
                cache.Remove(read.first);
        }
        prepared.erase(tid);
    }
    cache_lock.unlock();
}

void
CacheClient::Subscribe(const uint64_t reactive_id,
                       const std::set<std::string> &keys,
                       const Timestamp timestamp,
                       Promise *promise) {
    Debug("SUBSCRIBE");
    txnclient->Subscribe(reactive_id, keys, timestamp, promise);
}

void
CacheClient::Unsubscribe(const uint64_t reactive_id,
                         const std::set<std::string> &keys,
                         Promise *promise) {
    Debug("UNSUBSCRIBE");
    txnclient->Unsubscribe(reactive_id, keys, promise);
}

void
CacheClient::Ack(const uint64_t reactive_id,
                 const std::set<std::string> &keys,
                 const Timestamp timestamp,
                 Promise *promise) {
    Debug("ACK");
    txnclient->Ack(reactive_id, keys, timestamp, promise);
}

void
CacheClient::Notify(notification_handler_t notify,
                    const uint64_t reactive_id,
                    const Timestamp timestamp,
                    const std::map<std::string, VersionedValue> &values)
{
    Debug("Cache notify");
    cache_lock.lock();
    for (auto v : values) {
        // insert into cache with unbounded timestamp because we know
        // we'll get a notify update later
        v.second.SetEnd(Timestamp());
        Debug("Inserting %s into cache with ts %lu",
              v.second.GetValue().c_str(),
              v.second.GetTimestamp());
        cache.Put(v.first, v.second);
    }
    cache_lock.unlock();
    notify(reactive_id, timestamp, values);
}
