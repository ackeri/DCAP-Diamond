// -*- mode: c++; c-file-style: "k&r"; c-basic-offset: 4 -*-
/***********************************************************************
 *
 * store/strongstore/occstore.cc:
 *   Key-value store with support for strong consistency using OCC
 *
 * Copyright 2013-2015 Irene Zhang <iyzhang@cs.washington.edu>
 *                     Naveen Kr. Sharma <naveenks@cs.washington.edu>
 *                     Dan R. K. Ports  <drkp@cs.washington.edu>
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

#include "occstore.h"
#include <algorithm>

using namespace std;

namespace strongstore {

OCCStore::OCCStore() : store() { }
OCCStore::~OCCStore() { }

int
OCCStore::Get(const uint64_t tid,
	      const string &key,
	      Version &value,
	      const Timestamp &timestamp)
{
    Debug("[%lu] GET %s at %lu", tid, key.c_str(), timestamp);
    
    if (store.Get(key, timestamp, value)) {
        Debug("[%lu] Returning %s=%s ending at %lu last_commit=%lu",
              tid, key.c_str(), value.GetValue().c_str(),
              value.GetInterval().End(), last_committed);
        return REPLY_OK;
    } else {
        return REPLY_NOT_FOUND;
    }
}

int
OCCStore::Prepare(const uint64_t tid, const Transaction &txn)
{    
    Debug("[%lu] START PREPARE", tid);

    if (prepared.find(tid) != prepared.end()) {
        Debug("[%lu] Already prepared!", tid);
        return REPLY_OK;
    }

    Timestamp ts;
    // Do OCC checks.
    // Check for conflicts with the read set.
	for (auto &read : txn.getReadSet()) {
		Version cur;
		const string &key = read.first;
		const Interval &valid = read.second;

		// ignore if this version doesn't exist
		if (!store.Get(key, cur)) {
			continue;
		}

		// If this key has been written since we read it, abort.
		if (cur.GetTimestamp() > valid.Start()) {
			Debug("[%lu] ABORT LINEARIZABLE rw conflict key:%s %lu %lu",
					tid, key.c_str(), cur.GetTimestamp(),
					valid.Start());
		
			Abort(tid);
			return REPLY_FAIL;
		}

		// If there is a pending write for this key, abort.
		if (pWrites.find(key) != pWrites.end() &&
			pWrites[key] > 0) {
			Debug("[%lu] ABORT LINEARIZABLE rw conflict w/ prepared key:%s",
					tid, read.first.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}

		if (pIncrements.find(key) != pIncrements.end() &&
			pIncrements[key] > 0) {
			Debug("[%lu] ABORT LINEARIZABLE ri conflict w/ prepared key:%s",
					tid, read.first.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}
	}

    // Check for conflicts with the write set.
    for (auto &write : txn.getWriteSet()) {
        const string &key = write.first;
        
        // if there is a pending write, always abort
        if (pWrites.find(key) != pWrites.end() &&
            pWrites[key] > 0) {
            Debug("[%lu] ABORT ww conflict w/ prepared key:%s", tid,
                  key.c_str());
            Abort(tid);
            return REPLY_FAIL;
        }

        // if there is a pending increment, always abort
        if (pIncrements.find(key) != pIncrements.end() &&
            pIncrements[key] > 0) {
            Debug("[%lu] ABORT wi conflict w/ prepared key:%s", tid,
                  key.c_str());
            Abort(tid);
            return REPLY_FAIL;
        }
	    
		// If there is a pending read for this key, abort to stay linearizable.
		if (pReads.find(key) != pReads.end() &&
			pReads[key] > 0) {
			Debug("[%lu] ABORT LINEARIZABLE rw conflict w/ prepared key:%s", tid,
					key.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}

		// if there is a read at a later timestamp
		if (store.GetLastRead(key, ts) && ts > last_committed) {
			Debug("[%lu] ABORT LINEARIZABLE rw conflict w/ previous read:%s", tid,
					key.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}
	}

    // Check for conflicts with the increment set
    for (auto &inc : txn.getIncrementSet()) {
        const string &key = inc.first;
            
        // don't need to check for pending increments, they commute
            
        // if there is a write, we always abort
        if (pWrites.find(key) != pWrites.end() &&
            pWrites[key] > 0) {
            Debug("[%lu] ABORT wi conflict w/ prepared key:%s", tid,
                  key.c_str());
            Abort(tid);
            return REPLY_FAIL;
        }

		// Check for pending reads
		if (pReads.find(key) != pReads.end() &&
			pReads[key] > 0) {
			Debug("[%lu] ABORT LINEARIZABLE ri conflict w/ prepared key:%s",
					tid,
					key.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}

		// Check for timestamp reads
		if (store.GetLastRead(key, ts) && ts > last_committed) {
			Debug("[%lu] ABORT LINEARIZABLE rw conflict w/ previous read:%s",
					tid,
					key.c_str());
			Abort(tid);
			return REPLY_FAIL;
		}
	}
	

    // Otherwise, prepare this transaction for commit
    prepared[tid] = txn;
    for (auto &write : txn.getWriteSet()) {
        pWrites[write.first] = pWrites[write.first] + 1;
    }
    for (auto &read : txn.getReadSet()) {
        pReads[read.first] = pReads[read.first] + 1;
    }
    for (auto &incr : txn.getIncrementSet()) {
        pIncrements[incr.first] = pIncrements[incr.first] + 1;
    }

    prepared_last_commit[tid] = last_committed;
    Debug("[%lu] PREPARED TO COMMIT", tid);
    return REPLY_OK;
}

void
OCCStore::Commit(const uint64_t tid, const Timestamp &timestamp, const Transaction &txn)
{
    if (committed.find(tid) == committed.end()) {
        Transaction t;
        Debug("[%lu] COMMIT", tid);
        if (prepared.find(tid) != prepared.end()) {
            t = prepared[tid];
        } else {
            t = txn;
        }

        for (auto &write : t.getWriteSet()) {
            store.Put(write.first, // key
                      write.second, // value
                      timestamp); // timestamp
        }

        for (auto &inc : t.getIncrementSet()) {
            store.Increment(inc.first,
                            inc.second,
                            timestamp);
        }

		prepared.erase(tid);
		for (auto &write : t.getWriteSet()) {
			pWrites.at(write.first)--;
			ASSERT(pWrites[write.first] >= 0);
		}
		for (auto &read : t.getReadSet()) {
			pReads.at(read.first)--;
			ASSERT(pReads[read.first] >= 0);
		}
		for (auto &incr : t.getIncrementSet()) {
			pIncrements.at(incr.first)--;
			ASSERT(pIncrements[incr.first] >= 0);
		}
        
		committed[tid] = t;
        if (timestamp > last_committed) {
            last_committed = timestamp;
        }
    }
}

void
OCCStore::Abort(const uint64_t tid)
{
    Debug("[%lu] ABORT", tid);
    if (prepared.find(tid) != prepared.end()) {
        Transaction t = prepared[tid];
        prepared.erase(tid);
        for (auto &write : t.getWriteSet()) {
            pWrites.at(write.first)--;
            ASSERT(pWrites[write.first] >= 0);
        }
        for (auto &read : t.getReadSet()) {
            pReads.at(read.first)--;
            ASSERT(pReads[read.first] >= 0);
        }
        for (auto &incr : t.getIncrementSet()) {
            pIncrements.at(incr.first)--;
            ASSERT(pIncrements[incr.first] >= 0);
        }
    }
}

void
OCCStore::Load(const string &key, const string &value, const Timestamp &timestamp)
{
    store.Put(key, value, timestamp);
}

void
OCCStore::getPreparedOps(unordered_set<string> &reads, unordered_set<string> &writes, unordered_set<string> &increments)
{
    for (auto &t : prepared) {
        for (auto &write : t.second.getWriteSet()) {
            writes.insert(write.first);
        }
        for (auto &read : t.second.getReadSet()) {
            reads.insert(read.first);
        }
        for (auto &incr : t.second.getIncrementSet()) {
            increments.insert(incr.first);
        }
    }
}

Timestamp
OCCStore::getPreparedUpdate(const string &key)
{
    Timestamp ts = MAX_TIMESTAMP;
    for (auto &t : prepared) {
        for (auto &write : t.second.getWriteSet()) {
	    if (write.first == key) {
		if (prepared_last_commit[t.first] < ts)
		    ts = prepared_last_commit[t.first];
	    }		
        }
        for (auto &incr : t.second.getIncrementSet()) {
	    if (incr.first == key) {
		if (prepared_last_commit[t.first] < ts)
		    ts = prepared_last_commit[t.first];
	    }
        }
    }
    return ts;
}
    
} // namespace strongstore
