/**************************************************************************
 * 
 * Read temperature from TMP102 chip with I2C and write it to a file and
 * database. 
 *       
 * Copyright (C) 2014 - 2015 Jaakko Koivuniemi.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 *
 * Tue Feb 25 21:28:38 CET 2014
 * Edit: Thu Feb 26 20:13:08 CET 2015
 *
 * Jaakko Koivuniemi
 **/

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

const int version=20150226; // program version
int tempint1=300; // temperature reading interval [s]
int tempint2=0; // second temperature reading interval [s]
int tempint3=0; // third temperature reading interval [s]
int tempint4=0; // fourth temperature reading interval [s]

const char tdatafile1[200]="/var/lib/tmp102d/temperature";
const char tdatafile2[200]="/var/lib/tmp102d/temperature2";
const char tdatafile3[200]="/var/lib/tmp102d/temperature3";
const char tdatafile4[200]="/var/lib/tmp102d/temperature4";

// local SQLite database file
int dbsqlite=0; // store data to local SQLite database
char dbfile[200];

// MySQL database
int dbmysql=0; // store data to MySQL database
char dbhost[200]="";
char dbuser[200]="";
char dbpswd[200]="";
char database[200]="";
int dbport=3306;
#define HOSTNAME_SIZE 63
#define SENSORNAME_SIZE 20

const char i2cdev[100]="/dev/i2c-1";
int address1=0x4a;
int address2=0;
int address3=0;
int address4=0;

const int  i2lockmax=10; // maximum number of times to try lock i2c port  

const char confile[200]="/etc/tmp102d_config";

const char pidfile[200]="/var/run/tmp102d.pid";

int loglev=5; // log level
char message[200]="";

