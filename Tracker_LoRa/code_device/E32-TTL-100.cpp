#include <Arduino.h>
#include <SoftwareSerial.h>
#include "main.h"
#include "E32-TTL-100.h"


bool AUX_HL;
extern SoftwareSerial softSerial;


RET_STATUS SleepModeCmd(uint8_t CMD, void* pBuff)
{
  RET_STATUS STATUS = RET_SUCCESS;

  Serial.print("SleepModeCmd: 0x");  Serial.println(CMD, HEX);
  WaitAUX_H();

  SwitchMode(MODE_3_SLEEP);

  switch (CMD)
  {
    case W_CFG_PWR_DWN_SAVE:  // 0xC0 
      STATUS = Write_CFG_PDS((struct CFGstruct* )pBuff);
      break;
    case R_CFG: // 0xC1
      STATUS = Read_CFG((struct CFGstruct* )pBuff);
      break;
    case W_CFG_PWR_DWN_LOSE:  // 0xC2

      break;
    case R_MODULE_VERSION: // 0xC3
      Read_module_version((struct MVerstruct* )pBuff);
      break;
    case W_RESET_MODULE: // 0xC4
      Reset_module();
      break;

    default:
      return RET_INVALID_PARAM;
  }

  WaitAUX_H();
  return STATUS;
}



RET_STATUS SettingModule(struct CFGstruct *pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

#ifdef Device_A
  pCFG->ADDH = DEVICE_A_ADDR_H;
  pCFG->ADDL = DEVICE_A_ADDR_L;
#else
  pCFG->ADDH = DEVICE_B_ADDR_H;
  pCFG->ADDL = DEVICE_B_ADDR_L;
#endif

  pCFG->OPTION_bits.trsm_mode =TRSM_FP_MODE;
  pCFG->OPTION_bits.tsmt_pwr = TSMT_PWR_20DB;

  STATUS = SleepModeCmd(W_CFG_PWR_DWN_SAVE, (void* )pCFG);  // write config  (0xC0)

  SleepModeCmd(W_RESET_MODULE, NULL); // Reset module (0xC4)

  STATUS = SleepModeCmd(R_CFG, (void* )pCFG);// Read config  (0xC1) 

  return STATUS;
}


void SwitchMode(MODE_TYPE mode)
{
  if(!chkModeSame(mode))
  {
    WaitAUX_H();

    switch (mode)
    {
      case MODE_0_NORMAL:
        // Mode 0 | normal operation
        digitalWrite(M0_PIN, LOW);
        digitalWrite(M1_PIN, LOW);
        break;
      case MODE_1_WAKE_UP:
        digitalWrite(M0_PIN, HIGH);
        digitalWrite(M1_PIN, LOW);
        break;
      case MODE_2_POWER_SAVIN:
        digitalWrite(M0_PIN, LOW);
        digitalWrite(M1_PIN, HIGH);
        break;
      case MODE_3_SLEEP:
        // Mode 3 | Setting operation
        digitalWrite(M0_PIN, HIGH);
        digitalWrite(M1_PIN, HIGH);
        break;
      default:
        return ;
    }

    WaitAUX_H();
    delay(20); // PB 10
  }
}


RET_STATUS WaitAUX_H()
{
  RET_STATUS STATUS = RET_SUCCESS;

  uint8_t cnt = 0;
  uint8_t data_buf[100], data_len;

  while((ReadAUX()==LOW) && (cnt++<TIME_OUT_CNT))
  {
    Serial.print(".");
    delay(100);
  }

  if(cnt==0)
  {
  }
  else if(cnt>=TIME_OUT_CNT)
  {
    STATUS = RET_TIMEOUT;
    Serial.println(" TimeOut");
  }
  else
  {
    Serial.println("");
  }

  return STATUS;
}


RET_STATUS Write_CFG_PDS(struct CFGstruct* pCFG)
{
  softSerial.write((uint8_t *)pCFG, 6);

  WaitAUX_H();
  delay(1200);  //need ti check

  return RET_SUCCESS;
}



RET_STATUS Read_CFG(struct CFGstruct* pCFG)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_CFG);

  //3. Receive configure
  STATUS = Module_info((uint8_t *)pCFG, sizeof(CFGstruct));
  if(STATUS == RET_SUCCESS)
  {
  //Serial.print("  HEAD:     ");  Serial.println(pCFG->HEAD, HEX);
  Serial.print("  ADDRESS:     ");  Serial.print((256*pCFG->ADDH + pCFG->ADDL), HEX);
  //Serial.print("  ADDL:     ");  Serial.println(pCFG->ADDL, HEX);

  Serial.print("  CHANNEL:     ");  Serial.println(pCFG->CHAN, HEX);
  }

  return STATUS;
}



