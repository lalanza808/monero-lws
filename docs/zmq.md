# monero-lws ZeroMQ Usage
Monero-lws uses ZeroMQ-RPC to retrieve information from a Monero daemon,
ZeroMQ-SUB to get immediate notifications of blocks and transactions from a
Monero daemon, and ZeroMQ-PUB to notify external applications of payment_id
(web)hooks.

## External "pub" socket
The bind location of the ZMQ-PUB socket is specified with the `--zmq-pub`
option. Users are still required to "subscribe" to topics:
  * `json-full-hooks`: A JSON array of webhook events that have recently
    triggered (identical output as webhook).
  * `msgpack-full-hooks`: A msgpack array of webhook events that have recently
    triggered (identical output as webhook).


### `json-full-hooks`/`msgpack-full-hooks`i
These topics receive PUB messages when a webhook ([`webhook_add`](administration.md)),
event is triggered. If the specified URL is `zmq`, then notifications are only
done over the ZMQ-PUB socket, otherwise the notification is sent over ZMQ-PUB
socket AND the specified URL. Invoking `webhook_add` with a `payment_id` of
zeroes (the field is optional in `webhook_add), will match on all transactions
that lack a payment_id`.

Example of the "raw" output from ZMQ-SUB side:

```json
json-full-hooks:[
  {
    "event": "tx-confirmation",
    "payment_id": "9bc1a59b34253896",
    "token": "12345",
    "confirmations": 4,
    "event_id": "4dc201838af54dfe88686bea7e2b599f",
    "tx_info": {
      "id": {
        "high": 0,
        "low": 5664688
      },
      "block": 2264293,
      "index": 0,
      "amount": 100000000000,
      "timestamp": 1687110127,
      "tx_hash": "40718409ed636a9ed25f6855704f22e09d04f1093bbfce9780cbc5550972fc9f",
      "tx_prefix_hash": "20600af2f06c4854de96177d6e4c825d7dc59fb04eded2bab6c209557eb2a53c",
      "tx_public": "018899ed5e8c1e61c6d7eec7398a29dcd04506d5c8ad94f22a4580a658a7e10a",
      "rct_mask": "85fb4ae09147cf026119eab99879f3b9bd7bc8bbefdf6f782c24fe76cd322c03",
      "payment_id": "9bc1a59b34253896",
      "unlock_time": 0,
      "mixin_count": 15,
      "coinbase": false
    }
  },
  {
    "event": "tx-confirmation",
    "payment_id": "9bc1a59b34253896",
    "token": "12345",
    "confirmations": 4,
    "event_id": "615171e477464401a1a23cdb45b3b433",
    "tx_info": {
      "id": {
        "high": 0,
        "low": 5664688
      },
      "block": 2264293,
      "index": 0,
      "amount": 100000000000,
      "timestamp": 1687110127,
      "tx_hash": "40718409ed636a9ed25f6855704f22e09d04f1093bbfce9780cbc5550972fc9f",
      "tx_prefix_hash": "20600af2f06c4854de96177d6e4c825d7dc59fb04eded2bab6c209557eb2a53c",
      "tx_public": "018899ed5e8c1e61c6d7eec7398a29dcd04506d5c8ad94f22a4580a658a7e10a",
      "rct_mask": "85fb4ae09147cf026119eab99879f3b9bd7bc8bbefdf6f782c24fe76cd322c03",
      "payment_id": "9bc1a59b34253896",
      "unlock_time": 0,
      "mixin_count": 15,
      "coinbase": false
    }
  }
]
```

Notice the `json-full-hooks:` prefix - this is required for the ZMQ PUB/SUB
subscription model. The subscriber requests data from a certain "topic" where
matching is done by string prefixes.

> The `block` and `id` fields in the above example are NOT present when
`confirmations == 0`.
