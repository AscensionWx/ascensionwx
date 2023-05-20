Changes in this version:

Remove dead ABP code
Added sleep_light_millis()
Added MESSAGE_PENDING variable that gets set to false on TXCOMPLETE
Wait exactly until RX windows close after observation (saves battery)
wait_for_gps_time_and_location wait 10 seconds for GPS serial
GPS on for 5 minutes roughly every 3 days
Sends GPS message after on for 5 minutes
Remove RTC sync with every observation (was causing 1-second delay)


Todo:
GPS always 5 minutes every 3 days
GPS message 5 second wait for TX/RX output, then send
Send data every 20 minutes?

Test:
