#!/bin/sh

temperature=0
name="indoor"

read temperature < /var/lib/tmp102d/temperature

/usr/bin/sqlite3 /var/lib/tmp102d/tmp102d.db "insert into tmp102 \
(name,temperature) values (\"$name\",\"$temperature\");"

