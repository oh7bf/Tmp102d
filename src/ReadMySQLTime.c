#include "tmp102d.h"

// read time from MySQL database
int ReadMySQLTime(dbhost, dbuser, dbpswd, database, dbport)
char dbhost[ DBHOSTNAME_SIZE ]; 
char dbuser[ DBUSER_SIZE ];
char dbpswd[ DBPSWD_SIZE ]; 
char database[ DBNAME_SIZE ];
int dbport;
{
  char message[200]="";

  MYSQL *db = mysql_init(NULL);

  if( db==NULL )
  {
    sprintf(message, "MySQL handle creation failed: %s",  mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    return 0;
  }

  if( mysql_real_connect(db, dbhost, dbuser, dbpswd, database, dbport, NULL, 0)==NULL )
  {
      sprintf(message, "MySQL connection failed: %s", mysql_error(db));
      syslog(LOG_ERR|LOG_DAEMON, "%s", message);
      mysql_close(db);
      return 0;
  }

  if( mysql_query(db, "select now()") )
  {
    sprintf(message, "MySQL statement failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_close(db);
    return 0;
  }

  MYSQL_RES *result = mysql_store_result(db);

  if( result==NULL )
  {
    sprintf(message, "MySQL time could not be read: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_close(db);
    return 0;
  }

  MYSQL_ROW row;
  if( mysql_num_fields(result)==1 )
  {
    row=mysql_fetch_row(result);
    if( row )
    {
      sprintf(message, "MySQL time: %s", row[0]);
      syslog(LOG_INFO|LOG_DAEMON, "%s", message);
    }
    else
    {
      syslog(LOG_ERR|LOG_DAEMON, "MySQL time reading failed");
      mysql_close(db);
      return 0;
    }
  }
  else
  {
    syslog(LOG_ERR|LOG_DAEMON, "MySQL time reading failed");
    mysql_close(db);
    return 0;
  }

  mysql_close(db);
  return 1;
}


