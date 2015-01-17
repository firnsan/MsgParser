//
//  MsgParser.cpp
//  MsgParser
//  状态机，协议解析器
//
//  Created by leepood on 14/12/23.
//  Copyright (c) 2014年 leepood. All rights reserved.
//

#include "MsgParser.h"
#include <iostream>
using namespace std;

MsgParser::MsgParser(int bufferSie /* = BUFFER_SIZE */) {
    limitBufferedSize = bufferSie;
    buffer = (BYTE*) malloc(limitBufferedSize);
    memset(buffer, 0, limitBufferedSize);
    vArray =  (VArray*)malloc(sizeof(VArray));
    needToReadSize = 0;
    bufferedSize = 0;
    fixedHeaderSize = 2 + 1 + 2 + 2; // header | version | command | length
    currentState = STATE_READING_HEADER;
}

void MsgParser::parse(const BYTE datas[], const int size) {
    bzero(vArray,sizeof(VArray));
    analyze(datas,size,vArray);
    if(vArray->size != 0){
        parse(vArray->header,vArray->size);
    }
}

void MsgParser::analyze(const BYTE datas[], const int size, VArray* dst) {
    
    if (currentState == STATE_READING_HEADER) {
        cout << "reading header" << endl;
        needToReadSize = fixedHeaderSize;
        if (size >= needToReadSize) {
            BYTE length[] = { 0x00, 0x00, datas[5], datas[6] };
            memcpy(buffer, datas, fixedHeaderSize);
            bufferedSize += fixedHeaderSize;
            
            needToReadSize = BYTES_TO_INT(length) + 1;
            currentState = STATE_READING_CONTENT;
            if (size - fixedHeaderSize > 0) {
                //copy 剩下的数据到dst
                dst->size = size - fixedHeaderSize;
                dst->header = datas + fixedHeaderSize;
            }
        } else {
            needToReadSize = fixedHeaderSize - size;
            memcpy(buffer + bufferedSize, datas, size);
            bufferedSize += size;
            currentState = STATE_WAITTING_HEADER;
            cout << "waiting header" << endl;
        }
    } else if (currentState == STATE_WAITTING_HEADER) {
        
        if (size >= needToReadSize) {
            memcpy(buffer + bufferedSize, datas, needToReadSize);
            int tempNeedSize = needToReadSize;
            bufferedSize += needToReadSize;
            BYTE length[] = { 0X00, 0X00, buffer[5], buffer[6] };
            needToReadSize = BYTES_TO_INT(length) + 1; // +1 because of checksum
            
            currentState = STATE_READING_CONTENT;
            cout << "reading content" << endl;
            if (size - tempNeedSize > 0) {
                //剩下的返回出去把
                dst->size = size - tempNeedSize;
                dst->header = datas + tempNeedSize;
            }
        }
        else{
            memcpy(buffer + bufferedSize,datas,size);
            bufferedSize += size;
            needToReadSize = fixedHeaderSize - bufferedSize;
            cout << "still waiting header" << endl;
        }
    } else if (currentState == STATE_READING_CONTENT) {
        if(size >= needToReadSize){
            int tempNeedSize = needToReadSize;
            BYTE* packet = (BYTE*)malloc(fixedHeaderSize + needToReadSize);
            memcpy(packet,buffer,fixedHeaderSize);
            memcpy(packet + fixedHeaderSize,datas,needToReadSize);
            
            //校验数据合法
            if(validate(packet, fixedHeaderSize + needToReadSize)){
                if(receiveListener){
                    //发出去
                  cout << "find a packet successful,state changed to reading header" << endl;
                }
            }
      
            // 找到一个完整的消息啦
            clearBufferAndReset();
            
            if(size - tempNeedSize > 0){
                //剩下的返回出去
                dst->size = size - tempNeedSize;
                dst->header = datas + tempNeedSize;
            }
        }
        else{
            memcpy(buffer + bufferedSize,datas,size);
            bufferedSize += size;
            needToReadSize = needToReadSize - size;
            currentState = STATE_WAITING_CONTENT;
            cout << "start waiting content,need size :"<< needToReadSize << endl;
        }
    } else if (currentState == STATE_WAITING_CONTENT) {
        if(size >= needToReadSize){
            int tempNeedSize = needToReadSize;
            BYTE* packet = (BYTE*)malloc(bufferedSize + needToReadSize);
            //先从缓冲区取出数据
            memcpy(packet,buffer,bufferedSize);
            memcpy(packet + bufferedSize,datas,needToReadSize);
           
            if(validate(packet,bufferedSize + needToReadSize)){
                
                if(receiveListener){
                    //发出去
                     cout << "find a packet ok! change to reading header" << endl;
                }
            }
            
            //一个完整包取完了
            clearBufferAndReset();
            
            if(size - tempNeedSize > 0){
                //还有剩余，返回出去把
                dst->size = size - tempNeedSize;
                dst->header = datas + tempNeedSize;
            }
        }
        else{
            memcpy(buffer + bufferedSize,datas,size);
            needToReadSize = needToReadSize - size;
            bufferedSize += size;
            
            cout << "still waiting content,need size :"<< needToReadSize << endl;
        }
}
}

void MsgParser::clearBufferAndReset(){
    memset(buffer,0,limitBufferedSize);
    bufferedSize = 0;
    needToReadSize = 0;
    currentState = STATE_READING_HEADER;
}


void MsgParser::registerOnMsgReceiver(const std::function<void(MsgPacket&)>& msgReceiverListener){
    this->receiveListener = msgReceiverListener;
}


bool MsgParser::validate(const BYTE* data,int size){
    
    if(size < fixedHeaderSize + 1)
        return false;
    
    if(*data != 0x30 || *(data + 1) != 0x18){
        return false;
    }
    
    //checksum
    BYTE checksum = 0x00;
    for(int i=0;i < size - 1;i++){
        checksum += *(data + i);
    }
    if( *(data + size - 1) != checksum )
        return false;
    return true;
}

MsgParser::~MsgParser() {
    if(buffer){
        free(buffer);
        bufferedSize = 0;
        needToReadSize = 0;
        buffer = NULL;
    }
    
    if(vArray){
        free(vArray);
        vArray = NULL;
    }
}
