
#include <boost/utility/string_ref.hpp>
#include <chrono>
#include <string>
#include "byte_slice.h"      // monero/contrib/epee/include
#include "misc_log_ex.h"             // monero/contrib/epee/include
#include "net/http_client.h" // monero/contrib/epee/include
#include "span.h"
#include "wire/json.h"

namespace lws { namespace rpc
{
  namespace net = epee::net_utils;

  template<typename T>
  void http_send(net::http::http_simple_client& client, boost::string_ref uri, const T& event, const net::http::fields_list& params, const std::chrono::milliseconds timeout)
  {
    if (uri.empty())
      uri = "/";

    epee::byte_slice bytes{};
    const std::string& url = event.value.second.url;
    const std::error_code json_error = wire::json::to_bytes(bytes, event);
    const net::http::http_response_info* info = nullptr;
    if (json_error)
    {
      MERROR("Failed to generate webhook JSON: " << json_error.message());
      return;
    }

    MINFO("Sending webhook to " << url);
    if (!client.invoke(uri, "POST", std::string{bytes.begin(), bytes.end()}, timeout, std::addressof(info), params))
    {
      MERROR("Failed to invoke http request to  " << url);
      return;
    }

    if (!info)
    {
      MERROR("Failed to invoke http request to  " << url << ", internal error (null response ptr)");
      return;
    }

    if (info->m_response_code != 200)
    {
      MERROR("Failed to invoke http request to  " << url << ", wrong response code: " << info->m_response_code);
      return;
    }
  }

  template<typename T>
  void http_send(const epee::span<const T> events, const std::chrono::milliseconds timeout, net::ssl_verification_t verify_mode)
  {
    if (events.empty())
      return;

    net::http::url_content url{};
    net::http::http_simple_client client{};

    net::http::fields_list params;
    params.emplace_back("Content-Type", "application/json; charset=utf-8");

    for (const auto& event : events)
    {
      if (event.value.second.url.empty() || !net::parse_url(event.value.second.url, url))
      {
        MERROR("Bad URL for webhook event: " << event.value.second.url);
        continue;
      }

      const bool https = (url.schema == "https");
      if (!https && url.schema != "http")
      {
        MERROR("Only http or https connections: " << event.value.second.url);
        continue;
      }

      const net::ssl_support_t ssl_mode = https ?
        net::ssl_support_t::e_ssl_support_enabled : net::ssl_support_t::e_ssl_support_disabled;
      net::ssl_options_t ssl_options{ssl_mode};
      if (https)
        ssl_options.verification = verify_mode;

      if (url.port == 0)
        url.port = https ? 443 : 80;

      client.set_server(url.host, std::to_string(url.port), boost::none, std::move(ssl_options));
      if (client.connect(timeout))
        http_send(client, url.uri, event, params, timeout);
      else
        MERROR("Unable to send webhook to " << event.value.second.url);

      client.disconnect();
    }
  }

}} // lws // rpc
