#pragma once

#define MAX_RINGBUFFER_SIZE   1000
#define MAX_BUFFER_NAME_SIZE  80

#define FALSE                   0
#define TRUE                    1
#define INVALID_VALUE           0xffff
#ifndef NULL
    #define NULL                    0
#endif

typedef enum
{
    E_RET_BUF_NULL = 0,
    E_RET_BUF_FULL,
    E_RET_BUF_EMPTY,
    E_RET_BUF_NOT_INIT,
    E_RET_BUF_OK
}E_OPER_BUFFER_RET;

template <class T>
class T_PacketRing_Buffer
{
public:
    T* ptArrayBuffer[MAX_RINGBUFFER_SIZE];
    unsigned int          write;
    unsigned int          read;
    unsigned int          bufferSize;//缓存数组的个数
    unsigned int          curCount;
    unsigned int          writeCount;
    unsigned int          readCount;
    //char            pRingbufName[MAX_BUFFER_NAME_SIZE];
    bool            bIsInit;
    bool            bIsWrited;
    unsigned int          overwriteCnt;
};

#define RING_BUFFER_CUR_READ_POS(ring_buffer) ring_buffer->read
#define RING_BUFFER_CUR_WRITE_POS(ring_buffer) ring_buffer->write
#define RING_BUFFER_ADD_READ_POS(ring_buffer) {ring_buffer->read = (ring_buffer->read + 1)%(ring_buffer->bufferSize);}
#define RING_BUFFER_TO_WRITE_POS(ring_buffer) (ring_buffer->write + 1)%(ring_buffer->bufferSize)
#define RING_BUFFER_ADD_WRITE_POS(ring_buffer) {ring_buffer->write = (ring_buffer->write + 1)%(ring_buffer->bufferSize);}



template <class T>
class SrsRingBuffer
{
public:
    SrsRingBuffer(unsigned int iBufferSize);

    E_OPER_BUFFER_RET Read_Packet_RingBuffer(T** ptOutBuffer);
    E_OPER_BUFFER_RET Read_Packet_Top_RingBuffer(T** ptOutBuffer);
    E_OPER_BUFFER_RET Write_Packet_RingBuffer(T* ptInBuffer, unsigned int iReadIdx = 0);
    void Destroy_Packet_RingBuffer(unsigned int iReadIdx = 0);
    unsigned int Get_Packet_RingBuffer_Count();
    
    virtual ~SrsRingBuffer();

private:
    T_PacketRing_Buffer<T> mRingBuf;
};




template <class T>
SrsRingBuffer<T>::SrsRingBuffer(unsigned int iBufferSize)
{
    memset(&mRingBuf, 0, sizeof(T_PacketRing_Buffer<T>));
    mRingBuf.bufferSize = (iBufferSize <= 0 || iBufferSize >= MAX_RINGBUFFER_SIZE) ? MAX_RINGBUFFER_SIZE : iBufferSize;
    mRingBuf.bIsInit = TRUE;
}

template <class T>
unsigned int SrsRingBuffer<T>::Get_Packet_RingBuffer_Count()
{
    return (mRingBuf.write + mRingBuf.bufferSize - mRingBuf.read) % (mRingBuf.bufferSize);
}


template <class T>
E_OPER_BUFFER_RET SrsRingBuffer<T>::Read_Packet_RingBuffer(T** ptOutBuffer)
{
    if (NULL == ptOutBuffer)
    {
        return E_RET_BUF_NULL;
    }

    if (FALSE == mRingBuf.bIsInit)
    {
        return E_RET_BUF_NOT_INIT;
    }

    if (RING_BUFFER_CUR_READ_POS((&mRingBuf)) == RING_BUFFER_CUR_WRITE_POS((&mRingBuf)))
    {
        return E_RET_BUF_EMPTY;
    }
    else
    {
        mRingBuf.readCount++;
        *ptOutBuffer = mRingBuf.ptArrayBuffer[RING_BUFFER_CUR_READ_POS((&mRingBuf))];
        if (NULL == (*ptOutBuffer))
        {
            //ERRORLOG("Read_Packet_RingBuffer, mRingBuf is NULL, buffer name: %s\n", mRingBuf.pRingbufName);
            return E_RET_BUF_EMPTY;
        }
        mRingBuf.curCount = Get_Packet_RingBuffer_Count();
        RING_BUFFER_ADD_READ_POS((&mRingBuf));
        
        return E_RET_BUF_OK;
    }
}


