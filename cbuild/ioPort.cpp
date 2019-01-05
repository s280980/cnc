//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "ioPort.h"
#include "Unit1.h"
#include <stdint.h>
#pragma package(smart_init)
//---------------------------------------------------------------------------

__fastcall ReadThread::ReadThread(bool CreateSuspended)
        : TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------
void __fastcall ReadThread::Execute()
{
  OVERLAPPED overlapped;
  OVERLAPPED overlappedrw;
  COMSTAT comstat;
  DWORD temp, mask, signal;

  overlapped.hEvent = CreateEvent(NULL, true, true, NULL);
  SetCommMask(port->handle, EV_RXCHAR);

  while(!Terminated)
  {
    WaitCommEvent(port->handle, &mask, &overlapped);
    signal = WaitForSingleObject(overlapped.hEvent, INFINITE);
    if(signal == WAIT_OBJECT_0)
    {
      if(GetOverlappedResult(port->handle, &overlapped, &temp, true))
      if((mask & EV_RXCHAR)!=0)
      {
        ClearCommError(port->handle, &temp, &comstat);
        btr = comstat.cbInQue;
        if(btr)
        {
          ReadFile(port->handle, buf, btr, &temp, &overlapped);
          //tick+=btr;
          Synchronize(print);
          if(port->Suspended)port->Resume();
        }//if btr
      }//if if ev_rxchar
    }//if signal == wait
  }//while
  CloseHandle(overlapped.hEvent);
  FreeOnTerminate = true;
}
//---------------------------------------------------------------------------
void __fastcall ReadThread::print()
{
  //void * memcpy( void * destptr, const void * srcptr, size_t num );
  DWORD c;
  if(port->rhead + btr >= BUFSIZE)
  {
    c = BUFSIZE - port->rhead;
    memcpy(port->rbuf + port->rhead, buf, c);
    memcpy(port->rbuf, buf + c, btr - c);
  }
  else
  {
    memcpy(port->rbuf + port->rhead, buf, btr);
  }
  memset(buf, 0, EBUFSIZE);
  c = port->rhead + btr;
  if(c>=BUFSIZE)c-=BUFSIZE;
  port->rhead = c;
}//void
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

__fastcall WriteThread::WriteThread(bool CreateSuspended)
        : TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------
