#ifndef AZ_LOCAL_MSGLEVELS_H
#define AZ_LOCAL_MSGLEVELS_H

#define _MSGLevel_ 0

//#define MAX(a,b) (((a)>(b))?(a):(b))  ..is defined elsewhere

#define InsertTabs(_numtabs_) for (int i=0;i<_numtabs_;i++)Serial.print('\t')

#define SERIALPRINT(A) {if (NumTabs<=_MSGLevel_){ InsertTabs(NumTabs);Serial.print(A);}}
#define SERIALPRINTLN(A) {if (NumTabs<=_MSGLevel_){ InsertTabs(NumTabs);Serial.println(A);}}
#define SERIAL_PRINT(A) {if (NumTabs<=_MSGLevel_){ Serial.print(A);}}
#define SERIAL_PRINTLN(A) {if (NumTabs<=_MSGLevel_){ Serial.println(A);}}

#define _PRINT_BEGIN_SUB(_noTabs_,_A_,_ch_,_PLENGTH_) \
{ \
	int NumTabs = _noTabs_;if (NumTabs == 0) { Serial.println(); } \
	int PLENGTH = _PLENGTH_; \
	char __ch__=_ch_; \
	{ \
		if (NumTabs<=_MSGLevel_) \
		{ \
			InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }Serial.println(); \
			InsertTabs(NumTabs); Serial.println(_A_); \
			InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }Serial.println(); \
		} \
	} \


#define _PRINT_END_SUB(_noTabs_) \
	{ \
		if (NumTabs != _noTabs_) \
			{ SERIALPRINTLN("Number of tabs don't match!"); } \
		else \
		{ \
			if (NumTabs<=_MSGLevel_) \
			{ \
				{InsertTabs(NumTabs); for (int i = 0; i < PLENGTH; i++) { Serial.print(__ch__); }} \
				Serial.println(); \
			} \
		} \
	} \
} \

#define PRINT_BEGIN(A_) _PRINT_BEGIN_SUB(0,A_,'=',80);
#define PRINT_END(B) \
	{ \
		if (NumTabs<=_MSGLevel_) \
		{ \
			Serial.print(" - ");Serial.print(B);Serial.println(); \
			for (int i = 0; i < PLENGTH; i++) { Serial.print('='); } \
			Serial.println(); \
		} \
	} \
} \

#define PRINT_BEGIN_SUB_1(A_) _PRINT_BEGIN_SUB(1,A_,'*',60);
#define PRINT_END_SUB_1 _PRINT_END_SUB(1);

#define PRINT_BEGIN_SUB_2(A_) _PRINT_BEGIN_SUB(2,A_,'-',60);
#define PRINT_END_SUB_2 _PRINT_END_SUB(2);

#define PRINT_BEGIN_SUB_3(A_) _PRINT_BEGIN_SUB(3,A_,'.',50);
#define PRINT_END_SUB_3 _PRINT_END_SUB(3);

#define PRINT_BEGIN_SUB_4(A_) _PRINT_BEGIN_SUB(4,A_,'_',40);
#define PRINT_END_SUB_4 _PRINT_END_SUB(4);

#define PRINT_BEGIN_SUB_5(A_) _PRINT_BEGIN_SUB(5,A_,'+',30);
#define PRINT_END_SUB_5 _PRINT_END_SUB(5);

#endif
