
// Copyright (c) 2017 david++
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TINY_TIMEUSAGE_TIMEUSAGE_H
#define TINY_TIMEUSAGE_TIMEUSAGE_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <map>

struct BlockInfo {
    std::string name;
    std::string filename;
    std::string function;
    uint32_t line = 0;
};

struct BlockTimeStat : public BlockInfo {
    uint64_t calls = 0;
    uint64_t total_ms = 0;
    float percent = 0;

    static std::string dumpHeaderAsString(const std::string sperator = "\t") {
        std::ostringstream oss;
        oss << "%" << sperator
            << "ms" << sperator
            << "calls" << sperator
            << "ms/call" << sperator
            << "name" << sperator
            << "file:line";
        return oss.str();
    }

    std::string dumpAsString(const std::string sperator = "\t") {
        std::ostringstream oss;
        oss << percent << sperator
            << total_ms << sperator
            << calls << sperator
            << (float) total_ms / calls << sperator
            << name << sperator
            << filename << ":" << line;

        return oss.str();
    }
};

typedef std::shared_ptr<BlockTimeStat> BlockTimeStatPtr;

class BlockTimeUsageStat {
public:
    static BlockTimeUsageStat *instance() {
        static BlockTimeUsageStat stats_;
        return &stats_;
    }

    static void dump2cout(BlockTimeStatPtr stat) {
        std::cout << stat->dumpAsString() << std::endl;
    }

public:
    void called(const BlockInfo &block, uint64_t ms) {
        std::lock_guard<std::mutex> lock_(mutex_);

        total_ms_ += ms;

        auto it = blocks_.find(block.name);
        if (it != blocks_.end()) {
            it->second->calls++;
            it->second->total_ms += ms;
            if (total_ms_)
                it->second->percent = float(it->second->total_ms) / float(total_ms_) * 100;

        } else {
            BlockTimeStatPtr stat = std::make_shared<BlockTimeStat>();
            if (stat) {
                stat->name = block.name;
                stat->filename = block.filename;
                stat->function = block.function;
                stat->line = block.line;
                stat->calls++;
                stat->total_ms += ms;
                if (total_ms_)
                    stat->percent = float(stat->total_ms) / float(total_ms_) * 100;

                blocks_.insert(std::make_pair(stat->name, stat));
            }
        }
    }

    void forEach(const std::function<void(BlockTimeStatPtr)> &cb) {
        std::lock_guard<std::mutex> lock_(mutex_);

        for (auto &it : blocks_)
            cb(it.second);
    }

    void forEachByOrder(const std::function<void(BlockTimeStatPtr)> &cb) {
        std::lock_guard<std::mutex> lock_(mutex_);

        std::multimap<uint64_t, BlockTimeStatPtr> order;
        for (auto &it : blocks_)
            order.insert(std::make_pair(it.second->total_ms, it.second));

        for (auto &it : order)
            cb(it.second);
    }

    void
    dump(const std::function<void(BlockTimeStatPtr)> &cb = BlockTimeUsageStat::dump2cout, uint64_t threshhold = 0) {
        forEachByOrder([threshhold, cb](BlockTimeStatPtr stat) {
            if (stat->total_ms >= threshhold)
                cb(stat);
        });
    }

    void reset() {
        std::lock_guard<std::mutex> lock_(mutex_);
        total_ms_ = 0;
        blocks_.clear();
    }

    bool isOpen() const { return open_; }

    void setOpen(bool value) { open_ = value; }

private:
    volatile bool open_ = true;
    std::mutex mutex_;
    uint64_t total_ms_ = 0;
    std::unordered_map<std::string, BlockTimeStatPtr> blocks_;
};

class BlockTimeUsage {
public:
    BlockTimeUsage(const std::string &name,
                   const std::string &file, uint32_t line,
                   const std::string function,
                   BlockTimeUsageStat *stats = BlockTimeUsageStat::instance())
            : stats_(stats) {
        if (stats && stats->isOpen()) {
            block_.name = name;
            block_.filename = file;
            block_.line = line;
            block_.function = function;
            begin_ = std::chrono::high_resolution_clock::now();
        }
    }

    ~BlockTimeUsage() {
        if (stats_ && stats_->isOpen()) {
            end_ = std::chrono::high_resolution_clock::now();
            long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_ - begin_).count();
            stats_->called(block_, ms);
        }
    }

private:
    BlockInfo block_;
    BlockTimeUsageStat *stats_;

    std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_;
};

#ifndef __FILENAME__
#  define __FILENAME__ __FILE__
#endif

#define FILE_LINE (std::string(__FILENAME__) + ":" + std::to_string(__LINE__))

#ifdef _BLOCK_TIME_CONSUMING_ENABLE
#  define BLOCK_TIME_CONSUMING(name) BlockTimeUsage __time_usage__(name, __FILENAME__, __LINE__, __FUNCTION__);
#  define MYBLOCK_TIME_CONSUMING(stat, name) BlockTimeUsage __time_usage__(name, __FILENAME__, __LINE__, __FUNCTION__, stat);
#else
# define BLOCK_TIME_CONSUMING(name)
# define MYBLOCK_TIME_CONSUMING(stat, name)
#endif


#endif //TINY_TIMEUSAGE_TIMEUSAGE_H
