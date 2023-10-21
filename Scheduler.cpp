/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: Scheduler.cpp                                          */
/*                                                                       */
/*  Revision: V1.1     Date: 06.12.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Scheduler Radiospectrometer CALLISTO                        */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 01.12.2003
// Updated by: Chr. Monstein 03.12.2003
// Updated by: Chr. Monstein 26.11.2004 integration parameter deleted
// Updated by: Chr. Monstein 05.12.2004 ScrollBar replaced by TrackBar
// Updated by: Chr. Monstein 06.12.2004 FCx and FCy linked together
// Updated by: Chr. Monstein 11.06.2009 new function '8' Radio monitoring
// Updated by: Chr. Monstein 30.12.2009 update scheduler with fprog
// Updated by: Chr. Monstein 06.06.2010 repair scheduler with fprog
// Updated by: Chr. Monstein 25.10.2017 update OVS with FCx
//---------------------------------------------------------------------------

#pragma hdrstop

#include <stdio.h>
#include <fstream.h>

#include "Scheduler.h"
#include "mainunit.h"
#include "EEPROM.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
// import from EEPROM.cpp
extern EEPROM *UploadThread;
//---------------------------------------------------------------------------

extern char frqfilename[80];     // import from mainunit.cpp
extern char frqpath[120];        // import from mainunit.cpp
extern int measurestate;         // import from mainunit.cpp
extern bool transfer_once;       // import from mainunit.cpp
extern bool overview;            // import from mainunit.cpp
extern bool eepromuploaded;      // import from mainunit.cpp
extern char mmode;               // import from mainunit.cpp
extern int FCx;                  // import from mainunit.cpp
extern int timerpreread;         // import from mainunit.cpp
extern AnsiString DatumSt;       // import from mainunit.cpp
extern AnsiString TimerSt;       // import from mainunit.cpp
extern Word Year, Month, Day, Hour, Mint, Sect, MSec;  // import from mainunit.cpp, global timing
extern float Tact;               // import from mainunit.cpp
//---------------------------------------------------------------------------

const int Max_schedul_entries = 150; // maximum ammount of scheduler entries allowed
Tscheduler Schedule[Max_schedul_entries];
int Schedule_entries = 0; // number of read (actual) scheduler entries
bool scheduler_once=true;

//---------------------------------------------------------------------------
void __fastcall EvalScheduler(void) // what has when to be done?
{
    long ActTime, SchTime;
    int i = 0;
    char st[40];
    static Byte startstate=0;
    static bool first = true;
    ActTime = (long)(3600*Tact); // time now in seconds to compare with scheduler entries
    do
    {
        SchTime = Schedule[i].hh*3600 + Schedule[i].mm*60 + Schedule[i].ss;
        if (SchTime < ActTime)
            i++;
        else
        {
            // interesting entry found, schedule time >= actual time
            sprintf(st,"Next sched.: %02u:%02u:%02u",Schedule[i].hh,Schedule[i].mm,Schedule[i].ss);
            MainForm->Label20->Caption=st;

            if (((SchTime-timerpreread)==ActTime) && scheduler_once) // some spare (5sec) to stop actual measurement
            {
                scheduler_once = false;
                MainForm->StopButtonClick(0);
            }
            else
            {
              if ((SchTime==ActTime) && (transfer_once))// just time to start new measurement
              {
                  mmode = UpCase(Schedule[i].mmode);
                  switch(mmode) // which measurement mode is active?
                  {
                    case '0': // empty-mode or simulation-mode or idle, just wait
                    {
                       break;
                    }
                    case '8': // Radio-monitoring via spectral overview
                    {
                       if ((measurestate==1) && (!overview)) // only in stop and non-recursive
                       {
                          MainForm->Logging("$HST:Radio monitoring");
                          FCx = Schedule[i].FCx;   // required for auto calibration of OVS-files
                          MainForm->OverviewClick(0);
                       }
                       break;
                    }
                    default: // all other modes 1,2,3,4,5,6,7,-,9,A...Z -> do measurements
                    {
                        switch (startstate) // which will be too late .....(eeprom upload)
                        {
                            case 0:
                            {
                                MainForm->Logging("$HST:Autostart via scheduler");
                                FCx = Schedule[i].FCx;
                                if (strstr(StrUpper(Schedule[i].fprog),"FRQ"))
                                {
                                    sprintf(frqfilename,"%s",Schedule[i].fprog); // save filename into global variable
                                    sprintf(frqpath    ,"%s",""); // just local path
                                    MainForm->Logging("$HST:Automatic frequencyfile sel.");
                                    UploadThread->Upload(true);
                                    if ((eepromuploaded))
                                    {
                                        MainForm->StartButtonClick(0);
                                        startstate++;
                                    }
                                } else
                                {
                                   MainForm->StartButtonClick(0);
                                   startstate++;
                                }
                                break;
                            }
                            case 1:
                            {
                                if (measurestate!=2) // assure start has been triggered
                                    startstate=0;
                                break;
                            }
                        } // end switch (startstate)
                        break; // action has been taken, quit do-loop
                     } // end antenna calibration
                  } // end switch(scheduler)
              } // end if(timer==..)
            }
            break; // because probably an action has been taken
        }
        if (i >= Schedule_entries)
        {
            sprintf(st,"Next sched.: no entries");
            MainForm->Label20->Caption=st;
            if (first)
            {
                MainForm->Logging("$HST:No more entries in scheduler.");
                first = false;
            }
            break; // no more entries
        }
    } while(1);
}
//---------------------------------------------------------------------------
void __fastcall ReadScheduler(void) // what has when to be done?
{
  char buff[120];
  AnsiString st;
  int i = 0; // counts scheduler entries
  int c; // checks scheduler entry
  static first = true;

  ifstream infile("scheduler.cfg");
  while (infile)
  {
      infile.getline(buff,120);
      st = LowerCase(AnsiString(buff));
      if ((buff[0]>='0') && (buff[0]<='9') )
      {
        sprintf(Schedule[i].fprog,"nil");
        c = sscanf(buff,"%02u:%02u:%02u,%02u,%1c,%s",
                       &Schedule[i].hh,&Schedule[i].mm,&Schedule[i].ss,
                       &Schedule[i].FCx,
                       &Schedule[i].mmode,
                       &Schedule[i].fprog);
        if ((c==5) || (c==6)) // complete structure?
          i++;
      }
      if (i >= Max_schedul_entries) break;
  }
  infile.close();
  Schedule_entries=i;

  if (first)
  {
    if (i < 1)
       MainForm->Logging("$HST:File scheduler.cfg not found or no entries");
    else
       MainForm->Logging("$HST:File scheduler.cfg read");
    first = false;
  }
}
//---------------------------------------------------------------------------
