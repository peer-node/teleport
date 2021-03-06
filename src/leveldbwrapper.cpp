// Copyright (c) 2012-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "leveldbwrapper.h"

#include "base/util.h"

#include <boost/filesystem.hpp>
#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <memenv.h>
#include <src/base/util_file.h>


void HandleError(const leveldb::Status &status) throw(leveldb_error) {
    if (status.ok())
        return;
    //LogPrintf("%s\n", status.ToString());
    if (status.IsCorruption())
        throw leveldb_error("Database corrupted");
    if (status.IsIOError())
        throw leveldb_error("Database I/O error");
    if (status.IsNotFound())
        throw leveldb_error("Database entry missing");
    throw leveldb_error("Unknown database error");
}

static leveldb::Options GetOptions(size_t nCacheSize) {
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    return options;
}

CLevelDBWrapper::CLevelDBWrapper(const boost::filesystem::path &path, size_t nCacheSize, bool fMemory, bool fWipe) {
    penv = NULL;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            leveldb::DestroyDB(path.string(), options);
        }
        TryCreateDirectory(path);
    }
    leveldb::Status status = leveldb::DB::Open(options, path.string(), &pdb);
    HandleError(status);
}

CLevelDBWrapper::~CLevelDBWrapper() { }

bool CLevelDBWrapper::WriteBatch(CLevelDBBatch &batch, bool fSync) throw(leveldb_error) {
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    HandleError(status);
    return true;
}
