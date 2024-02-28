//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_KERNEL_IO_HPP
#define SRS_KERNEL_IO_HPP

#include <srs_core.hpp>

/**
 * The reader to read data from channel.
 */
class ISrsReader
{
public:
    ISrsReader();
    virtual ~ISrsReader();
public:
    /**
     * Read bytes from reader.
     * @param nread How many bytes read from channel. NULL to ignore.
     */
    virtual srs_error_t read(void* buf, size_t size, ssize_t* nread) = 0;
};

/**
 * The seeker to seek with a device.
 */
class ISrsSeeker
{
public:
    ISrsSeeker();
    virtual ~ISrsSeeker();
public:
    /**
     * The lseek() function repositions the offset of the file descriptor fildes to the argument offset, according to the
     * directive whence. lseek() repositions the file pointer fildes as follows:
     *      If whence is SEEK_SET, the offset is set to offset bytes.
     *      If whence is SEEK_CUR, the offset is set to its current location plus offset bytes.
     *      If whence is SEEK_END, the offset is set to the size of the file plus offset bytes.
     * @param seeked Upon successful completion, lseek() returns the resulting offset location as measured in bytes from
     *      the beginning of the file. NULL to ignore.
     */
    virtual srs_error_t lseek(off_t offset, int whence, off_t* seeked) = 0;
};

/**
 * The reader and seeker.
 */
class ISrsReadSeeker : public ISrsReader, public ISrsSeeker
{
public:
    ISrsReadSeeker();
    virtual ~ISrsReadSeeker();
};

/**
 * The writer to write stream data to channel.
 */
class ISrsWriter
{
public:
    ISrsWriter();
    virtual ~ISrsWriter();
public:
    /**
     * write bytes over writer.
     * @nwrite the actual written bytes. NULL to ignore.
     */
    virtual srs_error_t write(void* buf, size_t size, ssize_t* nwrite) = 0;
};

/**
 * The writer and seeker.
 */
class ISrsWriteSeeker : public ISrsWriter, public ISrsSeeker
{
public:
    ISrsWriteSeeker();
    virtual ~ISrsWriteSeeker();
};

#endif

