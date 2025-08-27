
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include "bufc.hpp"

using namespace bufc;

void test_basic_functionality()
{
    std::cout << "=== Testing Basic Functionality ===" << std::endl;

    AVBufC buf;

    if (!buf.Create(1*1024*1024)) {
        std::cerr << "Failed to create buffer" << std::endl;
        return;
    }
    std::cout << "Buffer created successfully" << std::endl;

    // 测试写入数据(视频帧)
    const char* test_data = "Hello, World!";
    size_t data_len = strlen(test_data) + 1;

    AVBufcPkt pkt;
    pkt.pkt_index = 1;

    buf.WritePkt(&pkt, (uint8_t*)test_data, data_len);
    std::cout << "Data written to buffer" << std::endl;

    // 测试读取数据（视频帧）
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
    buf.Create(100); 

    std::string large_data(200, 'X'); 
    AVBufcPkt pkt;
    pkt.pkt_index = 1;

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

    for (int i = 0; i < num_packets; i++) {
        std::vector<uint8_t> data(data_size, 'A' + (i % 26));
        AVBufcPkt pkt;
        pkt.pkt_index = i;

        buf.WritePkt(&pkt, data.data(), data.size());
    }

    auto mid = std::chrono::high_resolution_clock::now();

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

int main()
{
    std::cout << "AVBufC Test Suite" << std::endl;
    std::cout << "=================" << std::endl;

    test_basic_functionality();
    test_multiple_packets();
    test_concurrent_access();
    test_buffer_overflow();
    test_performance();

    std::cout << "\nAll tests completed successfully!" << std::endl;

    return 0;
}