template <class T>
E_OPER_BUFFER_RET SrsRingBuffer<T>::Read_Packet_Top_RingBuffer(T** ptOutBuffer)
{
    if (NULL == ptOutBuffer || 0 == mRingBuf.bufferSize)
    {
        return E_RET_BUF_NULL;
    }

    if (FALSE == mRingBuf.bIsInit)
    {
        return E_RET_BUF_NOT_INIT;
    }

    if (RING_BUFFER_CUR_READ_POS((&mRingBuf)) == RING_BUFFER_CUR_WRITE_POS((&mRingBuf)))
    {
        return E_RET_BUF_EMPTY;
    }
    else
    {
        *ptOutBuffer = mRingBuf.ptArrayBuffer[RING_BUFFER_CUR_READ_POS((&mRingBuf))];
        if (NULL == (*ptOutBuffer))
        {
            //ERRORLOG("Read_Packet_RingBuffer, mRingBuf is NULL, buffer name: %s\n", mRingBuf.pRingbufName);
            return E_RET_BUF_EMPTY;
        }
        
        return E_RET_BUF_OK;
    }
}


template <class T>
E_OPER_BUFFER_RET SrsRingBuffer<T>::Write_Packet_RingBuffer(T* ptInBuffer, unsigned int iReadIdx)
{
    if (NULL == ptInBuffer)
    {
        return E_RET_BUF_NULL;
    }
    if (0 == mRingBuf.bufferSize)
    {
       // MemPool::getInstance()->FreePacketBuffer(ptInBuffer, iReadIdx);
        // fix it
        return E_RET_BUF_NULL;
    }
    if (FALSE == mRingBuf.bIsInit)
    {
        //MemPool::getInstance()->FreePacketBuffer(ptInBuffer, iReadIdx);
         // fix it
        return E_RET_BUF_NOT_INIT;
    }
    mRingBuf.writeCount++;
    if (RING_BUFFER_TO_WRITE_POS((&mRingBuf)) == RING_BUFFER_CUR_READ_POS((&mRingBuf)))
    {
        //MemPool::getInstance()->FreePacketBuffer(ptInBuffer, iReadIdx);
         // fix it
        mRingBuf.overwriteCnt++;
        //ERRORLOG("Write_Packet_RingBuffer, ringbuf name = %s, ringbuf is full, write [%d] read [%d]", mRingBuf.pRingbufName, mRingBuf.writeCount, mRingBuf.readCount);
        return E_RET_BUF_FULL;
    }

    mRingBuf.ptArrayBuffer[RING_BUFFER_CUR_WRITE_POS((&mRingBuf))] = ptInBuffer;
    mRingBuf.curCount = Get_Packet_RingBuffer_Count();
    RING_BUFFER_ADD_WRITE_POS((&mRingBuf));
    if (FALSE == mRingBuf.bIsWrited)
    {
        mRingBuf.bIsWrited = TRUE;
    }
    
    return E_RET_BUF_OK;
}


template <class T>
void SrsRingBuffer<T>::Destroy_Packet_RingBuffer(unsigned int iReadIdx)
{
    T* ptBuffer = NULL;
    
    if (FALSE == mRingBuf.bIsInit)
    {
        return;
    }

    while (1)
    {
        if (E_RET_BUF_OK != Read_Packet_RingBuffer(&ptBuffer))
        {
            break;
        }
        //MemPool::getInstance()->FreePacketBuffer(ptBuffer, iReadIdx);
        //fit it
    }
}


template <class T>
SrsRingBuffer<T>::~SrsRingBuffer()
{
}

