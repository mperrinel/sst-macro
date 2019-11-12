#pragma once // We promise to use a recent clang so this is fine

/**
Copyright 2009-2019 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2019, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <iostream>
#include <tuple>

#include <chrono>
#include <map>
#include <utility>
#include <vector>

namespace memoize {
#if __cplusplus >= 201703L
template <typename... Args> void print_types(Args &&... args) {
  std::cout << "Types: ";
  (..., (std::cout << args << " "));
  std::cout << "\n";
}

template <typename... Args> class Capture {
public:
  Capture(char const *unique_id) : unique_id_(unique_id) {}

  void capture_start(char const *unique_id, Args ...args) {
    assert(unique_id_ == unique_id);
    arg_buffer_ = std::make_tuple(args...);
    start_setup();
  }

  void capture_stop(char const *unique_id) {
    assert(unique_id_ == unique_id);
    stop_setup();
  }

  std::tuple<Args...> const &StoredArgs() const { return arg_buffer_; }
  std::string const &ID() const { return unique_id_; }

private:
  virtual void start_setup(){};
  virtual void stop_setup(){};

private:
  std::string unique_id_;
  std::tuple<Args...> arg_buffer_;
};

template <typename... Args> class TimerPrinter : public Capture<Args...> {
public:
  TimerPrinter(char const *unique_id) : Capture<Args...>(unique_id) {}

private:
  void start_setup() override {
    t0_ = std::chrono::high_resolution_clock::now();
  }

  void stop_setup() override {
    t1_ = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration<double>(t1_ - t0_).count();
    std::cout << "Took " << time << " seconds\n\t";
    // std::apply(print_types<Args...>, this->StoredArgs());
    rows_.push_back(std::tuple_cat(this->StoredArgs(), std::tie(time)));
  }

  std::chrono::high_resolution_clock::time_point t0_;
  std::chrono::high_resolution_clock::time_point t1_;
  std::vector<std::tuple<Args..., double>> rows_;
};

// TODO replace type with global stuffs
template <typename... Args> Capture<Args...> *getCaptureType(char const *id) {
  switch (0) {
  default:
    return new TimerPrinter<Args...>(id);
  }
}

#else // Not c++17 or greater
template <typename... Args> void print_types(Args &&... args) {
  std::cout << "Number of args is: " << sizeof...(Args);
}

template <typename... Args> class Capture {
public:
  Capture(char const *unique_id) : unique_id_(unique_id) {}

  void capture_start(char const *unique_id, Args ...args) {
    assert(unique_id_ == unique_id);
    arg_buffer_ = std::make_tuple(args...);
    start_setup();
  }

  void capture_stop(char const *unique_id) {
    assert(unique_id_ == unique_id);
    stop_setup();
  }

  std::tuple<Args...> const &StoredArgs() const { return arg_buffer_; }
  std::string const &ID() const { return unique_id_; }

private:
  virtual void start_setup(){};
  virtual void stop_setup(){};

private:
  std::string unique_id_;
  std::tuple<Args...> arg_buffer_;
};

template <typename... Args> class TimerPrinter : public Capture<Args...> {
public:
  TimerPrinter(char const *unique_id) : Capture<Args...>(unique_id) {}

private:
  void start_setup() override {
    t0_ = std::chrono::high_resolution_clock::now();
  }

  void stop_setup() override {
    t1_ = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration<double>(t1_ - t0_).count();
    std::cout << "Took " << time << " seconds\n\t";
    // std::apply(print_types<Args...>, this->StoredArgs());
    rows_.push_back(std::tuple_cat(this->StoredArgs(), std::tie(time)));
  }

  std::chrono::high_resolution_clock::time_point t0_;
  std::chrono::high_resolution_clock::time_point t1_;
  std::vector<std::tuple<Args..., double>> rows_;
};

// TODO replace type with global stuffs
template <typename... Args> Capture<Args...> *getCaptureType(char const *id) {
  switch (0) {
  default:
    return new TimerPrinter<Args...>(id);
  }
}

#endif
} // namespace memoize
