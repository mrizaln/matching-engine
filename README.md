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

  - Source code for your matching engine implementation in C++.
  - A user manual that includes instructions for compiling and running your code, as well as detailed specification for the format of orders and trades over the TCP/IP connection.
  - A test plan that outlines how you tested your implementation and includes sample input and output data.
  - The data generator program that can generate high-speed order via TCP/IP.

The implementation will be graded on the following criteria:

- Correctness
- Efficiency
- TCP/IP connection
- Code quality
- Testing
