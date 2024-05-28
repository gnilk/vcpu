//
// Created by gnilk on 28.05.24.
//

#ifndef VCPU_RINGBUFFER_H
#define VCPU_RINGBUFFER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mutex>

namespace gnilk {
    namespace vcpu {
        template<size_t szBuffer>
        class Ringbuffer {
        public:
            Ringbuffer() = default;
            virtual ~Ringbuffer() = default;

            //
            // Returns the number of bytes available for writing
            //
            size_t BytesFree() {
                std::lock_guard guard(lock);
                return BytesFreeNoLock();
            }

            //
            // Returns the number of bytes available for reading
            //
            size_t BytesAvailable() {
                std::lock_guard guard(lock);
                return BytesAvailableNoLock();
            }

            //
            // Checks if the buffer is full
            //
            bool IsFull() {
                std::lock_guard guard(lock);
                return isFull;
            }

            void *PtrAtWrite() {
                std::lock_guard guard(lock);
                return &data[idxWrite];
            }

            void *PtrAtRead() {
                std::lock_guard guard(lock);
                return &data[idxRead];
            }

            //
            // Peeks a number of bytes forward in the stream
            // This is to allow the dispatcher to fetch meta data ahead of time
            //
            int32_t Peek(void *out, size_t num) {
                std::lock_guard guard(lock);
                // Save the read ptr
                size_t idxReadSave = idxRead;
                int32_t res = ReadNoLock(out, num);

                // Restore the read ptr
                idxRead = idxReadSave;
                return res;
            }

            //
            // Read data from the circular buffer
            //
            // Returns
            //   number of bytes read
            //   negative on return
            //
            int32_t Read(void *out, size_t num) {
                std::lock_guard guard(lock);
                return ReadNoLock(out, num);
            } // read

            //
            // Write data to the buffer, ALL DATA MUST FIT - otherwise error
            //
            // Returns
            //      number of bytes copied in to the buffer
            //      negative on error (-1)
            //
            int32_t Write(const void *src, size_t len) {
                if (len > szBuffer) {
                    return -1;
                }
                std::lock_guard guard(lock);

                auto nFree = BytesFreeNoLock();
                if (nFree < len) {
                    return -1;
                }
                if (len == 0) {
                    return 0;
                }
                if (idxWrite < idxRead) {
                    memcpy(data + idxWrite, src, len);
                    idxWrite += len;
                } else {
                    size_t byteUntilEnd = szBuffer - idxWrite;
                    if (len < byteUntilEnd) {
                        memcpy(data + idxWrite, src, len);
                        idxWrite += len;
                    } else {
                        size_t overrun = len - byteUntilEnd;
                        auto *srcBytePtr = static_cast<const uint8_t *>(src);

                        memcpy(data + idxWrite, src, byteUntilEnd);
                        memcpy(data, srcBytePtr + byteUntilEnd, overrun);
                        idxWrite = overrun;
                    }
                }
                if (idxRead == idxWrite) {
                    isFull = true;
                }
                if (idxWrite >= szBuffer) {
                    idxWrite -= szBuffer;
                }
                return (int32_t)len;
            } // write

        protected:
            //
            // Reads but assume the lock has been taken...
            //
            int32_t ReadNoLock(void *out, size_t num) {
                if (num > szBuffer) {
                    return -1;
                }
                if (BytesAvailableNoLock() < num) {
                    return -1;
                }
                if (idxWrite >= idxRead) {
                    // Is this safe?
                    memcpy(out, data + idxRead, num);
                    idxRead += num;
                } else {
                    size_t leftInBuffer = szBuffer - idxRead;
                    if (num < leftInBuffer) {
                        memcpy(out, data + idxRead, num);
                        idxRead += num;
                    } else {
                        auto *outBytePtr = static_cast<uint8_t *>(out);
                        size_t overrun = num - leftInBuffer;
                        memcpy(outBytePtr, data + idxRead, leftInBuffer);
                        memcpy(outBytePtr + leftInBuffer, data, overrun);
                        idxRead = overrun;
                    }
                }
                if ((num > 0) && (isFull == true)) {
                    isFull = false;
                }
                if (idxRead >= szBuffer) {
                    idxRead -= szBuffer;
                }
                return (int32_t)num;
            } // read


            size_t BytesFreeNoLock() {
                return (szBuffer - BytesAvailableNoLock());
            }
            size_t BytesAvailableNoLock() {
                if (isFull) {
                    // This is wrong...
                    return szBuffer;
                } else {
                    return (((szBuffer) + idxWrite - idxRead) % szBuffer);
                }
            }
        private:
            std::mutex lock;
            uint8_t data[szBuffer] = {};
            bool isFull = false;
            size_t idxRead = 0;
            size_t idxWrite = 0;
        };
    }
}


#endif //VCPU_RINGBUFFER_H
