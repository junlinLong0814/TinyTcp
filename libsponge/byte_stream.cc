#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_capacity(capacity),
                                                _totalw(0),_totalr(0),
                                                _bendin(false),_error(false){}
                                               

size_t ByteStream::write(const string &data) {
    size_t write_size = min(data.size(),remaining_capacity());
    for(size_t i = 0 ; i < write_size; ++i){
        deq.push_back(data[i]);
    }
    _totalw += write_size;
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t able_read_size =min(buffer_size(),len);
    string ret(able_read_size,'\0');
    for(size_t i = 0,j = 0; i < able_read_size; ++i,++j){
        ret[i] = deq[j];
    }

    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t able_read_size = min(buffer_size(),len);
    for(size_t i = 0 ; i < able_read_size; ++i){
        deq.pop_front();
    }
    _totalr += able_read_size;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() {
    _bendin = true;
}

bool ByteStream::input_ended() const { 
    return _bendin;
}

size_t ByteStream::buffer_size() const { 
    return  deq.size();
}

bool ByteStream::buffer_empty() const {return buffer_size()==0;}

bool ByteStream::eof() const { return buffer_empty() && _bendin; }

size_t ByteStream::bytes_written() const { return _totalw;}

size_t ByteStream::bytes_read() const { return _totalr; }

size_t ByteStream::remaining_capacity() const { return _capacity-buffer_size(); }
