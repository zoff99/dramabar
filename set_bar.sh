#! /bin/bash

dramabar_ip="10.20.30.22"

cursor_color="e3ee00"
mood_exp_time="69000"
button_exp_time="1"
bar_value="$1"

# execute setup
curl -d "cH=%2300ff00&cS=%23ff0000&cC=%23""$cursor_color""&sRf=10&sRb=15&sB=""$bar_value""&lRU=255&lRL=30&lBU=255&lBL=30&bSB=true&bBIn=true&tExp=""$mood_exp_time""&tBExp=""$button_exp_time""&save=Save+Settings" \
    -X POST http://"$dramabar_ip":80/ >/dev/null 2>&1

