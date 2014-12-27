//
//  MsgParser.h
//  MsgParser
//
//  Created by leepood on 14/12/23.
//  Copyright (c) 2014年 leepood. All rights reserved.
//

#ifndef __MsgParser__MsgParser__
#define __MsgParser__MsgParser__

#include <stdio.h>

#include <stdlib.h>
#include <functional>
#include <string.h>

#define BYTE  unsigned char

#define BUFFER_SIZE 0xffff + 8

#define BYTES_TO_INT(array) (array[0] << 24 | array[1] << 16 | array[2] << 8 | array[3])


typedef struct{
    int size;
    const BYTE* header;
}VArray;

class MsgPacket;
class MsgParser {
public:
    MsgParser(int bufferSize = BUFFER_SIZE);
    void registerOnMsgReceiver(const std::function<void(MsgPacket&)>& msgReceiverListener);
    void parse(const BYTE datas[], const int size);
    virtual ~MsgParser();
private:
    enum State {
        STATE_READING_HEADER, /* 开始读取消息头 */
        STATE_WAITTING_HEADER, /* 等待消息头完整 */
        STATE_READING_CONTENT, /* 开始读取内容部分 */
        STATE_WAITING_CONTENT /* 等待内容完整 */
    };
    
    int needToReadSize; /* 对于接下来的数据需要读的数量 */
    int bufferedSize; /* 已经存放在缓冲区的数据 */
    int fixedHeaderSize; /* 消息头部固定大小 */
    
private:
    void analyze(const BYTE datas[], const int size,VArray* dst);
    BYTE* buffer;
    State currentState;
    VArray* vArray;
    void clearBufferAndReset();
    int limitBufferedSize;
    std::function<void(MsgPacket&)> receiveListener;
public:
    bool validate(const BYTE* data,int size);
};




#endif /* defined(__MsgParser__MsgParser__) */
