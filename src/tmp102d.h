#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <sqlite3.h>
#include <my_global.h>
#include <mysql.h>
#include <limits.h>

#ifndef TMP102D_H_INCLUDED
#define TMP102D_H_INCLUDED

#define I2LOCK_MAX 10
#define TMP102_ADDRESS1 0x48
#define TMP102_ADDRESS2 0x49
#define TMP102_ADDRESS3 0x4A
#define TMP102_ADDRESS4 0x4B
#define TMP102_TEMP_REG 0x00
#define TMP102_CONFIG_REG 0x01
#define TMP102_TEMP_LOW_REG 0x02
#define TMP102_TEMP_HIGH_REG 0x03

#define HOSTNAME_SIZE 63
#define SENSORNAME_SIZE 20
#define SQLITEFILENAME_SIZE 200
#define SQLITEQUERY_SIZE 200
#define SQLITE_DOUBLES 10 
#define DBHOSTNAME_SIZE 200
#define DBUSER_SIZE 200
#define DBPSWD_SIZE 200
#define DBNAME_SIZE 200
#define MYSQLQUERY_SIZE 200
#define MYSQL_DOUBLES 10

extern const char *i2cdev;// i2c device
extern int loglev; // log level
extern int cont; // main loop flag

#endif

