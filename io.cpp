////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2017 Dimitry Ishenko
//
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL v3.
// For full terms see COPYING or visit https://www.gnu.org/licenses/gpl.html

////////////////////////////////////////////////////////////////////////////////
#include "firmata/io.hpp"

#include <asio.hpp>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{
namespace io
{

////////////////////////////////////////////////////////////////////////////////
serial::serial(asio::io_service& io, const std::string& device) :
    port_(io, device)
{ sched_read(); }

serial::~serial() noexcept { asio::error_code ec; port_.cancel(ec); }

////////////////////////////////////////////////////////////////////////////////
void serial::set(baud_rate    baud) { port_.set_option(asio::serial_port::baud_rate(baud)); }
void serial::set(flow_control flow) { port_.set_option(asio::serial_port::flow_control(flow)); }
void serial::set(parity       pari) { port_.set_option(asio::serial_port::parity(pari)); }
void serial::set(stop_bits    bits) { port_.set_option(asio::serial_port::stop_bits(bits)); }
void serial::set(char_size    bits) { port_.set_option(asio::serial_port::character_size(bits)); }

////////////////////////////////////////////////////////////////////////////////
void serial::write(msg_id id, const payload& data)
{
    std::vector<asio::const_buffer> message;

        message.push_back(asio::buffer(&id, size(id)));
    if(data.size())
        message.push_back(asio::buffer(data));
    if(is_sysex(id))
        message.push_back(asio::buffer(&end_sysex, sizeof(end_sysex)));

    asio::write(port_, message);
}

////////////////////////////////////////////////////////////////////////////////
std::tuple<msg_id, payload> serial::read()
{
    msg_id id = no_id;
    payload message;

    callback fn = [&](msg_id _id, const payload& _message)
    { id = _id; message = _message; };

    std::swap(fn, fn_);
    do port_.get_io_service().run_one(); while(id == no_id);
    std::swap(fn, fn_);

    return std::make_tuple(id, message);
}

////////////////////////////////////////////////////////////////////////////////
void serial::set_callback(callback fn) { fn_ = std::move(fn); }

////////////////////////////////////////////////////////////////////////////////
void serial::sched_read()
{
    asio::async_read(port_, store_,
        asio::transfer_at_least(3),
        std::bind(&serial::async_read, this, std::placeholders::_1)
    );
}

////////////////////////////////////////////////////////////////////////////////
void serial::async_read(const asio::error_code& ec)
{
    if(ec) return;

    while(store_.size() >= 3)
    {
        auto begin = asio::buffers_begin(store_.data());
        auto end = asio::buffers_end(store_.data());
        auto ci = begin, ci_end = ci;

        // get message id
        msg_id id = static_cast<msg_id>(*ci++);
        payload message;

        // if this is a sysex message, get sysex id
        if(is_sysex(id))
        {
            byte sid = *ci++;
            id = sysex(sid);

            // if this is an extended sysex message, get extended id
            if(is_ext_sysex(id))
            {
                // need 2 bytes for extended id
                if(store_.size() < 4) break;

                word eid = word(*ci++) + (word(*ci++) << 8);
                id = ext_sysex(eid);
            }

            // find end_sysex
            for(ci_end = ci; ci_end != end && *ci_end != char(end_sysex); ++ci_end);

            // end_sysex not found
            if(ci_end == end) break;
        }
        else
        {
            // standard message
            ci_end = ci + 2;
        }

        message.insert(message.end(), ci, ci_end);
        store_.consume(ci_end - begin + (is_sysex(id) ? 1 : 0));

        if(fn_) fn_(id, message);
    }

    sched_read();
}

////////////////////////////////////////////////////////////////////////////////
}
}
