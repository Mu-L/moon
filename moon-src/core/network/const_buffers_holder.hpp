#pragma once
#include "config.hpp"
#include "common/buffer.hpp"
#include "asio.hpp"

namespace moon
{
    class const_buffers_holder
    {
    public:
        const_buffers_holder() = default;

        template<typename BufType>
        void push_back(BufType&& buf)
        {
            if (buf->has_flag(buffer_flag::close))
            {
                close_ = true;
            }
            buffers_.emplace_back(buf->data(), buf->size());
            datas_.push_back(std::forward<BufType>(buf));
        }


        void framing_begin(size_t framing_size)
        {
            headers_.reserve(framing_size);
        }

        void push_framing(message_size_t header, const char* data, size_t len)
        {
            headers_.push_back(header);
            message_size_t& back = headers_.back();
            buffers_.emplace_back(reinterpret_cast<const char*>(&back), sizeof(back));
            buffers_.emplace_back(data, len);
        }

        template<typename BufType>
        void framing_end(BufType&& buf)
        {
            datas_.push_back(std::forward<BufType>(buf));
        }

        const auto& buffers() const
        {
            return buffers_;
        }

        size_t size() const
        {
            return buffers_.size();
        }

        void clear()
        {
            close_ = false;
            buffers_.clear();
            datas_.clear();
            headers_.clear();
        }

        bool close() const
        {
            return close_;
        }
    private:
        bool close_ = false;
        std::vector<asio::const_buffer> buffers_;
        std::vector<buffer_ptr_t> datas_;
        std::vector<message_size_t> headers_;
    };

    /*
    Don't pass heavy-weight buffer sequences to async IO operations
    https://github.com/chriskohlhoff/asio/issues/203
     */
    template<class BufferSequence>
    class buffers_ref
    {
        BufferSequence const& buffers_;

    public:
        using value_type = typename BufferSequence::value_type;

        using const_iterator = typename BufferSequence::const_iterator;

        buffers_ref(buffers_ref const&) = default;

        explicit
            buffers_ref(BufferSequence const& buffers)
            : buffers_(buffers)
        {
        }

        const_iterator
            begin() const
        {
            return buffers_.begin();
        }

        const_iterator
            end() const
        {
            return buffers_.end();
        }
    };

    // Return a reference to a buffer sequence
    template<class BufferSequence>
    buffers_ref<BufferSequence>
        make_buffers_ref(BufferSequence const& buffers)
    {
        return buffers_ref<BufferSequence>(buffers);
    }
}






