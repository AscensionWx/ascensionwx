These files are used for handling of the LoRaWAN messages. Hardware examples include:

- UG67 and UG65 Milesight IoT gateway
- Raspberry Pi
- Small AWS EC2 instance

These flows can be imported by:

1. Installing Node-RED globally
2. Cloning git repo
```
git clone https://github.com/sunburntcat/node-red-contrib-telos-eosio.git
```
3. Install packages
```
cd node-red-contrib-telos-eosio
npm install
```
4. Running node-red with the pre-built flow file
```
node-red <flow_filename>.json
```
5. Configuring MQTT and other nodes with appropriate credentials

