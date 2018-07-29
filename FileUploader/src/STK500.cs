using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace ArduinoFileUploader
{
    public class Board
    {
        public string Name;
        public int BootLoaderAddress;
        public int UpLoadSpeed;
        public enum DTRControlTypeEnum
        {
            Enable1000mSecAndDisable,
            EnableAndThrough,
            Unknown
        }
        public DTRControlTypeEnum DTRControl;
        public byte SoftwareMajorVersion;
        public byte SoftwareMinorVersion;
    }
    public class STK500
    {
        public  enum MainStateEnum
        {
            Idle,
            Reset,
            Send,
            Verify,
            ConsoleMonitor,
            Abort
        }
        public MainStateEnum MainState = MainStateEnum.Idle;
        public enum ErrorStatusEnum
        {
            None,
            Success,
            Unknown,
            RequestTimeout,
            AddressFault,
            InvalidVersion,
            InvalidMinorVirsion,
            CannotEnterProgrammingMode,
            SetAddress,
            WriteData,
            ReadData,
            PortNotFound,
            VerifyUnmuch,
        }
        public ErrorStatusEnum ErrorState = ErrorStatusEnum.None;
        public class DataRecord
        {
            public int Address;
            public byte[] Data;
            public bool Verify;
        }
        public class MessageRecord
        {
            public enum MsgType
            {
                Stop,
                Start,
                Clear,
                Abort,
                SetPortName,
                SetDevice,
                SetRecord,
                SetBaud,
                SetAfterWait,
                EOT
            }
            public MsgType Message;
            public object Data;
            public MessageRecord(MsgType msg, object data)
            {
                Message = msg;
                Data = data;
            }
            public MessageRecord(MsgType msg)
            {
                Message = msg;
                Data = null;
            }
        }
        //
        public int RecordIndex;
        public int RecordCount;
        public float RecordProcessing;
        public delegate void StateCallbackMethod();
        public StateCallbackMethod StateCallback=null;
        public delegate void ReciveCallbackMethod(string str);
        public ReciveCallbackMethod ReciveCallback = null;
        //
        private AutoResetEvent AwakeEvent=new AutoResetEvent(false);
        private AutoResetEvent AfterPopMessageEvent=new AutoResetEvent(false);
        private AutoResetEvent CriticalEvent = new AutoResetEvent(true);
        private int CriticalCounter = 0;
        private List<DataRecord> DaraRecordList = new List<DataRecord>();
        private ReaderWriterLock queueLock = new ReaderWriterLock();
        private Queue<MessageRecord> MessageQueue = new Queue<MessageRecord>();
        private Thread SerialIOThread = null;
        private string Portname;
        private bool AfterWait;
        private int ReciveBaudRate;
        private Board CurrentBoard = null;
        // ATMEL STK500 Communication Protocol
        // http://ww1.microchip.com/downloads/en/AppNotes/doc2591.pdf
        private const byte Cmnd_STK_GET_SYNC = 0x30;
        private const byte Cmnd_STK_GET_SIGN_ON = 0x31;
        private const byte Cmnd_STK_GET_PARAMETER = 0x41;
        private const byte Cmnd_STK_ENTER_PROGMODE = 0x50;
        private const byte Cmnd_STK_LEAVE_PROGMODE = 0x51;
        private const byte Cmnd_STK_LOAD_ADDRESS = 0x55;
        private const byte Cmnd_STK_PROG_PAGE = 0x64;
        private const byte Cmnd_STK_READ_PAGE = 0x74;

        private const byte Param_STK_SW_MAJOR = 0x81;
        private const byte Param_STK_SW_MINOR = 0x82;

        private const byte Sync_CRC_EOP = 0x20;

        private const byte Resp_STK_INSYNC = 0x14;
        private const byte Resp_STK_OK = 0x10;

        public STK500()
        {
            //
        }
        public void WorkerThreadMain()
        {
            while (true)
            {
                //Interlocked.Increment
                AwakeEvent.WaitOne();
                AwakeEvent.Reset();
                do
                {
                    MessageRecord mr = PopMessage();
                    if (mr == null)
                    {
                        break; // !!
                    }
                    // System.Console.WriteLine("Message:{0}",mr.Message.ToString());
                    switch (mr.Message)
                    {
                        case MessageRecord.MsgType.Clear:
                            DaraRecordList.Clear();
                            break;
                        case MessageRecord.MsgType.SetPortName:
                            Portname = (string)mr.Data;
                            break;
                        case MessageRecord.MsgType.SetDevice:
                            CurrentBoard = (Board)mr.Data;
                            break;
                        case MessageRecord.MsgType.SetBaud:
                            ReciveBaudRate = (int)mr.Data;
                            break;
                        case MessageRecord.MsgType.SetRecord:
                            DaraRecordList.Add((DataRecord)mr.Data);
                            break;
                        case MessageRecord.MsgType.SetAfterWait:
                            AfterWait = (bool)mr.Data;
                            break;
                        case MessageRecord.MsgType.Start:
                            break;
                        case MessageRecord.MsgType.Abort:
                            SerialIOThread.Abort();
                            break;
                    }
                    if (mr.Message == MessageRecord.MsgType.Start) break;
                } while (true);
                AfterPopMessageEvent.Set();
                Interlocked.Increment(ref CriticalCounter);     // Enter critical section
                if (CurrentBoard == null) continue;
                byte[] rbuf = new byte[256];
                MainState = MainStateEnum.Reset;
                ErrorState = ErrorStatusEnum.None;
                RecordIndex = 0;
                RecordCount = DaraRecordList.Count;
                System.IO.Ports.SerialPort port = new System.IO.Ports.SerialPort(Portname);
                port.ErrorReceived += new System.IO.Ports.SerialErrorReceivedEventHandler(port_ErrorReceived);
                {
                    try
                    {
                        port.Open();
                        port.DiscardInBuffer();
                        port.DiscardOutBuffer();
                    }
                    catch (Exception exp)
                    {
                        System.Console.WriteLine("{0}", exp.Message);
                        ErrorState = ErrorStatusEnum.PortNotFound;
                        goto ErrorExit;
                    }
                    if (StateCallback != null) StateCallback();
                    port.BaudRate = CurrentBoard.UpLoadSpeed;
                    port.WriteTimeout = 1000;
                    port.ReadTimeout = 1000;
                    switch (CurrentBoard.DTRControl)
                    {
                        case Board.DTRControlTypeEnum.Enable1000mSecAndDisable:
                            port.DtrEnable = true;
                            port.DiscardInBuffer();
                            port.DiscardOutBuffer();
                            Thread.Sleep(1000);
                            port.DtrEnable = false;
                            port.DiscardInBuffer();
                            port.DiscardOutBuffer();
                            Thread.Sleep(100);
                            port.DiscardInBuffer();
                            port.DiscardOutBuffer();
                            break;
                        case Board.DTRControlTypeEnum.EnableAndThrough:
                            port.DtrEnable = true;
                            port.DiscardInBuffer();
                            port.DiscardOutBuffer();
                            Thread.Sleep(500);
                            port.DiscardInBuffer();
                            port.DiscardOutBuffer();
                            break;
                    }
                    for (int i = 0; i < 3; i++)
                    {// Get sync
                        byte[] com = {  Cmnd_STK_GET_SYNC,      Sync_CRC_EOP };
                        byte[] rq = {   Resp_STK_INSYNC,        Resp_STK_OK };
                        if (!RequestForAnswer(port, com, rq, rbuf))
                        {
                            ErrorState = ErrorStatusEnum.RequestTimeout;
                            goto ErrorExit;
                        }
                    }
                    if (CurrentBoard.SoftwareMajorVersion < 4)
                    {
                        {// Software major version (No implementation on optiboot V4-)
                            byte[] com = { Cmnd_STK_GET_PARAMETER, Param_STK_SW_MAJOR, Sync_CRC_EOP };
                            byte[] rq = { Resp_STK_INSYNC, CurrentBoard.SoftwareMajorVersion, Resp_STK_OK };
                            if (!RequestForAnswerThanLess(port, com, rq, rbuf))
                            {
                                ErrorState = ErrorStatusEnum.InvalidVersion;
                                goto ErrorExit;
                            }
                        }
                        {// Enter programming mode (No implementation on optiboot V4-)
                            byte[] com = { Cmnd_STK_ENTER_PROGMODE, Sync_CRC_EOP };
                            byte[] rq = { Resp_STK_INSYNC, Resp_STK_OK };
                            if (!RequestForAnswer(port, com, rq, rbuf))
                            {
                                ErrorState = ErrorStatusEnum.CannotEnterProgrammingMode;
                                goto ErrorExit;
                            }
                        }
                    }
                    foreach (DataRecord record in DaraRecordList)
                    {
                        if (DaraRecordList.Count > 0) MainState = MainStateEnum.Send;
                        if (StateCallback != null) StateCallback();
                        if (!CheckData(record))
                        {
                            ErrorState = ErrorStatusEnum.AddressFault;
                            goto ErrorExit;
                        }
                        int sendcount;
                        RecordProcessing = 0;
                        for (sendcount = 0; sendcount < record.Data.Length; )
                        {
                            {// Load address
                                byte[] com = new byte[4];
                                byte[] rq = {   Resp_STK_INSYNC,    Resp_STK_OK };
                                int address = record.Address + sendcount;
                                com[0] = Cmnd_STK_LOAD_ADDRESS;
                                com[1] = (byte)((address >> 1) & 0xff);
                                com[2] = (byte)((address >> 9) & 0xff);
                                com[3] = Sync_CRC_EOP;
                                if (!RequestForAnswer(port, com, rq, rbuf))
                                {
                                    ErrorState = ErrorStatusEnum.SetAddress;
                                    goto ErrorExit;
                                }
                            }

                            byte cur_send = 128;
                            if (sendcount + cur_send > record.Data.Length) cur_send = (byte)(record.Data.Length - sendcount);
                            {// Write page
                                byte[] com = new byte[cur_send + 5];
                                byte[] rq = {   Resp_STK_INSYNC,    Resp_STK_OK };
                                com[0] = Cmnd_STK_PROG_PAGE;
                                com[1] = 0x00;
                                com[2] = cur_send;
                                com[3] = (byte)'F';// 'F'=flash 'E'=eeprom
                                for (int i = 0; i < cur_send; i++)
                                {
                                    com[4 + i] = record.Data[sendcount + i];
                                }
                                com[com.Length - 1] = Sync_CRC_EOP;
                                if (!RequestForAnswer(port, com, rq, rbuf))
                                {
                                    ErrorState = ErrorStatusEnum.WriteData;
                                    goto ErrorExit;
                                }
                            }
                            sendcount += cur_send;
                            RecordProcessing = (float)sendcount / (float)record.Data.Length;
                            if (StateCallback != null) StateCallback();

                        }
                        if (record.Verify)
                        {
                            if (DaraRecordList.Count > 0) MainState = MainStateEnum.Verify;
                            if (StateCallback != null) StateCallback();
                            RecordProcessing = 0;
                            for (sendcount = 0; sendcount < record.Data.Length; )
                            {
                                {// Load address
                                    byte[] com = new byte[4];
                                    byte[] rq = {   Resp_STK_INSYNC,    Resp_STK_OK };
                                    int address = record.Address + sendcount;
                                    com[0] = Cmnd_STK_LOAD_ADDRESS;
                                    com[1] = (byte)((address >> 1) & 0xff);
                                    com[2] = (byte)((address >> 9) & 0xff);
                                    com[3] = Sync_CRC_EOP;
                                    if (!RequestForAnswer(port, com, rq, rbuf))
                                    {
                                        ErrorState = ErrorStatusEnum.SetAddress;
                                        goto ErrorExit;
                                    }
                                }

                                byte cur_recv = 128;
                                if (sendcount + cur_recv > record.Data.Length) cur_recv = (byte)(record.Data.Length - sendcount);
                                {// Read page
                                    byte[] com = new byte[5];
                                    com[0] = Cmnd_STK_READ_PAGE;
                                    com[1] = 0x00;
                                    com[2] = cur_recv;
                                    com[3] = (byte)'F';
                                    com[4] = Sync_CRC_EOP;
                                    if (!RequestForAnswer(port, com, rbuf, cur_recv + 2))
                                    {
                                        ErrorState = ErrorStatusEnum.ReadData;
                                        goto ErrorExit;
                                    }
                                    if (rbuf[0] != Resp_STK_INSYNC || rbuf[cur_recv + 1] != Resp_STK_OK)
                                    {
                                        ErrorState = ErrorStatusEnum.VerifyUnmuch;
                                        goto ErrorExit;
                                    }
                                    for (int i = 0; i < cur_recv; i++)
                                    {
                                        if (record.Data[sendcount + i] != rbuf[i + 1])
                                        {
                                            ErrorState = ErrorStatusEnum.VerifyUnmuch;
                                            goto ErrorExit;
                                        }
                                    }
                                }
                                sendcount += cur_recv;
                                RecordProcessing = (float)sendcount / (float)record.Data.Length;
                                if (StateCallback != null) StateCallback();
                            }
                        }
                        RecordIndex++;
                    }
                    {// Leave programming mode
                        byte[] com = {  Cmnd_STK_LEAVE_PROGMODE,    Sync_CRC_EOP };
                        byte[] rq = {   Resp_STK_INSYNC,            Resp_STK_OK };
                        if (!RequestForAnswer(port, com, rq, rbuf))
                        {
                            ErrorState = ErrorStatusEnum.Unknown;
                            goto ErrorExit;
                        }
                    }
                    if (AfterWait)
                    {
                        Interlocked.Decrement(ref CriticalCounter);// Exit critical section
                        port.BaudRate = ReciveBaudRate;
                        MainState = MainStateEnum.ConsoleMonitor;
                        if (StateCallback != null) StateCallback();
                        System.Text.StringBuilder sb = new System.Text.StringBuilder();
                        while (true)
                        {
                            if (CountMessage() > 0)
                            {
                                port.Close();
                                port.Dispose();
                                MainState = MainStateEnum.Idle;
                                ErrorState = ErrorStatusEnum.Success;
                                // ここでstateをメインに返してInvokeするとAwakeWorkerに突入中の場合デッドロックするのでそのまま次に回す
                                break;
                            }
                            if (port.BytesToRead == 0)
                            {
                                Thread.Sleep(20);
                                continue;
                            }
                            do
                            {
                                char c = (char)port.ReadChar();
                                sb.Append(c);
                            } while (port.BytesToRead > 0);
                            if (sb.Length > 0)
                            {
                                if (ReciveCallback != null) ReciveCallback(sb.ToString());
                                sb.Remove(0, sb.Length);
                            }
                        }
                    }
                    else
                    {
                        port.Close();
                        port.Dispose();
                        Interlocked.Decrement(ref CriticalCounter);
                        MainState = MainStateEnum.Idle;
                        ErrorState = ErrorStatusEnum.Success;
                        if (StateCallback != null) StateCallback();
                        continue;
                    }
                    continue;
                ErrorExit:
                    while (PopMessage() != null) ;
                    port.Close();
                    port.Dispose();
                    if (CriticalCounter == 1) Interlocked.Decrement(ref CriticalCounter);
                    MainState = MainStateEnum.Abort;
                    if (StateCallback != null) StateCallback();
                    continue;
                }
            }
        }

        void port_ErrorReceived(object sender, System.IO.Ports.SerialErrorReceivedEventArgs e)
        {
            throw new NotImplementedException();
        }
        private bool WaitToRead(System.IO.Ports.SerialPort port,int cnt)
        {
            for (int i = 0; i < 1024; i++)
            {
                if (port.BytesToWrite==0 && port.BytesToRead >= cnt) return true;
                Thread.Sleep(4);
            }
            return false;
        }
        private bool CheckData(DataRecord r)
        {
            if (r.Address < 0) return false;
            if (r.Address + r.Data.Length >= CurrentBoard.BootLoaderAddress) return false;
            return true;
        }
        private bool RequestForAnswer(System.IO.Ports.SerialPort port,byte[] command,byte[] result,byte[] readbuf)
        {
            port.Write(command, 0, command.Length);
            if (!WaitToRead(port, result.Length)) return false;
            port.Read(readbuf, 0, result.Length);
            for (int i = 0; i < result.Length; i++)
            {
                if (result[i] != readbuf[i]) return false;
            }
            return true;
        }
        private bool RequestForAnswerThanLess(System.IO.Ports.SerialPort port, byte[] command, byte[] result, byte[] readbuf)
        {
            port.Write(command, 0, command.Length);
            if (!WaitToRead(port, result.Length)) return false;
            port.Read(readbuf, 0, result.Length);
            for (int i = 0; i < result.Length; i++)
            {
                if (i == 1)
                {
                    if (result[i] < readbuf[i]) return false;
                }
                else
                {
                    if (result[i] != readbuf[i]) return false;
                }
            }
            return true;
        }
        private bool RequestForAnswer(System.IO.Ports.SerialPort port, byte[] command, byte[] readbuf,int readlen)
        {
            port.Write(command, 0, command.Length);
            if (!WaitToRead(port, readlen)) return false;
            port.Read(readbuf, 0, readlen);
            return true;
        }
        // worker thread onry
        public int CountMessage()
        {
            return MessageQueue.Count;
        }
        // worker thread onry
        public MessageRecord PopMessage()
        {
            lock (MessageQueue)
            {
                if (MessageQueue.Count == 0) { return null; }
                return MessageQueue.Dequeue();
            }
        }
        // main thread onry
        public void PushMessage(MessageRecord rc)
        {
            lock (MessageQueue)
            {
                MessageQueue.Enqueue(rc);
            }
        }
        public void AwakeWorker()
        {
            if (SerialIOThread == null)
            {
                SerialIOThread = new Thread(WorkerThreadMain);
                SerialIOThread.Name = "STK500Control";
                SerialIOThread.Start();
            }
            int lcount=0;
            while (SerialIOThread.ThreadState != ThreadState.WaitSleepJoin)
            {
                Thread.Sleep(10);
                if (lcount++ > 100) return;
            } 
            AfterPopMessageEvent.Reset();
            AwakeEvent.Set();
            AfterPopMessageEvent.WaitOne();
        }
        public bool IsRunningCritical
        {
            get
            {
                if (CriticalCounter==0)
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }
        
        public void AbortThread()
        {
            if (SerialIOThread != null)
                SerialIOThread.Abort();
        }
    }
}
