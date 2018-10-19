#! /bin/bash

_HOME2_=$(dirname $0)
export _HOME2_
_HOME_=$(cd $_HOME2_;pwd)
export _HOME_

cd $_HOME_

runtime_min=1

# 0 Red blink
# 15 all green
# 16 rainbow colors

bar_steps=16
bar_current_value=15

calc_sleep_secs_per_step=$(printf "$runtime_min *  60 / $bar_steps\n"|bc)

echo "sleep per step = $calc_sleep_secs_per_step""s"

while [ $bar_current_value -ge 0 ]; do
    echo "val=$bar_current_value"
    ./set_bar.sh "$bar_current_value"
    bar_current_value=$[ $bar_current_value - 1 ]
    sleep $calc_sleep_secs_per_step
done
