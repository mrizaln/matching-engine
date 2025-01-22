# matching-engine

Order matching engine

## Requirements

The implementation must include

- Accepting orders

  Both buy and sell orders must be handled, on which the orders have the following information,

  - `id`: unique identifier;
  - `type`: buy or sell;
  - `price`: The price at which the buyer is willing to buy or the seller is willing to sell;
  - `quantity`: The quantity of shares the buyer wants to buy or the seller wants to sell.

- Matching orders

  The matching engine should match buy and sell orders in a fair and efficient manner: **when a buy and sell order have the same price, they should be matched based on the time they were received**.

- Partial fills

  The matching engine should support partial fills, which means that an order can be matched with multiple orders, and the quantity of shares bought or sold can be less than the quantity of shares specified in the original order.

- TCP/IP connection

  The matching engine should receive orders and send matched trades via TCP/IP. You should also provide a data generator program that can generate high-speed order data via TCP/IP.

- Efficiency

  The implementation should be optimized for speed and able to handle a large volume of orders and trades.

- Deliverables

  - [x] Source code for your matching engine implementation in C++.
  - [x] A user manual that includes instructions for compiling and running your code, as well as detailed specification for the format of orders and trades over the TCP/IP connection.
  - [x] A test plan that outlines how you tested your implementation and includes sample input and output data.
  - [x] The data generator program that can generate high-speed order via TCP/IP.

The implementation will be graded on the following criteria:

- Correctness
- Efficiency
- TCP/IP connection
- Code quality
- Testing

## Dependencies

Build dependencies

- C++20 capable compiler (with coroutines support)
- Conan 2
- CMake 3.16+

All the dependencies of the project are managed using Conan.

- fmt
- asio
- spdlog
- rapidjson
- ut (test dependency)

## Building

The project dependencies are managed by Conan and constructed using CMake, make sure both installed and configured properly ([read here for Conan](https://docs.conan.io/2/installation.html)).

The first step is getting all the dependencies from Conan remotes,

```sh
conan install . --build missing -s build_type=Debug
```

Then the next step is to configure the project using CMake

```sh
cmake --preset conan-release    # if you are on Windows, use conan-default instead
```

The last step is building the project

```sh
cmake --build --preset conan-release
```

The built binary is in the `build/Release` directory,

```sh
# running on port 8080
./build/Release/main 8080      # has .exe extension if on Windows
```

## Usage

The matching engine is ran as a TCP server, it can receive both buy and sell orders. The engine uses a very simple Message protocol implemented as Length-Value Encoded string. To run the engine, You just need to provide a port number on which the engine will be running on.

```sh
./build/Release/main 8080
```

### Connecting

To connect to the engine, one must use typical TCP socket of the platform to the port that has been chosen.

### Make an order

To make an order, the sender must send a JSON string in the shape of,

```json
{
  "price":    <uint>,
  "quantity": <uint>,
  "type":     <"buy"|"sell">
}
```

For example, if you want to make an order to buy a stock at price `100` with quantity `20`, you construct a JSON like so,

```json
{
  "price": 100,
  "quantity": 20,
  "type": "buy"
}
```

The next step is to send the JSON as string to the engine through such socket

> `tldr`: See the [generator program](test/server_test_client.py) to see an example on how to make an order.

### Message protocol

As previously said, the engine use Length-Value encoding for the TCP message sent. The specification for this is very simple:

```
offset  0 . . 3 . . . . . . . . . .
        [ len ] [data...           ]
```

There are two part to the protocol which is length in [`network long`](https://www.ibm.com/docs/ja/zvm/7.2?topic=domains-network-byte-order-host-byte-order) and the rest is the data. Every message must be lain in a form that the first 4 bytes is the length of the data, and the rest is the data itself with the length of previously set.

For example, for the message `"Hello"` it has 5 bytes of data. In the underlying data then it laid out like this

```
| length    | data         |
[00 00 00 05 48 65 6c 6c 6f]
```

> `tldr`: See the [generator program](test/server_test_client.py) to see an example on how the protocol is implemented.

## Output

Since the sheet does not mention any output handling, where when a matching order is reached, I assume outputting the result to `stdout` would be sufficient.

## Test plan

There are two parts to testing:

### Unit test

The project is structured so that the core matching engine and the TCP server is separated. This leads to easier unit tests to each component. Both the core matching engine component and the TCP server component, unit tests performed using [`ut`](https://github.com/boost-ext/ut) library. The library is easy to use and configure.

Unit tests performed to tests:

- order accumulation,
- order matching fully on one order,
- partial fill for an order with many order,
- order matching prioritization (older first),
- matching engine benchmark, and
- TCP server message protocol implementation.

> see the unit tests in [test](test) directory

### Integration test

The integration tests is more difficult that unit tests. At the moment, I haven't done a streamlined integration test for this project due to time constraint. What I have done is creating an order generator script that generates large volume of trades in Python that feeds into the engine through TCP. Using the trades, stress tests performed repeatedly and any error encountered immediately fixed and eliminated.

If it is possible, I would like to streamline this test by creating a predefined stocks list in a text file that then sent to the server. The output of the server then captured in a file or piped. The next step is to compare result to the predefined text file. This is very similar to my unit test, but instead of directly feeding the order to the engine in program in C++, the orders need to go through hoops first before matched. The test will probably using a python script to run the executable project and capture the output from then.
