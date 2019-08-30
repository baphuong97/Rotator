#!/bin/bash

echo -e "\n==========Welcome to the servo control program==========\n"
while true;
do
    echo -e "Enter the angle (degree C) to rotate (or enter \"q\" to exit): \c"
    read req
    if test $req = q
    then
        exit 0
    fi

    if [[ $req -lt -50 || $req -gt 50 ]]
    then
        echo -e "The entered value is out of range! Please re-enter!\n"
    elif [[ !($req =~ [0-9]) ]]
    then
        echo -e "You have to enter a number! Please re-enter!\n"
    else
        a=$(($req/10 + 15))
        echo $a > /sys/my_gpio_module2/value
    fi
done