RET_STATUS Read_module_version(struct MVerstruct* MVer)
{
  RET_STATUS STATUS = RET_SUCCESS;

  //1. read UART buffer.
  cleanUARTBuf();

  //2. send CMD
  triple_cmd(R_MODULE_VERSION);

  //3. Receive configure
  STATUS = Module_info((uint8_t *)MVer, sizeof(MVerstruct));
  if(STATUS == RET_SUCCESS)
  {
    Serial.print("  HEAD:     0x");  Serial.println(MVer->HEAD, HEX);
    Serial.print("  Model:    0x");  Serial.print(MVer->Model, HEX);
    Serial.print("  Version:  0x");  Serial.print(MVer->Version, HEX);
    Serial.print("  features: 0x");  Serial.print(MVer->features, HEX);
  }

  return RET_SUCCESS;
}



void Reset_module()
{
  triple_cmd(W_RESET_MODULE);

  WaitAUX_H();
  delay(1000);
}




bool chkModeSame(MODE_TYPE mode)
{
  static MODE_TYPE pre_mode = MODE_INIT;

  if(pre_mode == mode)
  {
    //Serial.print("SwitchMode: (no need to switch) ");  Serial.println(mode, HEX);
    return true;
  }
  else
  {
    Serial.print("SwitchMode: from ");  Serial.print(pre_mode, HEX);  Serial.print(" to ");  Serial.println(mode, HEX);
    pre_mode = mode;
    return false;
  }
}



void triple_cmd(SLEEP_MODE_CMD_TYPE Tcmd)
{
  uint8_t CMD[3] = {Tcmd, Tcmd, Tcmd};
  softSerial.write(CMD, 3);
  delay(50);  //need ti check
}


void cleanUARTBuf()
{
  bool IsNull = true;

  while (softSerial.available())
  {
    IsNull = false;

    softSerial.read();
  }
}


RET_STATUS Module_info(uint8_t* pReadbuf, uint8_t buf_len)
{
  RET_STATUS STATUS = RET_SUCCESS;
  uint8_t Readcnt, idx;

  Readcnt = softSerial.available();
  //Serial.print("softSerial.available(): ");  Serial.print(Readcnt);  Serial.println(" bytes.");
  if (Readcnt == buf_len)
  {
    for(idx=0;idx<buf_len;idx++)
    {
      *(pReadbuf+idx) = softSerial.read();
      Serial.print(" 0x");
      Serial.print(0xFF & *(pReadbuf+idx), HEX);    // print as an ASCII-encoded hexadecimal
    } Serial.println("");
  }
  else
  {
    STATUS = RET_DATA_SIZE_NOT_MATCH;
    Serial.print("  RET_DATA_SIZE_NOT_MATCH - Readcnt: ");  Serial.println(Readcnt);
    cleanUARTBuf();
  }

  return STATUS;
}


bool ReadAUX()
{
  int val = analogRead(AUX_PIN);

  if(val<50)
  {
    AUX_HL = LOW;
  }else {
    AUX_HL = HIGH;
  }

  return AUX_HL;
}

RET_STATUS ReceiveMsg(uint8_t *pdatabuf, uint8_t *data_len)
{

  RET_STATUS STATUS = RET_SUCCESS;
  uint8_t idx;

  SwitchMode(MODE_0_NORMAL);
  *data_len = softSerial.available();

  if (*data_len > 0)
  {
    Serial.print("ReceiveMsg: ");  Serial.print(*data_len);  Serial.println(" bytes.");

    for(idx=0;idx<*data_len;idx++)
      *(pdatabuf+idx) = softSerial.read();

    for(idx=0;idx<*data_len;idx++)
    {
      Serial.print(" 0x");
      Serial.print(0xFF & *(pdatabuf+idx), HEX);    // print as an ASCII-encoded hexadecimal
    } Serial.println("");
  }
  else
  {
    STATUS = RET_NOT_IMPLEMENT;
  }

  return STATUS;
}



RET_STATUS SendMsg(uint8_t data)
{
  RET_STATUS STATUS = RET_SUCCESS;

  if(ReadAUX()!=HIGH) return RET_NOT_IMPLEMENT;
    delay(10);
  if(ReadAUX()!=HIGH) return RET_NOT_IMPLEMENT;

  //TRSM_FP_MODE
  //Send format : ADDH ADDL CHAN DATA_0 DATA_1 DATA_2 ...
  uint8_t SendBuf[4] = { DEVICE_A_ADDR_H, DEVICE_A_ADDR_L, CHANNEL, data};

  softSerial.write(SendBuf, 4);

  return STATUS;
}



