#ifndef INSERTMYSQL_H_INCLUDED
#define INSERTMYSQL_H_INCLUDED
int InsertMySQL(
char dbhost[ DBHOSTNAME_SIZE ],
char dbuser[ DBUSER_SIZE ],
char dbpswd[ DBPSWD_SIZE ],
char database[ DBNAME_SIZE ],
int dbport,
const char query[ MYSQLQUERY_SIZE ],
const char name[ SENSORNAME_SIZE ],
int n,
double data[ MYSQL_DOUBLES ]
);
#endif
