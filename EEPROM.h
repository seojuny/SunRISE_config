/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: EEPROM.h                                               */
/*                                                                       */
/*  Revision: V1.1     Date: 01.12.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: EEPROM upload thread Radiospectrometer CALLISTO             */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 27.11.2003
// Updated by: Chr. Monstein 01.12.2004 structure reduced to OLTP
// Updated by: Chr. Monstein 09.06.2009 new structure for calibration
// Updated by: Chr. Monstein 01.01.2010  more (5) light curves possible
// Updated by: Chr. Monstein 05.09.2017  more (now 10) light curves possible
//---------------------------------------------------------------------------

#ifndef EEPROMH
#define EEPROMH

#define MaxNolc 10

//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
class EEPROM : public TThread
{
private:
protected:
    void __fastcall Execute();
public:
    __fastcall EEPROM(bool CreateSuspended);
    void __fastcall Init();
    void __fastcall Upload(bool upload);
    void __fastcall Close();
    void __fastcall StateMachine();
    void __fastcall ReadCalibrationFile(); // lese IDL parameter
};

typedef struct
{
    int poltp;
    float foltp;
} TTP;

typedef struct
{
    int channel;
    float f_MHz;
    float b;
    float a;
    float cf;
    float Tb;
} Tcal;

typedef struct
{
   int channel;
   int count;
   float MHz;
} Tolc;
//---------------------------------------------------------------------------
#endif
