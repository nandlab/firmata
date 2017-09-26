////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#ifndef FIRMATA_PINS_HPP
#define FIRMATA_PINS_HPP

////////////////////////////////////////////////////////////////////////////////
#include "firmata/pin.hpp"
#include "firmata/types.hpp"

#include <algorithm>
#include <utility>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace firmata
{

////////////////////////////////////////////////////////////////////////////////
struct pins
{
    ////////////////////
    pins() = default;

    pins(const pin&) = delete;
    pins(pins&&) = default;

    pins& operator=(const pins&) = delete;
    pins& operator=(pins&&) = default;

    ////////////////////
    auto begin() noexcept { return pins_.begin(); }
    const auto begin() const noexcept { return pins_.begin(); }

    auto end() noexcept { return pins_.end(); }
    const auto end() const noexcept { return pins_.end(); }

    auto count() const noexcept { return pins_.size(); }
    auto count(mode n) const noexcept
    {
        return std::count_if(begin(), end(),
            [&](auto& pin){ return pin.supports(n); }
        );
    }

    ////////////////////
    auto& get(pos n) { return pins_.at(n); }
    const auto& get(pos n) const { return pins_.at(n); }

    auto& get(analog n) { return get(analog_in, n); }
    auto const& get(analog n) const { return get(analog_in, n); }

    firmata::pin& get(mode, pos);
    const firmata::pin& get(mode, pos) const;


private:
    ////////////////////
    std::vector<firmata::pin> pins_;

    void push_back(firmata::pin&& pin) { pins_.push_back(std::move(pin)); }
    auto& back() noexcept { return pins_.back(); }

    friend class command;
};

////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////
#endif