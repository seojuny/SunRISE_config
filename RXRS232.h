/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: rxrs232.cpp                                            */
/*                                                                       */
/*  Revision: V1.0     Date: 17.06.2003    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Receiver control header Radiospectrometer CALLISTO          */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 17.06.2003

//---------------------------------------------------------------------------
#ifndef RXRS232H
#define RXRS232H
//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
// Attention: think about lenght of buffer and type of pointer!
#define RXbuflen 4000
//---------------------------------------------------------------------------
class TWrite : public TThread
{
private:
protected:
	void __fastcall Execute();
public:
    bool __fastcall Init(void);
    __fastcall TWrite(bool CreateSuspended);
    void __fastcall Parametrisation(void); // startup
    void __fastcall GetRXRS232Data(void); // Read data from RS buffer
    void __fastcall Transmit(char byte); // Send byte to client via RS
    void __fastcall Send(char * cmd); // Send string to client via RS
    void __fastcall Close(void); // termination

};
//---------------------------------------------------------------------------
#endif
