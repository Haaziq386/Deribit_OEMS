# Derbit_OEMS

## Usage

clone this repo (use ubuntu)

cd build/

cmake ..

make 

./deribit_order_management


For running server

cd server/

g++ websocket_server.cpp utils.cpp -lcurl

./a.out
