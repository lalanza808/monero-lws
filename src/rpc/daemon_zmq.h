// Copyright (c) 2020, The Monero Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cstdint>
#include <vector>

#include "common/pod-class.h" // monero/src
#include "wire/json/fwd.h"

namespace crypto
{
  POD_CLASS hash;
}

namespace cryptonote
{
  class transaction;
  void read_bytes(wire::json_reader& source, transaction& self);

  namespace rpc
  {
    struct block_with_transactions;
  }
}

namespace lws
{
namespace rpc
{
  struct get_blocks_fast_request
  {
    get_blocks_fast_request() = delete;
    std::vector<crypto::hash> block_ids;
    std::uint64_t start_height;
    bool prune;
  };
  struct get_blocks_fast_response
  {
    get_blocks_fast_response() = delete;
    std::vector<cryptonote::rpc::block_with_transactions> blocks;
    std::vector<std::vector<std::vector<std::uint64_t>>> output_indices;
    std::uint64_t start_height;
    std::uint64_t current_height;
  };
  struct get_blocks_fast
  {
    using request = get_blocks_fast_request;
    using response = get_blocks_fast_response;
  };
  void read_bytes(wire::json_reader&, get_blocks_fast_response&);
} // rpc
} // lws
