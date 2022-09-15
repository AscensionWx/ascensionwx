These files are used for handling of the LoRaWAN messages. Hardware examples include:

- UG67 and UG65 Milesight IoT gateway
- Raspberry Pi
- Small AWS EC2 instance

These flows can be imported by:

1. Installing Node-RED globally
2. Updating your $HOME/.node-red/package.json file to include the following property.
```
    "dependencies": {
        "node-red-contrib-telos-eosio": <path to where you will run node-red>
    }
```
  And then run "npm install" to take effect

3. Go to the location where you want to run node-red and the flow file. Cloning external git repo
```
git clone https://github.com/sunburntcat/node-red-contrib-telos-eosio.git
```
4. Install packages
```
cd node-red-contrib-telos-eosio
npm install
```
5. Running node-red with the pre-built flow file
```
node-red <flow_filename>.json
```
6. Configuring MQTT and other nodes with appropriate credentials

