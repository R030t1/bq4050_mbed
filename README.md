# bq4050_mbed
Pull data from a TI bq4050 battery protection and monitoring chip.

# Requirements
1. `pip install mbed-cli protobuf grpcio-tools scons`
2. `nanopb/generator/protoc --nanopb_out=./ bq4050.proto 
