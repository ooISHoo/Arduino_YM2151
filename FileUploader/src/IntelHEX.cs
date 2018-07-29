using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ArduinoFileUploader
{
    public class IntelHEX
    {
        public int StartAddress;
        public  byte[] Data;
        List<DataRecord>  RecordList=new List<DataRecord>();
        public IntelHEX(string path)
        {
            Load(path);
        }
        public IntelHEX()
        {
        }
        public int Load(string path)
        {
            if(!System.IO.File.Exists(path))return 0;
            // 最初の読み込みで先頭アドレスとサイズを確定する
            System.IO.StreamReader sr = new System.IO.StreamReader(path);
            char[]  tokenbuf=new char[16];
            int cpos,datalength,offset_h,offset_l,command;
            while (sr.Peek() > 0)
            {
                string linestr = sr.ReadLine();
                cpos=0;
                linestr.CopyTo(cpos, tokenbuf, 0, 1);cpos+=1;
                if (tokenbuf[0] != ':') continue;
                datalength = Convert.ToInt32(linestr.Substring(cpos,2), 16); cpos += 2;
                offset_h = Convert.ToInt32(linestr.Substring(cpos, 2), 16); cpos += 2;
                offset_l = Convert.ToInt32(linestr.Substring(cpos, 2), 16); cpos += 2;
                command = Convert.ToInt32(linestr.Substring(cpos, 2), 16); cpos += 2;
                switch (command)
                {
                    case    0://Data
                        byte[] localdata = new byte[datalength];
                        for (int i = 0; i < datalength; i++)
                        {
                            localdata[i] = Convert.ToByte(linestr.Substring(cpos, 2), 16); cpos += 2;
                        }
                        DataRecord record = new DataRecord((offset_h << 8) | offset_l, datalength, localdata);
                        RecordList.Add(record);
                        break;
                    case    1://End
                    case    2://SegmentAddress
                    case    3://StartSegmentAddress
                    case    4://ExtLinearAddressCode
                    case    5://StartLinearAddress
                        break;
                }

            }
            sr.Close();
            MargeRecord();
            return Data.Length;
        }
        void MargeRecord()
        {
            int min = int.MaxValue;
            int max = -int.MaxValue;
            foreach (DataRecord record in RecordList)
            {
                if (min > record.Address)
                {
                    min = record.Address;
                }
                if (max < record.Address + record.Length)
                {
                    max = record.Address + record.Length;
                }
            }
            StartAddress = min;
            Data = new byte[max - min];
            foreach (DataRecord record in RecordList)
            {
                Array.Copy(record.Data, 0, Data, record.Address - StartAddress, record.Length);
            }
        }

        class DataRecord
        {
            public int Address;
            public int Length;
            public byte[] Data;
            public DataRecord(int addr, int len, byte[] data)
            {
                Address = addr;
                Length = len;
                Data = data;
            }
        }

    }
}
