# BufC - 视频缓冲区测试工具
    BufC 是一个用于视频数据的写入和读取操作的缓存c++类。
    参考ffmpeg avutil 中的fifo 
    无任何第三方依赖

    BufC is a C++ class for buffering video data write and read operations.

    Referencing the fifo in FFmpeg's avutil

    No third-party dependencies

- 创建指定大小的视频缓冲区
- 写入视频帧数据（支持索引标记）
- 读取视频帧数据
- 自动内存管理


- Create video buffers of specified sizes

- Write video frame data (supports index tagging)

- Read video frame data

- Automatic memory management

## 示例代码
```
    #include "bufc.hpp"

    AVBufC buf;
    if (!buf.Create(1 * 1024 * 1024)) {
        std::cerr << "Failed to create buffer" << std::endl;
        return;
    }

    const char* frame_data = "Hello, World!";
    size_t data_len = strlen(frame_data) + 1;

    AVBufcPkt pkt;
    pkt.pkt_index = 1;

    buf.WritePkt(&pkt, (uint8_t*)frame_data, data_len);

    AVBufcPkt* read_pkt = buf.ReadPkt();
    if (read_pkt) {
        std::cout << "Read packet index: " << read_pkt->pkt_index << std::endl;
        std::cout << "Read data: " << read_pkt->data << std::endl;
    }

    buf.Destroy();
```