void __fastcall WriteThread::buf_update()
{
  buf_used = 0;
  if(port->wtail == port->whead) { return; }
  if(port->wtail < port->whead)
  {//t<w
    buf_used = port->whead - port->wtail;
    if (buf_used > EBUFSIZE)buf_used = EBUFSIZE;
    memcpy(buf, port->wbuf + port->wtail, buf_used);
    port->wtail += buf_used;
  }//if
  else
  {//w<t
    buf_used = BUFSIZE - port->wtail;
    if (buf_used > EBUFSIZE) buf_used = EBUFSIZE;
    memcpy(buf, port->wbuf + port->wtail, buf_used);
    port->wtail += buf_used;
    if(port->wtail >= BUFSIZE)port->wtail=0;
  }//if else
}//void
//---------------------------------------------------------------------------
void __fastcall WriteThread::Execute()
{
  DWORD temp, signal, c, c2;
  OVERLAPPED overlapped;
  overlapped.hEvent = CreateEvent(NULL, true, true, NULL);

  while(!Terminated)
  {
    Synchronize(buf_update);
    while( buf_used==0 ){ this->Suspend(); Synchronize(buf_update); }
    WriteFile(port->handle, buf, buf_used, &temp, &overlapped);
    signal = WaitForSingleObject(overlapped.hEvent, INFINITE);
    if((signal == WAIT_OBJECT_0) && (GetOverlappedResult(port->handle, &overlapped, &temp, true))) fl = true;
    else fl = false;
    //Synchronize(print);
  }

  CloseHandle(overlapped.hEvent);
  FreeOnTerminate = true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

__fastcall ComPortThread::ComPortThread(bool CreateSuspended)
        : TThread(CreateSuspended), reader(0), writer(0),
        pr_bytes_read(0), pr_command(0), pr_bytes_wait(0)
{
  memset(&pr_buffer,0,sizeof(uint8_t)*PROTOCOL_RX_BUFFER_SIZE);
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::Write(uint8_t data)
{
  if(!writer){return;}
  wbuf[whead++]=data;
  if(whead==BUFSIZE)whead=0;
  if(writer->Suspended){writer->Resume();}
}//void
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall ComPortThread::task_send(task_t * task)
{
  Write( CMD_TASK );
  Write( (task->id>>1) &127 );
  Write( ((task->id<<6) ) &127 );
  //Write(  );
  //Write(  );
  //Write(  );
  //Write(  );
  //Write(  );
  //Write(  );
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::on_link()
{
Form1->Memo1->Lines->Add("link()");
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::stepper_position__rep_dt_set(uint16_t ms)
{
  Write( CMD_STEPPER_POSITION_REP_DT_SET );
  Write((uint8_t)(ms>>7)&127);
  Write((uint8_t)ms&127);
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::on_task_running_state()
{
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::on_stepper_position()
{
  uint8_t ax =  pr_buffer[0]>>4;
  uint32_t ps = pr_buffer[4];
  ps |= ((uint32_t)pr_buffer[3])<<7;
  ps |= ((uint32_t)pr_buffer[2])<<14;
  ps |= ((uint32_t)pr_buffer[1])<<21;
  ps |= ((uint32_t)pr_buffer[0])<<28;
  //stepper_position[ax] = ps;
  Form1->Memo1->Lines->Add("ps["+String(ax)+"]="+String(ps));
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::task_running_state__rep_dt_set(uint16_t ms)
{
  Write(CMD_TASK_RUNNING_STATE_REP_DT_SET);
  Write((ms>>7)&127);
  Write(ms&127);
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::Execute()
{
  while (!Terminated){
    while(rtail == rhead){ Suspend(); }
    uint8_t data = Read();
    if(data>127){
      pr_command = data;
      pr_bytes_wait = pr_bytes_read = 0;
      switch(pr_command){
        case CMD_STEPPER_POSITION:{pr_bytes_wait=5;}break;
        case CMD_TASK_RUNNING_STATE:{pr_bytes_wait=4;}break;
        case CMD_LINK:{on_link();}break;

      }//switch
    }//if cmd
    else if(pr_bytes_wait>0){
      pr_buffer[ pr_bytes_read++ ] = data;
      pr_bytes_wait--;
      if(pr_bytes_wait==0){
        switch(pr_command){
          case CMD_STEPPER_POSITION:{on_stepper_position();}break;
          case CMD_TASK_RUNNING_STATE:{on_task_running_state();}break;

        }//switch
      }//if wait==0
    }//if wait..

  }//while
  FreeOnTerminate = true;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
uint8_t __fastcall ComPortThread::Read()
{
  if(rtail==rhead){ return 255; }
  uint8_t data = rbuf[rtail++];
  if(rtail==BUFSIZE)rtail=0;
  return data;
}
//---------------------------------------------------------------------------
int __fastcall ComPortThread::Open(int numPort, uint32_t boudrate)
{
  if(handle){return -3;}
  String name = "COM"+String(numPort);
  handle = CreateFile(name.c_str(),GENERIC_READ | GENERIC_WRITE, 0,
		     NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  if(handle==INVALID_HANDLE_VALUE){return -1;}
  DCB dcb;
  dcb.DCBlength = sizeof(DCB);
  GetCommState(handle, &dcb);
  /*Form1->Memo1->Lines->Add(dcb.DCBlength);
  Form1->Memo1->Lines->Add(dcb.BaudRate);
  Form1->Memo1->Lines->Add(dcb.fBinary);
  Form1->Memo1->Lines->Add(dcb.fParity);
  Form1->Memo1->Lines->Add(dcb.fOutxCtsFlow);
  Form1->Memo1->Lines->Add(dcb.fOutxDsrFlow);
  Form1->Memo1->Lines->Add(dcb.fDtrControl);
  Form1->Memo1->Lines->Add(dcb.fDsrSensitivity);
  Form1->Memo1->Lines->Add(dcb.fTXContinueOnXoff);
  Form1->Memo1->Lines->Add(dcb.fOutX);
  Form1->Memo1->Lines->Add(dcb.fInX);
  Form1->Memo1->Lines->Add(dcb.fErrorChar);
  Form1->Memo1->Lines->Add(dcb.fNull);
  Form1->Memo1->Lines->Add(dcb.fRtsControl);
  Form1->Memo1->Lines->Add(dcb.fAbortOnError);
  Form1->Memo1->Lines->Add(dcb.fDummy2);
  Form1->Memo1->Lines->Add(dcb.XonLim);
  Form1->Memo1->Lines->Add(dcb.XoffLim);
  Form1->Memo1->Lines->Add(dcb.ByteSize);
  Form1->Memo1->Lines->Add(dcb.StopBits);
  Form1->Memo1->Lines->Add((uint8_t)dcb.XonChar);
  Form1->Memo1->Lines->Add((uint8_t)dcb.XoffChar);
  Form1->Memo1->Lines->Add((uint8_t)dcb.ErrorChar);
  Form1->Memo1->Lines->Add((uint8_t)dcb.EofChar);
  Form1->Memo1->Lines->Add((uint8_t)dcb.EvtChar); */

  dcb.fBinary = 1;
  dcb.fParity = 0;
  dcb.BaudRate = boudrate;
  dcb.ByteSize = 8;
  dcb.fDtrControl = 0;
  dcb.fRtsControl = 0;
  SetCommState(handle, &dcb);
  //COMMTIMEOUTS CommTimeOuts;
  //GetCommTimeouts(handle, &CommTimeOuts);
  //SetCommTimeouts(handle, &CommTimeOuts);
  PurgeComm(handle, PURGE_RXCLEAR);
  PurgeComm(handle, PURGE_TXCLEAR);
  reader = new ReadThread(true);
  if(!reader){CloseHandle(handle); handle=0; return -2;}
  reader->port = this;
  reader->Resume();
  writer = new WriteThread(true);
  if(!writer){reader->Terminate();CloseHandle(handle); handle=0; return -2;}
  writer->port = this;
  writer->Resume();

  return 1;
}
//---------------------------------------------------------------------------
void __fastcall ComPortThread::Close()
{
  if(writer)writer->Terminate();
  if(reader)reader->Terminate();
  CloseHandle(handle);
  handle=0;
}
//---------------------------------------------------------------------------