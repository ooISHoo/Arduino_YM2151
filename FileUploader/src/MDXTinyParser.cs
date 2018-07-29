using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ArduinoFileUploader
{
    public class MDXTinyParser
    {
        byte[] FileImage;
        public MDXTinyParser(byte[] image)
        {
            FileImage = image;
        }
        public string GetTitle()
        {
            byte[] strstr = new byte[FileImage.Length+1];
            int strindex = 0;
            foreach (byte b in FileImage)
            {
                if (b == 0xd || b==0xa || b==0x1a || b ==0x00 )
                {
                    strstr[strindex] = 0x0;
                    break;
                }
                strstr[strindex++] = b;
            }
            return System.Text.Encoding.GetEncoding(932).GetString(strstr);
        }
    }
}
