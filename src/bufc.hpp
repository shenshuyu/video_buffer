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

#if 0
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

namespace bufc
{

// 简单的测试函数
void test_basic_functionality()
{
    std::cout << "=== Testing Basic Functionality ===" << std::endl;

    AVBufC buf;

    // 测试创建
    if (!buf.Create(1024)) {
        std::cerr << "Failed to create buffer" << std::endl;
        return;
    }
    std::cout << "Buffer created successfully" << std::endl;

    // 测试写入数据
    const char* test_data = "Hello, World!";
    size_t data_len = strlen(test_data) + 1;

    AVBufcPkt pkt;
    pkt.pkt_index = 1;

    buf.WritePkt(&pkt, (uint8_t*)test_data, data_len);
    std::cout << "Data written to buffer" << std::endl;

    // 测试读取数据
    AVBufcPkt* read_pkt = buf.ReadPkt();
    if (read_pkt) {
        std::cout << "Read packet index: " << read_pkt->pkt_index << std::endl;
        std::cout << "Read data: " << read_pkt->data << std::endl;
    } else {
        std::cout << "Fail!, No data available to read" << std::endl;
    }

    buf.Destroy();
    std::cout << "Buffer destroyed" << std::endl;
}

void test_multiple_packets()
{
    std::cout << "\n=== Testing Multiple Packets ===" << std::endl;

    AVBufC buf;
    buf.Create(2048);

    // 写入多个数据包
    for (int i = 0; i < 5; i++) {
        std::string data = "Packet " + std::to_string(i);
        AVBufcPkt pkt;
        pkt.pkt_index = i;

        buf.WritePkt(&pkt, (uint8_t*)data.c_str(), data.size() + 1);
        std::cout << "Wrote packet " << i << std::endl;


        // 读取所有数据包
        std::cout << "Reading all packets:" << std::endl;
        int t = 0;
        while (AVBufcPkt* pkt = buf.ReadPkt()) {
            std::cout << "  Packet " << pkt->pkt_index << ": " << pkt->data << std::endl;
            if (t ++ > 100) {
                break;
            }
        }


    }

    buf.Destroy();
}

void test_concurrent_access()
{
    std::cout << "\n=== Testing Concurrent Access ===" << std::endl;

    AVBufC buf;
    buf.Create(4096);

    // 生产者线程函数
    auto producer = [&buf]() {
        for (int i = 0; i < 10; i++) {
            std::string data = "Message " + std::to_string(i) + " from producer";
            AVBufcPkt pkt;
            pkt.pkt_index = i;

            buf.WritePkt(&pkt, (uint8_t*)data.c_str(), data.size() + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "Producer finished" << std::endl;
    };

    // 消费者线程函数
    auto consumer = [&buf]() {
        int count = 0;
        while (count < 10) {
            AVBufcPkt* pkt = buf.ReadPkt();
            if (pkt) {
                std::cout << "Consumer received: " << pkt->data << std::endl;
                count++;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        std::cout << "Consumer finished" << std::endl;
    };

    // 启动线程
    std::thread prod_thread(producer);
    std::thread cons_thread(consumer);

    // 等待线程完成
    prod_thread.join();
    cons_thread.join();

    buf.Destroy();
}

void test_buffer_overflow()
{
    std::cout << "\n=== Testing Buffer Overflow Handling ===" << std::endl;

    AVBufC buf;
    buf.Create(100);  // 小缓冲区

    // 尝试写入大量数据
    std::string large_data(200, 'X');  // 200字节的数据
    AVBufcPkt pkt;
    pkt.pkt_index = 1;

    // 这可能会失败或触发某种处理机制
    buf.WritePkt(&pkt, (uint8_t*)large_data.c_str(), large_data.size());
    std::cout << "Attempted to write large data to small buffer" << std::endl;

    // 尝试读取
    AVBufcPkt* read_pkt = buf.ReadPkt();
    if (read_pkt) {
        std::cout << "Successfully read data from buffer" << std::endl;
    } else {
        std::cout << "No data available (possibly due to overflow)" << std::endl;
    }

    buf.Destroy();
}

void test_performance()
{
    std::cout << "\n=== Testing Performance ===" << std::endl;

    AVBufC buf;
    buf.Create(1024 * 1024);  // 1MB buffer

    const int num_packets = 1000;
    const int data_size = 100;

    auto start = std::chrono::high_resolution_clock::now();

    // 写入性能测试
    for (int i = 0; i < num_packets; i++) {
        std::vector<uint8_t> data(data_size, 'A' + (i % 26));
        AVBufcPkt pkt;
        pkt.pkt_index = i;

        buf.WritePkt(&pkt, data.data(), data.size());
    }

    auto mid = std::chrono::high_resolution_clock::now();

    // 读取性能测试
    for (int i = 0; i < num_packets; i++) {
        AVBufcPkt* pkt = buf.ReadPkt();
        if (pkt) {
            // 只是读取，不做处理
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto write_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto read_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Write " << num_packets << " packets: " << write_time.count() << " μs" << std::endl;
    std::cout << "Read " << num_packets << " packets: " << read_time.count() << " μs" << std::endl;
    std::cout << "Total time: " << total_time.count() << " μs" << std::endl;
    std::cout << "Average time per operation: " << total_time.count() / (2.0 * num_packets) << " μs" << std::endl;

    buf.Destroy();
}

} // namespace bufc

int bufc_test()
{
    std::cout << "AVBufC Test Suite" << std::endl;
    std::cout << "=================" << std::endl;

    try {
        bufc::test_basic_functionality();
        bufc::test_multiple_packets();
        bufc::test_concurrent_access();
        bufc::test_buffer_overflow();
        bufc::test_performance();

        std::cout << "\nAll tests completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#endif