/*
 * FAN_Subsystem.c
 *
 * Created: 11/02/2013 23:10:36
 *  Author: NASSER GHOSEIRI
 * Company: Butterfly Labs
 */ 

// Include standard definitions
#include "std_defs.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include <string.h>
#include "AVR32X\AVR32_Module.h"
#include <avr32/io.h>
#include "AVR32_OptimizedTemplates.h"
#include "FAN_Subsystem.h"
#include "ASIC_Engine.h"

// Now to our codes
volatile void FAN_SUBSYS_Initialize(void)
{
	// Initialize state to 0
	__AVR32_FAN_Initialize();
	FAN_SUBSYS_SetFanState(FAN_STATE_AUTO);			
	GLOBAL_CRITICAL_TEMPERATURE = FALSE;
}

volatile void FAN_SUBSYS_IntelligentFanSystem_Spin(void)
{
	
	// Check temperature
	volatile int iTemp1 = __AVR32_A2D_GetTemp1();
	volatile int iTemp2 = __AVR32_A2D_GetTemp2();
	volatile int iTempHigh = 80;  // VERY HIGH SPEED MODE ( In case of problems with temp sensors )
	
	// Using averaged temp is risky. Preferring highest temp reading 
	//volatile int iTempAveraged = (iTemp1 > iTemp2) ? iTemp1 : iTemp2; // (iTemp2 + iTemp1) / 2;
	if ( iTemp1 >= iTemp2 ){ iTempHigh = iTemp1; }
	else { iTempHigh = iTemp2; }
	
	// Check for critical temperature on either sensor
	if ( iTempHigh >= 90 )  //Do this if only 1 reaches 90° ( in case 1 has a low amount of engines or  )
	{
		// Holy jesus! We're in a critical situation...
		GLOBAL_CRITICAL_TEMPERATURE = TRUE;
		// Set max fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_REMAIN_FULL_SPEED);
			
	}

	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (iTempHigh < 60) // Hysterysis
		{ 
			GLOBAL_CRITICAL_TEMPERATURE = FALSE;
				
			// Increase thermal cycle counter
			GLOBAL_TOTAL_THERMAL_CYCLES++;
				
			// Also, restart the ASICs
			#if defined(__ASICS_RESTART_AFTER_HIGH_TEMP_RECOVERY)
				init_ASIC();				
			#endif
		}
	}
	
	
	// Do we remain at full speed? If so, get it done and return
	#if defined(FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED)
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_REMAIN_FULL_SPEED);
		return;
	#endif
	
	// We're in AUTO mode... There are rules to respect form here...
	if (iTempHigh <= 32)
	{
		FAN_ActualState = FAN_STATE_VERY_SLOW;
	}
	else if ((iTempHigh > 35) && (iTempHigh <= 42))
	{
		FAN_ActualState = FAN_STATE_SLOW;
	}
	else if ((iTempHigh > 45) && (iTempHigh <= 53))
	{
		FAN_ActualState = FAN_STATE_MEDIUM;
	}
	else if ((iTempHigh > 57) && (iTempHigh <= 67))
	{
		FAN_ActualState = FAN_STATE_FAST;
	}
	else if (iTempHigh > 70)
	{
		FAN_ActualState = FAN_STATE_VERY_FAST;
	}
	
	
	// Ok, now set the FAN speed according to our setting
	switch (FAN_ActualState)
	{
	case FAN_STATE_VERY_SLOW:
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_SLOW);
		break;
	case FAN_STATE_SLOW:
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_SLOW);
		break;
	case FAN_STATE_MEDIUM:
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_MEDIUM);
		break;
	case FAN_STATE_FAST:
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_FAST);
		break;
	case FAN_STATE_VERY_FAST:
	default:
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_FAST);
		break;
	}	
		
	// Ok, We're done...
}

volatile void FAN_SUBSYS_SetFanState(char iState)
{
	FAN_ActualState = iState;
	FAN_ActualState_EnteredTick = MACRO_GetTickCountRet;
}
