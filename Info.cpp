/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: Info.cpp                                               */
/*                                                                       */
/*  Revision: V1.1     Date: 08.04.2008    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Info panel for Radiospectrometer CALLISTO                   */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 29.11.2004
// Updated by: Chr. Monstein 30.11.2004  header included
// Updated by: Chr. Monstein 25.04.2006 Version included
// Updated by: Chr. Monstein 08.04.2008 new version
// Updated by: Chr. Monstein 11.06.2009 new disk space, califile, FC and mmode
// Updated by: Chr. Monstein 05.08.2009 new on-line light curves
// Updated by: Chr. Monstein 30.12.2009 hide plots, scheduler with new frq
// Updated by: Chr. Monstein 31.08.2012 repair bug in light curve calibration
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Info.h"
#include "mainunit.h"
#include <stdio.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm7 *Form7;
extern char version[40];           // import from mainunit.cpp
extern char author[80];            // import from mainunit.cpp
extern char datapath[120];         // import from mainunit.cpp
//---------------------------------------------------------------------------
__fastcall TForm7::TForm7(TComponent* Owner)
    : TForm(Owner)
{
    Form7->Left = MainForm->Left;
    Form7->Top  = MainForm->Top + MainForm->Height + 10;
    char drive = UpCase(datapath[0]);
    int n = drive - 65 + 1;

    __int64 AmtFree = DiskFree(n);  // (0 = aktuelles Laufwerk, 1 = A, 2 = B usw.).
    __int64 Total   = DiskSize(n);  // (0 = aktuelles Laufwerk, 1 = A, 2 = B usw.).
    AnsiString S;
    S.sprintf("%6.3f percent of the space on drive %c: is free: %I64d KB", (float)AmtFree*100.0/(float)Total, drive, AmtFree/1024 );
    Label15->Caption = S; // See also FitsWrite

    Label7->Caption = version;
    Label9->Caption = author;
}
//---------------------------------------------------------------------------
