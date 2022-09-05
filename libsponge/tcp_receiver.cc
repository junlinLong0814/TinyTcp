#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader st_header = seg.header();
    if(!st_header.syn && !_syn){
        /*无效报文*/
        return ;
    }
    if(st_header.syn){
        /*syn 报文*/
        if(_syn){
            /*之前已经收到syn报文了 目前考虑单peer的情况下*/
            return ;
        }
        _syn = true;
        _ACK = true;    //二次握手 可以使用ack字段
        _syn_with_data = seg.payload().size()>0;
        _isn_peer = st_header.seqno;
    }
    if(st_header.fin){
        /*fin报文*/
        if(_finish){
            return ;
        }
        _finish = true;
    }

    /*_syn_with_data true时表示syn报文携带数据，说明stream_buff 冲入了syn报文的数据即syn报文的relative_idx=0
    当它为false时，说明syn报文的relative_idx=-1，白费一个序号，此时要将seq与isn的差值减掉1*/
    uint64_t relative_seq = unwrap(st_header.seqno,_isn_peer.value(),next_relative_seq()) - (!_syn_with_data) ;
   
    _reassembler.push_substring(seg.payload().copy(),relative_seq ,_finish);
}


optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_ACK || !_syn){
        return std::nullopt;
    }
    /*fin报文会消耗一个序列号 但是不能直接+_finish 因为有可能该fin报文带数据，且当前数据没全部放入stream，只有当当前数据全部放入stream即
    有eof产生才可以+1*/
    uint64_t next_seq = next_relative_seq() + 1 + stream_out().input_ended();
    return wrap(next_seq,_isn_peer.value()); 
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }


uint64_t TCPReceiver::next_relative_seq() const {return _reassembler.get_first_unassembled();}
