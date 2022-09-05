#include "wrapping_integers.hh"
#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    /*unsignedInt的溢出行为在C语言里面是是用的模运算*/
    return WrappingInt32{isn + static_cast<uint32_t>(n)}; 
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t ret ;
    /*求出n的相对seq(zero-inddexed):relative_n*/
    const uint32_t relative_n = n - isn;
    /*求出relative_n和checkpoint的相对距离*/
    /*offset ∈[0,2^32-1]*/
    uint32_t offset = relative_n - checkpoint;
    if(offset <= (1UL << 31)){
        /*relative_n 在checkpoint 右侧 即relative_n > checkpoint*/
        ret = offset + checkpoint;
    }else{
        /*relative_n 在checkpoint 左侧 即relative_n < checkpoint*/
        /*offset ∈(2^31,2^32-1]*/
        ret = checkpoint - ((1UL<<32)-offset);

        /*考虑 relative_n 本来落在 (2^31,2^32-1]这种情况
            此时只要checkpoint是个较小值
            offset都会 落在(2^31,2^32-1] 
            但实际上checkPoint是在relative_n的左边*/
        if( (relative_n > (1U << 31)) && relative_n > checkpoint){
            ret = checkpoint + offset;
        }
    }

    return ret;
}
