#include "stream_reassembler.hh"
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/*
|<----------------------StreamReassembler.capacity---------------------|
|---<first_unread>---------<first_unassembled>-----<first_unacceptable>|
*/
StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),
                                                              stream_start(0),first_unread(0),
                                                              first_unassembled(0),first_unacceptable(0),_eof(false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) 
{
    first_unread = _output.bytes_read();
    first_unassembled = _output.bytes_written();
    first_unacceptable = first_unread + _capacity;
    add_new_seg(data,index);
    match_and_write();

    //printf("index[%lld],parm_eof[%d],ori_eof[%d]\n",static_cast<long long>(index),eof,_eof);
    if(eof == true && _eof == false && index + data.size() <=  first_unread + _capacity)
         _eof = true;
    if(empty() && (_eof||eof)){
        _output.end_input();
    }
}

void StreamReassembler::match_and_write(){
    while(!store_segs.empty() && store_segs.begin()->idx == first_unassembled){
        _output.write(store_segs.begin()->data);
        store_segs.erase(store_segs.begin());
        first_unassembled = _output.bytes_written();
    }
}


void StreamReassembler::add_new_seg(const string& data,size_t idx){
   
    /*截取能放入当前StreamReassembler的数据流*/
    if(idx >= first_unacceptable || idx + data.size() <= first_unassembled){
        /*不在接收窗口内的字节流 抛弃 || 该字节流已经被读取过了 抛弃*/
        return ;
    }
    if(idx < first_unassembled){
        /*前部分已经被接收 找出未被接收的部分*/
        size_t len = idx+data.size() - first_unassembled;
        string store_data = data.substr(first_unassembled-idx,len);
        data_seg new_seg = data_seg(store_data,first_unassembled);

        merge_seg(new_seg);
    }
    else if(idx < first_unacceptable){
        /*起始序号落在无序区域 找出能放的最长流*/
        size_t data_end_idx = min(idx+data.size(),first_unacceptable);
        string store_data = data.substr(0,data_end_idx-idx);
        if(store_data.size() == 0){
            return ;
        }

        data_seg new_seg = data_seg(store_data,idx);
        merge_seg(new_seg);
    }

}

void StreamReassembler::merge_seg(data_seg& seg){
    if(store_segs.empty()){
        store_segs.insert(seg);
    }
    else{
        

        for(auto iter = store_segs.begin(); iter != store_segs.end(); ){
            size_t cmp_start_idx = iter->idx, cmp_end_idx = iter->idx + iter->length;
            size_t cur_start_idx = seg.idx, cur_end_idx = seg.idx+seg.length;
            
            if(cur_end_idx < cmp_start_idx){
                store_segs.insert(seg);
                return ;
            }
            else if(cur_start_idx > cmp_end_idx){
                ++iter;
                continue;
            }
            else{
                /*交集情况*/
                /*包含*/
                if( cmp_start_idx>=cur_start_idx && cmp_end_idx<=cur_end_idx) {
                    iter = store_segs.erase(iter);
                    continue;
                }
                else if( cur_start_idx>= cmp_start_idx && cmp_end_idx>= cur_end_idx){
                    return ;
                }
                /*相交一部分*/
                else{
                    if(cur_end_idx == cmp_start_idx){
                        seg.idx = cur_start_idx;
                        seg.data += iter->data;
                        seg.length = seg.data.size();
                    }
                    else if(cur_start_idx == cmp_end_idx){
                        seg.idx = cmp_start_idx;
                        seg.data = iter->data + seg.data;
                        seg.length = seg.data.size();
                    }
                    else if(cur_end_idx < cmp_end_idx){
                        string tmp = seg.data.substr(0,cmp_start_idx-cur_start_idx);
                        seg.idx = cur_start_idx;
                        seg.data = tmp + iter->data;
                        seg.length = seg.data.size();
                    }
                    else if(cur_end_idx > cmp_end_idx){
                        string tmp = iter->data.substr(0,cur_start_idx-cmp_start_idx);
                        seg.idx = cmp_start_idx;
                        seg.data = tmp + seg.data;
                        seg.length = seg.data.size();
                    }
                    iter = store_segs.erase(iter);
                    continue;
                }
            }
        }
        store_segs.insert(seg);
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t ret = 0;
    for(auto iter = store_segs.begin(); iter != store_segs.end(); ++iter){
        ret += iter->length;
    }
    return ret;
 }

bool StreamReassembler::empty() const { return unassembled_bytes()==0; }
