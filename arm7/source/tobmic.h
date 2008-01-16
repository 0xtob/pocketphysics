#ifndef _TOBMIC_H_
#define _TOBMIC_H_

#define PM_GAIN_OFFSET	3

void tob_PM_SetAmp(u8 control);
void tob_PM_SetGain(u8 gain);
u16 tob_MIC_ReadData112(void);
void tob_StartRecording(u16* buffer, int length);
int tob_StopRecording(void);
void tob_ProcessMicrophoneTimerIRQ(void);
void tob_MIC_On(void);
void tob_MIC_Off(void);

#endif
