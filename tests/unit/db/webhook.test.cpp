// Copyright (c) 2022, The Monero Project
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

#include <boost/core/demangle.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/uuid/random_generator.hpp>
#include <cstdint>
#include <limits>
#include "common/util.h"   // monero/src/
#include "crypto/crypto.h" // monero/src
#include "db/data.h"
#include "db/storage.h"

namespace
{
  boost::filesystem::path get_db_location()
  {
    return tools::get_default_data_dir() + "light_wallet_server_unit_testing";
  }

  struct cleanup_db
  {
    ~cleanup_db()
    {
      boost::filesystem::remove_all(get_db_location());
    }
  };

  lws::db::storage get_fresh_db()
  {
    const boost::filesystem::path location = get_db_location();
    boost::filesystem::remove_all(location);
    boost::filesystem::create_directories(location);
    return lws::db::storage::open(location.c_str(), 5);
  }

  lws::db::account make_db_account(const lws::db::account_address& pubs, const crypto::secret_key& key)
  {
    lws::db::view_key converted_key{};
    std::memcpy(std::addressof(converted_key), std::addressof(unwrap(unwrap(key))), sizeof(key));
    return {
      lws::db::account_id(1), lws::db::account_time(0), pubs, converted_key
    };
  }

  lws::account make_account(const lws::db::account_address& pubs, const crypto::secret_key& key)
  {
    return lws::account{make_db_account(pubs, key), {}, {}};
  }
}

LWS_CASE("db::storage::*_webhook")
{
  lws::db::account_address account{};
  crypto::secret_key view{};
  crypto::generate_keys(account.spend_public, view);
  crypto::generate_keys(account.view_public, view);

  SETUP("One Account and one Webhook Database")
  {
    cleanup_db on_scope_exit{};
    lws::db::storage db = get_fresh_db();
    const lws::db::block_info last_block =
      MONERO_UNWRAP(MONERO_UNWRAP(db.start_read()).get_last_block());
    MONERO_UNWRAP(db.add_account(account, view));

    const boost::uuids::uuid id = boost::uuids::random_generator{}();
    {
      lws::db::webhook_value value{
        lws::db::webhook_dupsort{500, id},
        lws::db::webhook_data{"http://the_url", "the_token", 3}
      };
      MONERO_UNWRAP(
        db.add_webhook(lws::db::webhook_type::tx_confirmation, account, std::move(value))
      );
    }

    SECTION("get_webhooks()")
    {
      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      const auto result = MONERO_UNWRAP(reader.get_webhooks());
      EXPECT(result.size() == 1);
      EXPECT(result[0].first.user == lws::db::account_id(1));
      EXPECT(result[0].first.type == lws::db::webhook_type::tx_confirmation);
      EXPECT(result[0].second.size() == 1);
      EXPECT(result[0].second[0].first.payment_id == 500);
      EXPECT(result[0].second[0].first.event_id == id);
      EXPECT(result[0].second[0].second.url == "http://the_url");
      EXPECT(result[0].second[0].second.token == "the_token");
      EXPECT(result[0].second[0].second.confirmations == 3);
    }

    SECTION("clear_webhooks(addresses)")
    {
      EXPECT(MONERO_UNWRAP(MONERO_UNWRAP(db.start_read()).get_webhooks()).size() == 1);
      MONERO_UNWRAP(db.clear_webhooks({std::addressof(account), 1}));

      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      const auto result = MONERO_UNWRAP(reader.get_webhooks());
      EXPECT(result.empty());
    }

    SECTION("clear_webhooks(uuid)")
    {
      EXPECT(MONERO_UNWRAP(MONERO_UNWRAP(db.start_read()).get_webhooks()).size() == 1);
      MONERO_UNWRAP(db.clear_webhooks({id}));

      lws::db::storage_reader reader = MONERO_UNWRAP(db.start_read());
      const auto result = MONERO_UNWRAP(reader.get_webhooks());
      EXPECT(result.empty());
    }
  }
}
