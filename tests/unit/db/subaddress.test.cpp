// Copyright (c) 2023, The Monero Project
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

#include "framework.test.h"

#include <boost/uuid/random_generator.hpp>
#include <cstdint>
#include "crypto/crypto.h" // monero/src
#include "db/data.h"
#include "db/storage.h"
#include "db/storage.test.h"
#include "error.h"

LWS_CASE("db::storage::upsert_subaddresses")
{
  lws::db::account_address account{};
  crypto::secret_key view{};
  crypto::generate_keys(account.spend_public, view);
  crypto::generate_keys(account.view_public, view);

  SETUP("One Account")
  {
    lws::db::test::cleanup_db on_scope_exit{};
    lws::db::storage db = lws::db::test::get_fresh_db();
    const lws::db::block_info last_block =
      MONERO_UNWRAP(MONERO_UNWRAP(db.start_read()).get_last_block());
    MONERO_UNWRAP(db.add_account(account, view));

    SECTION("Empty get_subaddresses")
    {
      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      EXPECT(MONERO_UNWRAP(reader.get_subaddresses(lws::db::account_id(1))).empty());
    }

    SECTION("Upsert Basic")
    {
      std::vector<lws::db::subaddress_dict> subs{};
      subs.emplace_back(
        lws::db::major_index(0),
        lws::db::index_ranges{lws::db::index_range{lws::db::minor_index(1), lws::db::minor_index(100)}}
      );
      auto result = db.upsert_subaddresses(lws::db::account_id(1), account, view, subs, 100);
      EXPECT(result.has_value());
      EXPECT(result->size() == 1);
      EXPECT(result->at(0).first == lws::db::major_index(0));
      EXPECT(result->at(0).second.size() == 1);
      EXPECT(result->at(0).second[0][0] == lws::db::minor_index(1));
      EXPECT(result->at(0).second[0][1] == lws::db::minor_index(100));

      subs.back().first = lws::db::major_index(1);
      result = db.upsert_subaddresses(lws::db::account_id(1), account, view, subs, 199);
      EXPECT(result.has_error());
      EXPECT(result == lws::error::max_subaddresses);

      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      const auto fetched = reader.get_subaddresses(lws::db::account_id(1));
      EXPECT(fetched.has_value());
      EXPECT(fetched->size() == 1);
      EXPECT(fetched->at(0).first == lws::db::major_index(0));
      EXPECT(fetched->at(0).second.size() == 1);
      EXPECT(fetched->at(0).second[0][0] == lws::db::minor_index(1));
      EXPECT(fetched->at(0).second[0][1] == lws::db::minor_index(100));
    }

    SECTION("Upsert Appended")
    {
      std::vector<lws::db::subaddress_dict> subs{};
      subs.emplace_back(
        lws::db::major_index(0),
        lws::db::index_ranges{lws::db::index_range{lws::db::minor_index(1), lws::db::minor_index(100)}}
      );
      auto result = db.upsert_subaddresses(lws::db::account_id(1), account, view, subs, 100);
      EXPECT(result.has_value());
      EXPECT(result->size() == 1);
      EXPECT(result->at(0).first == lws::db::major_index(0));
      EXPECT(result->at(0).second.size() == 1);
      EXPECT(result->at(0).second[0][0] == lws::db::minor_index(1));
      EXPECT(result->at(0).second[0][1] == lws::db::minor_index(100));

      subs.back().second =
        lws::db::index_ranges{lws::db::index_range{lws::db::minor_index(101), lws::db::minor_index(200)}};
      result = db.upsert_subaddresses(lws::db::account_id(1), account, view, subs, 200);
      EXPECT(result.has_value());
      EXPECT(result->size() == 1);
      EXPECT(result->at(0).first == lws::db::major_index(0));
      EXPECT(result->at(0).second.size() == 1);
      EXPECT(result->at(0).second[0][0] == lws::db::minor_index(101));
      EXPECT(result->at(0).second[0][1] == lws::db::minor_index(200));

      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      const auto fetched = reader.get_subaddresses(lws::db::account_id(1));
      EXPECT(fetched.has_value());
      EXPECT(fetched->size() == 1);
      EXPECT(fetched->at(0).first == lws::db::major_index(0));
      EXPECT(fetched->at(0).second.size() == 1);
      EXPECT(fetched->at(0).second[0][0] == lws::db::minor_index(1));
      EXPECT(fetched->at(0).second[0][1] == lws::db::minor_index(200));
    }
  }
}
