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
 * Edit: Mon May 25 22:59:38 CEST 2015
 *
 * Jaakko Koivuniemi
 **/

#include "tmp102d.h"
#include "InsertSQLite.h"
#include "ReadSQLiteTime.h"
#include "InsertMySQL.h"
#include "ReadMySQLTime.h"
#include "I2cReadBytes.h"


const int version=20150525; // program version
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
char dbfile[ SQLITEFILENAME_SIZE ];

// MySQL database
int dbmysql=0; // store data to MySQL database
char dbhost[ DBHOSTNAME_SIZE ]="";
char dbuser[ DBUSER_SIZE ]="";
char dbpswd[ DBPSWD_SIZE ]="";
char database[ DBNAME_SIZE ]="";
int dbport=3306;

const char *i2cdev="/dev/i2c-1";
int address1=0x4a;
int address2=0;
int address3=0;
int address4=0;

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

void InsSQLite(const char dbfile[ SQLITEFILENAME_SIZE ], double t, int addr)
{
  const char query[ SQLITEQUERY_SIZE ] = "insert into tmp102 (name,temperature) values (?,?)";  
  double data[ SQLITE_DOUBLES ];

  data[0] = t;
  if( addr == TMP102_ADDRESS1 )
  { 
    InsertSQLite(dbfile, query, "T1", 1, data);
  }
  else if( addr == TMP102_ADDRESS2 )
  { 
    InsertSQLite(dbfile, query, "T2", 1, data);
  }
  else if( addr == TMP102_ADDRESS3 )
  { 
    InsertSQLite(dbfile, query, "T3", 1, data);
  }
  else if( addr == TMP102_ADDRESS4 )
  { 
    InsertSQLite(dbfile, query, "T4", 1, data);
  }

  return;
}

void InsMySQL(double t, int addr)
{
  const char query[ SQLITEQUERY_SIZE ] = "insert into tmp102 values(default,default,?,?)";
  double data[ SQLITE_DOUBLES ];
  data[0] = t;

  if( addr == TMP102_ADDRESS1 )
    InsertMySQL(dbhost, dbuser, dbpswd, database, dbport, query, "T1", 1, data);
  else if( addr == TMP102_ADDRESS2 )
    InsertMySQL(dbhost, dbuser, dbpswd, database, dbport, query, "T2", 1, data);
  else if( addr == TMP102_ADDRESS3 )
    InsertMySQL(dbhost, dbuser, dbpswd, database, dbport, query, "T3", 1, data);
  else if( addr == TMP102_ADDRESS4 )
    InsertMySQL(dbhost, dbuser, dbpswd, database, dbport, query, "T4", 1, data);
}

void read_temp(int addr)
{
  short value=0;
  double temp=0;

  value=(short)(I2cReadBytes(addr, 2));
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
    if(dbsqlite==1) InsSQLite(dbfile, temp, addr);
    if(dbmysql==1) InsMySQL(temp, addr);
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

  if( dbsqlite == 1 )
  {
    if( ReadSQLiteTime( dbfile ) == 0 ) 
    {
      syslog(LOG_ERR|LOG_DAEMON, "SQLite database read failed, drop database connection");
      dbsqlite=0; 
    }
  }

  if( dbmysql == 1 )
  {
    if( ReadMySQLTime(dbhost, dbuser, dbpswd, database, dbport) == 0 ) 
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
