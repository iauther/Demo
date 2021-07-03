//
//  wsc.h (version 7.0.0)
//
//  Used for Win64 and Win32 compilers supporting the "declspec" keyword.
//

#ifdef STATIC_LIBRARY
  #define DLL_IMPORT_EXPORT
#else
  #ifdef DLL_SOURCE_CODE
     #define DLL_IMPORT_EXPORT __declspec(dllexport) __stdcall
  #else
     #define DLL_IMPORT_EXPORT __declspec(dllimport) __stdcall
  #endif
#endif

#ifdef __cplusplus
  #define NoMangle extern "C"
#else
  #define NoMangle
#endif

NoMangle int DLL_IMPORT_EXPORT    SioBaud(int,UINT);
NoMangle int DLL_IMPORT_EXPORT    SioBrkSig(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioByteToShort(char *Ptr);
NoMangle int DLL_IMPORT_EXPORT    SioClose(int);
NoMangle int DLL_IMPORT_EXPORT    SioCRC16(BYTE *, int);
NoMangle UINT DLL_IMPORT_EXPORT   SioCRC32(BYTE *, int);
NoMangle int DLL_IMPORT_EXPORT    SioCTS(int);
NoMangle int DLL_IMPORT_EXPORT    SioDCD(int);
NoMangle int DLL_IMPORT_EXPORT    SioDebug(int);
NoMangle int DLL_IMPORT_EXPORT    SioDone(int);
NoMangle int DLL_IMPORT_EXPORT    SioDSR(int);
NoMangle int DLL_IMPORT_EXPORT    SioDTR(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioErrorText(int, char *, int);
NoMangle int DLL_IMPORT_EXPORT    SioEvent(int, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioEventChar(int, char , UINT);
NoMangle int DLL_IMPORT_EXPORT    SioEventWait(int, UINT, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioFlow(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioGetc(int);
NoMangle int DLL_IMPORT_EXPORT    SioGetReg(char *,int);
NoMangle int DLL_IMPORT_EXPORT    SioGets(int,char *,UINT);
NoMangle int DLL_IMPORT_EXPORT    SioGetsC(int, char *, int, UINT, char);
NoMangle int DLL_IMPORT_EXPORT    SioGetsQ(int,int,char *,int);
NoMangle int DLL_IMPORT_EXPORT    SioHexView(char *,int,char *,int);
NoMangle int DLL_IMPORT_EXPORT    SioInfo(char);
NoMangle int DLL_IMPORT_EXPORT    SioKeyCode(UINT);
NoMangle int DLL_IMPORT_EXPORT    SioLRC(char *, int);
NoMangle int DLL_IMPORT_EXPORT    SioMessage(int, HWND, int, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioOpen(int);
NoMangle int DLL_IMPORT_EXPORT    SioParms(int,int,int,int);
NoMangle int DLL_IMPORT_EXPORT    SioPortInfo(int, char);
NoMangle int DLL_IMPORT_EXPORT    SioPutc(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioPuts(int,char *,UINT);
NoMangle int DLL_IMPORT_EXPORT    SioQuiet(int, UINT, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioRead(int,int);
NoMangle int DLL_IMPORT_EXPORT    SioReset(int,int,int);
NoMangle int DLL_IMPORT_EXPORT    SioRI(int);
NoMangle int DLL_IMPORT_EXPORT    SioRTS(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioRxClear(int);
NoMangle int DLL_IMPORT_EXPORT    SioRxQue(int);
NoMangle int DLL_IMPORT_EXPORT    SioRxWait(int, UINT, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioSetInteger(int, int, int);
NoMangle int DLL_IMPORT_EXPORT    SioSetTimeouts(int,UINT,UINT,UINT,UINT,UINT);
NoMangle int DLL_IMPORT_EXPORT    SioShortToByte(char *Ptr);
NoMangle int DLL_IMPORT_EXPORT    SioSleep(int);
NoMangle int DLL_IMPORT_EXPORT    SioStatus(int,UINT);
NoMangle UINT DLL_IMPORT_EXPORT   SioTimeMark(UINT);
NoMangle UINT DLL_IMPORT_EXPORT   SioTimer(void);
NoMangle int DLL_IMPORT_EXPORT    SioTxClear(int);
NoMangle int DLL_IMPORT_EXPORT    SioTxQue(int);
NoMangle int DLL_IMPORT_EXPORT    SioUnGetc(int,char);
NoMangle int DLL_IMPORT_EXPORT    SioWaitFor(int, UINT);
NoMangle int DLL_IMPORT_EXPORT    SioWinError(char *,int);

#define COM1   0
#define COM2   1
#define COM3   2
#define COM4   3
#define COM5   4
#define COM6   5
#define COM7   6
#define COM8   7
#define COM9   8
#define COM10  9
#define COM11 10
#define COM12 11
#define COM13 12
#define COM14 13
#define COM15 14
#define COM16 15
// COM17 thru COM256 are defined in the same pattern as above
#define COM256 255

// Parity Codes

#define WSC_NoParity 0
#define WSC_OddParity  1
#define WSC_EvenParity 2
#define WSC_MarkParity 3
#define WSC_SpaceParity 4

// Stop Bit Codes

#define WSC_OneStopBit   0
#define WSC_One5StopBits 1
#define WSC_TwoStopBits  2

// Word Length Codes

#define WSC_WordLength5  5
#define WSC_WordLength6  6
#define WSC_WordLength7  7
#define WSC_WordLength8  8

// Data Bit Codes (same as word length)
#define WSC_DataBits_5   5
#define WSC_DataBits_6   6
#define WSC_DataBits_7   7
#define WSC_DataBits_8   8

// return codes

#define WSC_BUSY      (-100)
#define WSC_NO_DATA   (-100)
#define WSC_RANGE     (-101)
#define WSC_ABORTED   (-102)
#define WSC_WIN32ERR  (-103)
#define WSC_EXPIRED   (-104)
#define WSC_BUFFERS   (-105)
#define WSC_THREAD    (-106)
#define WSC_TIMEOUT   (-107)
#define WSC_KEYCODE   (-108)

#define WSC_BUFFER_RANGE (-109)
#define WSC_BUFLEN_RANGE (-110)
#define WSC_BAD_CMD      (-111)
#define WSC_BAD_PARITY   (-112)
#define WSC_BAD_STOPBIT  (-113)
#define WSC_BAD_WORDLEN  (-114)

#define WSC_IE_BADID     (-1)
#define WSC_IE_OPEN      (-2)
#define WSC_IE_NOPEN     (-3)
#define WSC_IE_MEMORY    (-4)
#define WSC_IE_DEFAULT   (-5)
#define WSC_IE_HARDWARE  (-10)
#define WSC_IE_BYTESIZE  (-11)
#define WSC_IE_BAUDRATE  (-12)
#define WSC_IO_ERROR     (-13)

// baud codes

#define WSC_Baud110    0
#define WSC_Baud300    1
#define WSC_Baud1200   2
#define WSC_Baud2400   3
#define WSC_Baud4800   4
#define WSC_Baud9600   5
#define WSC_Baud19200  6
#define WSC_Baud38400  7
#define WSC_Baud57600  8
#define WSC_Baud115200 9

// SioGetError masks

#define WSC_RXOVER   0x0001
#define WSC_OVERRUN  0x0002
#define WSC_PARITY   0x0004
#define WSC_FRAME    0x0008
#define WSC_BREAK    0x0010
#define WSC_TXFULL   0x0100

// SioDTR and SioRTS Commands

#define WSC_SET_LINE     'S'
#define WSC_CLEAR_LINE   'C'
#define WSC_READ_LINE    'R'

// SioBrkSig Commands

#define WSC_ASSERT_BREAK 'A'
#define WSC_CANCEL_BREAK 'C'
#define WSC_DETECT_BREAK 'D'

// SioFlow Commands

#define WSC_HARDWARE_FLOW_CONTROL 'H'
#define WSC_SOFTWARE_FLOW_CONTROL 'S'
#define WSC_NO_FLOW_CONTROL       'N'

// SioInfo commands

#define WSC_GET_VERSION  'V'
#define WSC_GET_BUILD    'B'
#define WSC_IS_32BITS    '3'

// SioSetInteger commands

#define WSC_WAIT_ON_PUTS 'W'
#define WSC_SIGNAL       'S'
#define WSC_OVERLAPPED   'O'

// SioEvent, SioEventChar, and SioEventWait codes

#define WSC_IO_PENDING   2
#define WSC_IO_COMPLETE  3

// SioPortInfo parameters

#define WSC_PORT_BAUD  'B'
#define WSC_PORT_CPS   'C'

// DTR & RTS selection masks

#define WSC_DTR_MASK   0x01
#define WSC_RTS_MASK   0x02

// END

