#include "tmp102d.h"

int InsertMySQL(dbhost, dbuser, dbpswd, database, dbport, query, name, n, data)
char dbhost[ DBHOSTNAME_SIZE ]; // MySQL database hostname 
char dbuser[ DBUSER_SIZE ]; // MySQL user
char dbpswd[ DBPSWD_SIZE ]; // MySQL password
char database[ DBNAME_SIZE ]; // MySQL database name
int dbport; // MySQL database port
const char query[ SQLITEQUERY_SIZE ]; // statement string
const char name[ SENSORNAME_SIZE ]; // sensor name or other tag
int n;                            // number of doubles to insert
double data[ SQLITE_DOUBLES ];      // array of doubles
{
  char longname[ SENSORNAME_SIZE + HOSTNAME_SIZE ]="";
  char myhost[ HOSTNAME_SIZE ]="";
  float value=0;
  char message[200]="";

  MYSQL *db = mysql_init(NULL);

  if( db==NULL )
  {
    sprintf(message, "MySQL handle creation failed: %s",  mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    return 0;
  }

  if( mysql_real_connect(db, dbhost, dbuser, dbpswd, database, dbport, NULL, 0) == NULL ) 
  {
    sprintf(message, "MySQL connection failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_close(db);
    return 0;
  }  

  MYSQL_STMT *stmt = mysql_stmt_init(db);

  if( stmt == NULL )
  {
    sprintf(message, "MySQL statement handler creation failed %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_close(db);
    return 0;
  }

//  char instmt[200]="insert into tmp102 values(default,default,?,?)";
  if( mysql_stmt_prepare(stmt, query, strlen(query)) != 0 )
  {
    sprintf(message, "MySQL statement prepare failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_stmt_close(stmt);
    mysql_close(db);
    return 0;
  } 

  size_t len = HOSTNAME_SIZE;
  if( gethostname(myhost, len) != 0 )
  {
    sprintf(message, "gethostname failed: %s", strerror(errno));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  strcpy(longname, name);
  strcat(longname, "@");
  strcat(longname, myhost);

  MYSQL_BIND bind[n];
  memset(bind, 0, sizeof(bind));

  bind[0].buffer_type= MYSQL_TYPE_STRING;
  bind[0].buffer = (char *)longname;
  bind[0].buffer_length = strlen(longname);
  bind[0].is_null = 0;
  bind[0].length = 0;

  int i=0;
  for(i=1;i<n+1;i++)
  {
    bind[i].buffer_type= MYSQL_TYPE_FLOAT;
    value = (float)data[i-1];
    bind[i].buffer = (char *)&value;
    bind[i].is_null = 0;
    bind[i].length = 0;
  }

  if( mysql_stmt_bind_param(stmt, bind) != 0)
  {
    sprintf(message, "MySQL statement bind failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  if( mysql_stmt_execute(stmt) != 0)
  {
    sprintf(message, "MySQL statement execution failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  mysql_stmt_close(stmt);
  mysql_close(db);

  return 1;
}


