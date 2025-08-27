#pragma once

#include "fifo.h"
#include <vector>
#include <mutex>

namespace bufc
{

struct AVBufcPkt {
    int32_t pkt_index;
    int32_t pkt_size;
    int64_t pts;
    uint8_t *data;
};

class AVBufC
{
    std::mutex mtx_;
    AVFifo *fifo_ = nullptr;
    AVBufcPkt tmppkt_ = {};
    std::vector<uint8_t> tmppktbuf_;
    AVFrameIter defultiter_ = {};
public:
    bool Create(size_t bytes)
    {
        std::lock_guard<std::mutex> l(mtx_);

        if (fifo_) {
            return true;
        }
        fifo_ = av_fifo_create(bytes);
        if (!fifo_) {
            std::cerr << "av_fifo_create fail, bytes: " << bytes << std::endl;
            return false;
        }
        tmppktbuf_.resize(2*1024*1024);
        return true;
    }

    void WritePkt(AVBufcPkt *hdr, const uint8_t *data, size_t len)
    {

        struct iovec iov[2] = {
            { .iov_base = hdr,        .iov_len = sizeof(AVBufcPkt) },
            { .iov_base = (void*)data,       .iov_len = len       },
        };
        ssize_t wr = av_fifo_writev(fifo_, iov, 2);
        if (wr <= 0) {
            std::cerr << "av_fifo_writev fail, bytes: " << len << std::endl;
        }
    }

    AVBufcPkt *ReadPkt()
    {
        std::lock_guard<std::mutex> l(mtx_);

        if (!defultiter_.frame) {
            defultiter_ = av_fifo_first_frame(fifo_, 1);
        }

        struct iovec iov[2] = {
            { .iov_base = &tmppkt_,         .iov_len = sizeof(AVBufcPkt) },
            { .iov_base = &tmppktbuf_[0],   .iov_len = tmppktbuf_.size()},
        };

        size_t len = av_fifo_readv(fifo_, iov, 2, &defultiter_);

        if (len <= 0) {
            return nullptr;
        }
        tmppkt_.data = &tmppktbuf_[0];
        return &tmppkt_;
    }

    size_t LeftPkt()
    {
        if (defultiter_.frame) {
            return av_fifo_left(fifo_, &defultiter_);
        }
        return 0;
    }

    void Destroy()
    {
        std::lock_guard<std::mutex> l(mtx_);
        if (fifo_) {
            av_fifo_destroy(fifo_);
            fifo_ = nullptr;
        }
    }
};
}