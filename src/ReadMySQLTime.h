#ifndef READMYSQLTIME_H_INCLUDED
#define READMYSQLTIME_H_INCLUDED
int ReadMySQLTime(
char dbhost[ DBHOSTNAME_SIZE ],
char dbuser[ DBUSER_SIZE ],
char dbpswd[ DBPSWD_SIZE ],
char database[ DBNAME_SIZE ],
int dbport);
#endif

