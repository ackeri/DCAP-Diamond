// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
// vim: set ts=4 sw=4:
/***********************************************************************
 *
 * client/diamondclient.cc:
 *   Client to Diamond frontend servers
 *
 * Copyright 2016 Irene Zhang <iyzhang@cs.washington.edu>
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

#include "client/diamondclient.h"

namespace diamond {

using namespace std;

thread_local uint64_t txnid = 0;
thread_local Transaction txn;
thread_local uint64_t reactiveid = 0;
    
DiamondClient::DiamondClient(string configPath)
    : transport()
{
    initState();

    /* Start a client for the front-end. */
    frontend::Client *frontendclient =
        new frontend::Client(configPath + ".config",
                             &transport,
                             client_id);

    startTransport(frontendclient);
}

DiamondClient::DiamondClient(const string &hostname, const string &port)
    : transport()
{
    initState();

    /* Start a client for the front-end. */
    frontend::Client *frontendclient =
        new frontend::Client(hostname,
                             port,
                             &transport,
                             client_id);
    startTransport(frontendclient);
}

/* Constructor helper method that does everything up to creating the frontend::Client */
void
DiamondClient::initState() {
    // Initialize all state here;
    client_id = 0;
    while (client_id == 0) {
        random_device rd;
        mt19937_64 gen(rd());
        uniform_int_distribution<uint64_t> dis;
        client_id = dis(gen);
    }
    txnid_counter = (client_id/10000)*10000;
    last_notification_ts = Timestamp(0);

    Debug("Initializing Diamond Store client with id [%lu]", client_id);
}

/* Constructor helper method that does everything after creating the frontend::Client */
void
DiamondClient::startTransport(frontend::Client *frontendclient) {
    client = new CacheClient(frontendclient);

    NotificationInit([] () {});
    /* Run the transport in a new thread. */
    pthread_create(&clientTransport,
                   NULL,
                   &DiamondClient::startHelper,
                   (void *)this);

    Debug("Diamond Store client [%lu] created!", client_id);

}

void *
DiamondClient::startHelper(void * arg) {
    DiamondClient * client = (DiamondClient *)arg;
    client->run_client();
    return NULL;
}

DiamondClient::~DiamondClient()
{
    transport.Stop();
    void * status;
    pthread_join(clientTransport, &status);
}

/* Runs the transport event loop. */
void
DiamondClient::run_client()
{
    transport.Run();
}

void
DiamondClient::SetCaching(bool cachingEnabled) {
    client->SetCaching(cachingEnabled);
}

void
DiamondClient::SetIsolationLevel(int level) {
	Panic("Setting Isolation Level unimplemented");	 
}
    
/* Begins a transaction. All subsequent operations before a commit() or
 * abort() are part of this transaction.
 */
void
DiamondClient::Begin()
{
    if (txnid == 0) {
        Debug("Diamondclient::BEGIN Transaction");
        txnid_lock.lock();
        txnid = ++txnid_counter;
        txnid_lock.unlock();
        txn = Transaction();
		reactiveid = 0;
        client->Begin(txnid);
    }
}

void
DiamondClient::BeginRO()
{
    if (txnid == 0) {
        Debug("Diamondclient::BEGIN Transaction");
        txnid_lock.lock();
        txnid = ++txnid_counter;
        txnid_lock.unlock();
        txn = Transaction();
		reactiveid = 0;
        client->BeginRO(txnid);
    }
}

void
DiamondClient::BeginReactive(uint64_t reactive_id)
{
    if (txnid == 0) {
        txnid_lock.lock();
        txnid = ++txnid_counter;
        txnid_lock.unlock();
       
        Timestamp timestamp = Timestamp();
        auto it = timestamp_map.find(reactive_id);
        if (it != timestamp_map.end()) {
            timestamp = it->second;
        }

        Debug("Diamondclient::BEGIN_REACTIVE transaction \
               for reactive_id %lu at timestamp %lu",
              reactive_id, timestamp);

        txn = Transaction();
		//TODO need to specify transaction timestamp for cache hits
		reactiveid = reactive_id;
		//TODO we should enforce read only somehow
        client->BeginRO(txnid);
    }
}

/* Returns the value corresponding to the supplied key. */
int
DiamondClient::Get(const string &key, string &value)
{
    if (txnid == 0) {
        Panic("Doing a GET outside a transaction. \
               YOU ARE A BAD PERSON!!");
    }
    
    Debug("GET [%lu] %s", txnid, key.c_str());

	auto it = txn.getWriteSet().find(key);
    // Read your own writes, check the write set first.
    if (it != txn.getWriteSet().end()) {
        value = it->second;
        return REPLY_OK;
    }

    Promise promise(GET_TIMEOUT);
    // Otherwise, do the GET
    client->Get(txnid, key, &promise);
    
    if (promise.GetReply() == REPLY_OK) {
        Debug("Adding [%s] with ts %lu to the read set",
              key.c_str(), promise.GetTimestamp());
        txn.addReadSet(key, promise.GetTimestamp());
        // if (!txn.HasTimestamp()) {
        //     txn.SetTimestamp(promise.GetValue(key).GetInterval().End());
        //     Debug("Setting ts to %lu", txn.GetTimestamp());
        // }
		//TODO: should read our increments
    } else if (promise.GetReply() == REPLY_NOT_FOUND) {
        Debug("Adding [%s] (not found) to the read set", key.c_str());
        txn.addReadSet(key, Timestamp());
    }

    value = promise.GetValue();

    return promise.GetReply();
}

