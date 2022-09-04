#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <set>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    
    size_t _capacity;    //!< The maximum number of bytes
    size_t stream_start;
    size_t first_unread;
    size_t first_unassembled;
    size_t first_unacceptable;
    bool _eof;
    struct data_seg{
      size_t idx;
      size_t length;
      std::string data;
      data_seg(const std::string& strdata,size_t nidx):idx(nidx),length(strdata.size()),data(strdata){}
      bool operator <(const data_seg& t) const{
        return idx < t.idx;
      }
    };

    std::set<data_seg> store_segs{};
  private:
    typedef struct data_seg data_seg;
  private:
    /*add new seg into store_segs */
    void add_new_seg(const std::string& data,size_t idx);

    /*判断seg能否与别的seg合并,若能则合并*/
    void merge_seg(data_seg& seg);

    /*判定当前有无新的排好序的字节流 有的话输入bytestream*/
    void match_and_write();


  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
