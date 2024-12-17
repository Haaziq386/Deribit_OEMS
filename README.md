# Derbit_OEM

## Usage

clone this repo (use ubuntu)

make  .env file of form in root directory

API_KEY = "YOUR_API_KEY";

SECRET_KEY = "YOUR_SECRET_CLIENT_KEY";

cd build/

cmake ..

make 

./deribit_order_management


For server

cd server/

g++ websocket_server.cpp utils.cpp -lcurl

./a.out