/* Sets the value corresponding to the supplied key. */
int
DiamondClient::Put(const string &key, const string &value)
{
    if (txnid == 0) {
        Panic("Doing a PUT outside a transaction. YOU ARE A BAD PERSON!!");
    }

    Debug("PUT [%lu] %s (%s)", txnid, key.c_str(), value.c_str());
    
    // Update the write set.
    txn.addWriteSet(key, value);

    return REPLY_OK;
}

int
DiamondClient::Increment(const string &key, const class::Increment inc)
{
    if (txnid == 0) {
        Panic("Doing a PUT outside a transaction. YOU ARE A BAD PERSON!!");
    }

    Debug("INCREMENT [%lu]", txnid, key.c_str());
    
	// Update the increment set.
    txn.addIncrementSet(key, inc);
    return REPLY_OK;
}

/* Attempts to commit the ongoing transaction. */
bool
DiamondClient::Commit()
{
    if (txnid == 0) {
        Panic("Doing a COMMIT outside a transaction. \
               YOU ARE A BAD PERSON!!");
    }

    Debug("COMMIT [%lu]", txnid);
    
    Promise promise(COMMIT_TIMEOUT);

    client->Commit(txnid, txn, Timestamp(), &promise);
    int reply = promise.GetReply();

    if (reactiveid != 0) {
        if (reply != REPLY_OK) {
            Panic("Reactive transaction failed to commit");
        }

        set<string> regset;
		for(auto r : txn.getReadSet()) {
			regset.insert(r.first);
		}
        Timestamp timestamp = promise.GetTimestamp();

        Debug("Reactive commit id %lu ts %lu", reactiveid, timestamp); 
        // Register reactive transaction if it is new or if its
        // registration set has changed
        if (regMap.find(reactiveid) == regMap.end() ||
            regMap[reactiveid] != regset) {
            Promise rp(COMMIT_TIMEOUT);
            client->Subscribe(reactiveid, regset, timestamp, &rp);
            int reply = rp.GetReply();
            if (reply == REPLY_OK) {
                regMap[reactiveid] = regset;
            }
            else {
                //TODO: implement registration retry
                Panic("Registration error: %d", reply);
            }
        }
        if (timestamp_map.find(reactiveid) == timestamp_map.end())
            timestamp_map[reactiveid] = timestamp;
        
    }

    // update the latest txn timestamp we know about
    if (reply == REPLY_OK &&
        promise.GetTimestamp() > last_notification_ts) {
        last_notification_ts = promise.GetTimestamp();
    }
    txnid = 0;
    txn = Transaction();
	reactiveid = 0;
    return reply == REPLY_OK;
}

/* Aborts the ongoing transaction. */
void
DiamondClient::Abort()
{
    if (txnid == 0) {
        Panic("Doing an ABORT outside a transaction. YOU ARE A BAD PERSON!!");
    }

    Debug("ABORT [%lu]",txnid);

    Promise promise(COMMIT_TIMEOUT);
    
    client->Abort(txnid, txn, &promise);
    // block until abort returns
    promise.GetReply();
    txnid = 0;
    txn = Transaction();
	reactiveid = 0;
}

uint64_t
DiamondClient::GetNextNotification(bool blocking)
{
    unique_lock<mutex> l(lock);

    while (pending.size() == 0) {
        if (blocking) {
            cv.wait(l);
        } else {
            return 0;
        }
    }
    uint64_t next = pending.front();
    pending.pop();
    return next;
}

void
DiamondClient::Notify(function<void (void)> callback,
                      const uint64_t reactive_id,
                      const Timestamp timestamp,
                      const map<string, VersionedValue> &values) {
    Debug("Received notification for reactive transaction %lu at ts %lu",
          reactive_id,
          timestamp);
    lock_guard<mutex> l(lock);
    if (timestamp > timestamp_map[reactive_id]) {
        Debug("Invoking notification callback");
        timestamp_map[reactive_id] = timestamp;

        if (timestamp > last_notification_ts) {
            last_notification_ts = timestamp;
        }

        pending.push(reactive_id);
        cv.notify_all();
    }
    callback();
}

void
DiamondClient::Unsubscribe(uint64_t reactive_id)
{
    Promise p;
    client->Unsubscribe(reactive_id, regMap[reactive_id], &p);
    p.GetReply();
}

void
DiamondClient::NotificationInit(function<void (void)> callback)
{
    client->SetNotify(bind(&DiamondClient::Notify,
                           this,
                           callback,
                           placeholders::_1,
                           placeholders::_2,
                           placeholders::_3));
}
        
} // namespace diamond
