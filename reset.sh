#! /bin/bash

dramabar_ip="10.20.30.22"

# orig defaults:
#
# ?cH=%2300ff00&cS=%23ff0000&cC=%23ffffff&sRf=10&sRb=15&sB=30&lRU=255&lRL=30&lBU=255&lBL=30&bSB=true&bBIn=true&tExp=60&tBExp=10&save=Save+Settings
#

cursor_color="ffffff"
mood_exp_time="60"
button_exp_time="10"

# execute setup
curl -d "cH=%2300ff00&cS=%23ff0000&cC=%23""$cursor_color""&sRf=10&sRb=15&sB=30&lRU=255&lRL=30&lBU=255&lBL=30&bSB=true&bBIn=true&tExp=""$mood_exp_time""&tBExp=""$button_exp_time""&save=Save+Settings" \
    -X POST http://"$dramabar_ip":80/

