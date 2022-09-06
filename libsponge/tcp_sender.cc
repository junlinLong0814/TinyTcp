#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno; }

void TCPSender::fill_window() {
    if(!_is_established && !_noack_segs.empty() && _noack_segs.front().header().syn){
        return ;
    }

    if(_next_seqno < _ackno && _is_established){
        return ;
    }
    if(_rec_win_size == 0 && !_noack_segs.empty()){
        /*接收窗口为0 且 有未确认的报文 返回*/
        return ;
    }

    /*数据负荷大小取决于接收方接收窗口大小以及已经发送的数据大小*/
    uint64_t send_size = _next_seqno - _ackno;
    uint64_t playload_size = _rec_win_size>send_size ? (_rec_win_size-send_size):0;
    
    /*_rec_win_size == 0时 发送探测报文(有数据发数据 有fin||syn发对应字段)*/
    playload_size = _rec_win_size > 0 ? playload_size : 1;
    for( ; ; ){
        TCPSegment seg{};
        if(!_is_established){
            /*未建立连接*/
            _is_established = true;
            seg.header().syn = 1;
            --playload_size;
        }

        Buffer buffer{_stream.read(playload_size > TCPConfig::MAX_PAYLOAD_SIZE?
                                TCPConfig::MAX_PAYLOAD_SIZE : playload_size)};
        seg.payload() = buffer;
        
        playload_size -= buffer.size();

        if(playload_size > 0 && 
            !_is_finwait1 && 
            _stream.eof())
        {
            _is_finwait1 = true;
            seg.header().fin = 1;
            --playload_size;
        }

        if(seg.length_in_sequence_space() != 0 ){
            seg.header().seqno = wrap(_next_seqno,_isn);
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.push(seg);
            _noack_segs.push(seg);
            if(!_timer_start){
                _timer_start = true;
                _elapsed_time = 0;
                _rto = _initial_retransmission_timeout;
            }
        }    

        if(seg.length_in_sequence_space() == 0){
            break;
        }
    }
    

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_ack = unwrap(ackno,_isn,_next_seqno);
    if(abs_ack > _next_seqno || abs_ack < _ackno){
        /*收到的确认号大于即将发送的下一个序列号||小于上一个发送来的确认号
         非法确认号*/
        return ;
    }

    _rec_win_size = window_size;
    _ackno = abs_ack;
    
    while(!_noack_segs.empty()){
        TCPSegment& seg= _noack_segs.front();
        if(_ackno >= seg.length_in_sequence_space()+unwrap(seg.header().seqno,_isn,_next_seqno)){
            _noack_segs.pop();
            /*重置计时器*/
            _elapsed_time = 0;
            _rto = _initial_retransmission_timeout;
            _retran_count = 0;
            continue;
        }
        break;
    }

    if(_noack_segs.empty()){
        _timer_start = false;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if(!_timer_start || _noack_segs.empty()){
        return ;
    }
    _elapsed_time += ms_since_last_tick;
    if(_elapsed_time >= _rto){
        TCPSegment& retran_seg = _noack_segs.front();
        _segments_out.push(retran_seg);
        if(_rec_win_size != 0 || retran_seg.header().syn ){
            /*接收者窗口为0 且 不是syn报文 ++重传次数*/
            ++_retran_count;
            _rto <<= 1;
        }
        _elapsed_time = 0;
    }

 }

unsigned int TCPSender::consecutive_retransmissions() const { return _retran_count; }

void TCPSender::send_empty_segment() {
    TCPSegment seg{};
    seg.header().seqno = wrap(_next_seqno,_isn);
    _segments_out.push(seg);
}