void read_config()
{
  FILE *cfile;
  char *line=NULL;
  char par[20];
  float value;
  unsigned int addr;
  size_t len;
  ssize_t read;

  cfile=fopen(confile, "r");
  if(NULL!=cfile)
  {
    syslog(LOG_INFO|LOG_DAEMON, "Read configuration file");

    while((read=getline(&line,&len,cfile))!=-1)
    {
       if(sscanf(line,"%s %f",par,&value)!=EOF) 
       {
          if(strncmp(par,"LOGLEVEL",8)==0)
          {
             loglev=(int)value;
             sprintf(message,"Log level set to %d",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
             setlogmask(LOG_UPTO (loglev));
          }
          if(strncmp(par,"DBSQLITE",8)==0)
          {
            if(sscanf(line,"%s %s",par,dbfile)!=EOF)  
            {
              dbsqlite=1;
              sprintf(message, "Store data to database %s", dbfile);
              syslog(LOG_INFO|LOG_DAEMON, "%s", message);
            }
          }
          if(strncmp(par,"DBMYSQLHOST",11)==0)
          {
            if(sscanf(line,"%s %s",par,dbhost)!=EOF)  
            {
              dbmysql=1;
              sprintf(message, "MySQL host %s", dbhost);
              syslog(LOG_INFO|LOG_DAEMON, "%s", message);
            }
          }
          if(strncmp(par,"DBMYSQLUSER",11)==0)
          {
            if(sscanf(line,"%s %s",par,dbuser)!=EOF)  
            {
              dbmysql=1;
              sprintf(message, "MySQL user %s", dbuser);
              syslog(LOG_INFO|LOG_DAEMON, "%s", message);
            }
          }
          if(strncmp(par,"DBMYSQLPSWD",11)==0)
          {
            if(sscanf(line,"%s %s",par,dbpswd)!=EOF)  
            {
              dbmysql=1;
              syslog(LOG_INFO|LOG_DAEMON, "MySQL password set");
            }
          }
          if(strncmp(par,"DBMYSQLDB",9)==0)
          {
            if(sscanf(line,"%s %s",par,database)!=EOF)  
            {
              dbmysql=1;
              sprintf(message, "MySQL database %s", database);
              syslog(LOG_INFO|LOG_DAEMON, "%s", message);
            }
          }
          if(strncmp(par,"DBMYSQLPORT",11)==0)
          {
            dbport=(int)value;
            syslog(LOG_INFO|LOG_DAEMON, "MySQL host port %d", dbport);
          }
          if(strncmp(par,"I2CADDR1",8)==0)
          {
             if(sscanf(line,"%s 0x%x",par,&addr)!=EOF)
             { 
                sprintf(message,"First TMP102 chip address set to 0x%x",addr);
                syslog(LOG_INFO|LOG_DAEMON, "%s", message);
                address1=(int)addr;
             }
          }
          if(strncmp(par,"I2CADDR2",8)==0)
          {
             if(sscanf(line,"%s 0x%x",par,&addr)!=EOF)
             { 
                sprintf(message,"Second TMP102 chip address set to 0x%x",addr);
                syslog(LOG_INFO|LOG_DAEMON, "%s", message);
                address2=(int)addr;
             }
          }
          if(strncmp(par,"I2CADDR3",8)==0)
          {
             if(sscanf(line,"%s 0x%x",par,&addr)!=EOF)
             { 
                sprintf(message,"Third TMP102 chip address set to 0x%x",addr);
                syslog(LOG_INFO|LOG_DAEMON, "%s", message);
                address3=(int)addr;
             }
          }
          if(strncmp(par,"I2CADDR4",8)==0)
          {
             if(sscanf(line,"%s 0x%x",par,&addr)!=EOF)
             { 
                sprintf(message,"Fourth TMP102 chip address set to 0x%x",addr);
                syslog(LOG_INFO|LOG_DAEMON, "%s", message);
                address4=(int)addr;
             }
          }
          if(strncmp(par,"TEMPINT1",8)==0)
          {
             tempint1=(int)value;
             sprintf(message,"First temperature reading interval set to %d s",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
          }
          if(strncmp(par,"TEMPINT2",8)==0)
          {
             tempint2=(int)value;
             sprintf(message,"Second temperature reading interval set to %d s",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
          }
          if(strncmp(par,"TEMPINT3",8)==0)
          {
             tempint3=(int)value;
             sprintf(message,"Third temperature reading interval set to %d s",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
          }
          if(strncmp(par,"TEMPINT4",8)==0)
          {
             tempint4=(int)value;
             sprintf(message,"Fourth temperature reading interval set to %d s",(int)value);
             syslog(LOG_INFO|LOG_DAEMON, "%s", message);
          }
       }
    }
    fclose(cfile);
  }
  else
  {
    sprintf(message, "Could not open %s", confile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }
}

int cont=1; /* main loop flag */

// read data with i2c from address, length is the number of bytes to read 
// return: -1=open failed, -2=lock failed, -3=bus access failed, 
// -4=i2c slave reading failed
int read_data(int address, int length)
{
  int rdata=0;
  int fd,rd;
  int cnt=0;
  unsigned char buf[10];

  if((fd=open(i2cdev, O_RDWR)) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to open i2c port");
    return -1;
  }

  rd=flock(fd, LOCK_EX|LOCK_NB);

  cnt=i2lockmax;
  while((rd==1)&&(cnt>0)) // try again if port locking failed
  {
    sleep(1);
    rd=flock(fd, LOCK_EX|LOCK_NB);
    cnt--;
  }
  if(rd)
  {
    syslog(LOG_ERR|LOG_DAEMON, "Failed to lock i2c port");
    close(fd);
    return -2;
  }

  if(ioctl(fd, I2C_SLAVE, address) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "Unable to get bus access to talk to slave");
    close(fd);
    return -3;
  }

  if(length==1)
  {
     if(read(fd, buf,1)!=1) 
     {
       syslog(LOG_ERR|LOG_DAEMON, "Unable to read from slave, exit");
       cont=0;
       close(fd);
       return -4;
     }
     else 
     {
       sprintf(message,"Receive 0x%02x",buf[0]);
       syslog(LOG_DEBUG, "%s", message); 
       rdata=buf[0];
     }
  } 
  else if(length==2)
  {
     if(read(fd, buf,2)!=2) 
     {
       syslog(LOG_ERR|LOG_DAEMON, "Unable to read from slave, exit");
       cont=0;
       close(fd);
       return -4;
     }
     else 
     {
       sprintf(message,"Receive 0x%02x%02x",buf[0],buf[1]);
       syslog(LOG_DEBUG, "%s", message);  
       rdata=256*buf[0]+buf[1];
     }
  }
  else if(length==4)
  {
     if(read(fd, buf,4)!=4) 
     {
       syslog(LOG_ERR|LOG_DAEMON, "Unable to read from slave, exit");
       cont=0;
       close(fd);
       return -4;
     }
     else 
     {
        sprintf(message,"Receive 0x%02x%02x%02x%02x",buf[0],buf[1],buf[2],buf[3]);
        syslog(LOG_DEBUG, "%s", message);  
        rdata=16777216*buf[0]+65536*buf[1]+256*buf[2]+buf[3];
     }
  }

  close(fd);

  return rdata;
}

void write_temp(double t, int addr)
{
  FILE *tfile=NULL;

  if(addr==address1) tfile=fopen(tdatafile1, "w");
  else if(addr==address2) tfile=fopen(tdatafile2, "w"); 
  else if(addr==address3) tfile=fopen(tdatafile3, "w"); 
  else if(addr==address4) tfile=fopen(tdatafile4, "w"); 

  if(NULL==tfile) syslog(LOG_ERR|LOG_DAEMON, "could not write to file");
  else
  { 
    fprintf(tfile,"%2.1f",t);
    fclose(tfile);
  }
}

void insertSQLite(double t, int addr)
{
  sqlite3 *db;
  sqlite3_stmt *stmt; 
  const char query[200]="insert into tmp102 (name,temperature) values (?,?)";  
  int rc;

  rc = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READWRITE, NULL);
  if( rc!=SQLITE_OK ){
    sprintf(message, "Can't open database: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    sqlite3_close(db);
    return;
  }

  rc = sqlite3_prepare_v2(db, query, 200, &stmt, 0);
  if( rc==SQLITE_OK )
  {
    if( stmt!=NULL )
    {
      if(addr==address1) 
        rc = sqlite3_bind_text(stmt, 1, "T1", 2, SQLITE_STATIC);
      else if(addr==address2) 
        rc = sqlite3_bind_text(stmt, 1, "T2", 2, SQLITE_STATIC);
      else if(addr==address3) 
        rc = sqlite3_bind_text(stmt, 1, "T3", 2, SQLITE_STATIC);
      else if(addr==address4) 
        rc = sqlite3_bind_text(stmt, 1, "T4", 2, SQLITE_STATIC);
      if( rc!=SQLITE_OK)
      {
        sprintf(message, "Binding failed: %s", sqlite3_errmsg(db));
        syslog(LOG_ERR|LOG_DAEMON, "%s", message);
      }

      rc = sqlite3_bind_double(stmt, 2, t);
      if( rc!=SQLITE_OK)
      {
        sprintf(message, "Binding failed: %s", sqlite3_errmsg(db));
        syslog(LOG_ERR|LOG_DAEMON, "%s", message);
      }

      rc = sqlite3_step(stmt); 
      if( rc!=SQLITE_DONE )// could be SQLITE_BUSY here 
      {
        sprintf(message, "Statement failed: %s", sqlite3_errmsg(db));
        syslog(LOG_ERR|LOG_DAEMON, "%s", message);
      }
    }
  }
  else
  {
    sprintf(message, "Statement prepare failed: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(db);

  return;
}

int readSQLiteTime()
{
  sqlite3 *db;
  sqlite3_stmt *stmt; 
  const char query[200]="select datetime()";  
  int rc;
  int ok=0;

  rc = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READONLY, NULL);
  if( rc!=SQLITE_OK ){
    sprintf(message, "Can't open database: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    sqlite3_close(db);
    return 0;
  }

  rc = sqlite3_prepare_v2(db, query, 200, &stmt, 0);
  if( rc==SQLITE_OK )
  {
    if( stmt!=NULL )
    {
      rc = sqlite3_step(stmt); 
      if( rc!=SQLITE_ROW )// could be SQLITE_BUSY here 
      {
        sprintf(message, "Statement failed: %s", sqlite3_errmsg(db));
        syslog(LOG_ERR|LOG_DAEMON, "%s", message);
        ok=0;
      }
      else
      {
        sprintf(message, "SQLite time: %s", sqlite3_column_text(stmt, 0));
        syslog(LOG_INFO|LOG_DAEMON, "%s", message);
        ok=1;
      }
    }
    else ok=0;
  }
  else
  {
    sprintf(message, "Statement prepare failed: %s", sqlite3_errmsg(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    ok=0;
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(db);

  return ok;
}


int readMySQLTime()
{
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

int insertMySQL(double t, int addr)
{
  char name[SENSORNAME_SIZE]="";
  char myhost[HOSTNAME_SIZE]="";
  float temperature=(float)t;

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

  MYSQL_STMT *stmt = mysql_stmt_init(db);

  if( stmt==NULL )
  {
    sprintf(message, "MySQL statement handler creation failed %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_close(db);
    return 0;
  }

  char instmt[200]="insert into tmp102 values(default,default,?,?)";
  if( mysql_stmt_prepare(stmt, instmt, strlen(instmt))!=0 )
  {
    sprintf(message, "MySQL statement prepare failed: %s", mysql_error(db));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    mysql_stmt_close(stmt);
    mysql_close(db);
    return 0;
  } 

  size_t len=HOSTNAME_SIZE;
  if( gethostname(myhost, len)!=0 )
  {
    sprintf(message, "gethostname failed: %s", strerror(errno));
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
  }

  if(addr==address1) snprintf(name, SENSORNAME_SIZE, "T1@%s", myhost); 
  else if(addr==address2) snprintf(name, SENSORNAME_SIZE, "T2@%s", myhost); 
  else if(addr==address3) snprintf(name, SENSORNAME_SIZE, "T3@%s", myhost);
  else if(addr==address4) snprintf(name, SENSORNAME_SIZE, "T4@%s", myhost);

  MYSQL_BIND bind[2];
  memset(bind, 0, sizeof(bind));
  bind[0].buffer_type= MYSQL_TYPE_STRING;
  bind[0].buffer= (char *)name;
  bind[0].buffer_length= strlen(name);
  bind[0].is_null= 0;
  bind[0].length= 0;

  bind[1].buffer_type= MYSQL_TYPE_FLOAT;
  bind[1].buffer= (char *)&temperature;
  bind[1].is_null= 0;
  bind[1].length= 0;

// error handling here
  mysql_stmt_bind_param(stmt, bind);
// error handling here
  mysql_stmt_execute(stmt);

  mysql_stmt_close(stmt);
  mysql_close(db);

  return 1;
}



void read_temp(int addr)
{
  short value=0;
  double temp=0;

  value=(short)(read_data(addr,2));
  value/=16;
  temp=(double)(value*0.0625);

  if(cont==1)
  {
    if(addr==address1) sprintf(message,"T1=%f C",temp);
    else if(addr==address2) sprintf(message,"T2=%f C",temp);
    else if(addr==address3) sprintf(message,"T3=%f C",temp);
    else if(addr==address4) sprintf(message,"T4=%f C",temp);
    syslog(LOG_INFO|LOG_DAEMON, "%s", message);
    write_temp(temp,addr);
    if(dbsqlite==1) insertSQLite(temp,addr);
    if(dbmysql==1) insertMySQL(temp,addr);
  }
}

void stop(int sig)
{
  sprintf(message, "signal %d catched, stop", sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);
  cont=0;
}

void terminate(int sig)
{
  sprintf(message, "signal %d catched", sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);

  sleep(1);
  syslog(LOG_NOTICE|LOG_DAEMON, "stop");

  cont=0;
}

void hup(int sig)
{
  sprintf(message,"signal %d catched",sig);
  syslog(LOG_NOTICE|LOG_DAEMON, "%s", message);
}


int main()
{  
  int ok=0;

  setlogmask(LOG_UPTO (loglev));
  syslog(LOG_NOTICE|LOG_DAEMON, "tmp102d v. %d started",version); 

  signal(SIGINT,&stop); 
  signal(SIGKILL,&stop); 
  signal(SIGTERM,&terminate); 
  signal(SIGQUIT,&stop); 
  signal(SIGHUP,&hup); 

  read_config();

  pid_t pid, sid;
        
  pid=fork();
  if(pid<0) 
  {
    exit(EXIT_FAILURE);
  }

  if(pid>0) 
  {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  /* Create a new SID for the child process */
  sid=setsid();
  if(sid<0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "failed to create child process"); 
    exit(EXIT_FAILURE);
  }
        
  if((chdir("/")) < 0) 
  {
    syslog(LOG_ERR|LOG_DAEMON, "failed to change to root directory"); 
    exit(EXIT_FAILURE);
  }
        
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  FILE *pidf;
  pidf=fopen(pidfile,"w");

  if(pidf==NULL)
  {
    sprintf(message,"Could not open PID lock file %s, exiting", pidfile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    exit(EXIT_FAILURE);
  }

  if(flock(fileno(pidf),LOCK_EX||LOCK_NB)==-1)
  {
    sprintf(message,"Could not lock PID lock file %s, exiting", pidfile);
    syslog(LOG_ERR|LOG_DAEMON, "%s", message);
    exit(EXIT_FAILURE);
  }

  fprintf(pidf,"%d\n",getpid());
  fclose(pidf);

  if(dbsqlite==1)
  {
    if(readSQLiteTime()==0) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "SQLite database read failed, drop database connection");
      dbsqlite=0; 
    }
  }

  if(dbmysql==1)
  {
    if(readMySQLTime()==0) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "MySQL database read failed, drop database connection");
      dbmysql=0; 
    }
  }

  int unxs=(int)time(NULL); // unix seconds
  int nxtemp1=unxs; // next time to read temperature
  int nxtemp2=unxs; // next time to read second temperature
  int nxtemp3=unxs; // next time to read third temperature
  int nxtemp4=unxs; // next time to read fourth temperature

  while(cont==1)
  {
    unxs=(int)time(NULL); 

    if(((unxs>=nxtemp1)||((nxtemp1-unxs)>tempint1))&&(tempint1>0)&&(address1>=0x48)&&(address1<=0x4B)) 
    {
      nxtemp1=tempint1+unxs;
      read_temp(address1);
    }

    if(((unxs>=nxtemp2)||((nxtemp2-unxs)>tempint2))&&(tempint2>0)&&(address2>=0x48)&&(address2<=0x4B)) 
    {
      nxtemp2=tempint2+unxs;
      read_temp(address2);
    }

    if(((unxs>=nxtemp3)||((nxtemp3-unxs)>tempint3))&&(tempint3>0)&&(address3>=0x48)&&(address3<=0x4B)) 
    {
      nxtemp3=tempint3+unxs;
      read_temp(address3);
    }

    if(((unxs>=nxtemp4)||((nxtemp4-unxs)>tempint4))&&(tempint4>0)&&(address4>=0x48)&&(address4<=0x4B)) 
    {
      nxtemp4=tempint4+unxs;
      read_temp(address4);
    }

    sleep(1);
  }

  syslog(LOG_NOTICE|LOG_DAEMON, "remove PID file");
  ok=remove(pidfile);

  return ok;
}
