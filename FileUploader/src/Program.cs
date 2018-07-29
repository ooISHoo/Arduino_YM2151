using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace ArduinoFileUploader
{
    static class Program
    {
        /// <summary>
        /// アプリケーションのメイン エントリ ポイントです。
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Length > 0 && args[0] != null)
            {
                System.Threading.Thread.CurrentThread.CurrentCulture = new System.Globalization.CultureInfo(args[0]);
                System.Threading.Thread.CurrentThread.CurrentUICulture = new System.Globalization.CultureInfo(args[0]);
            }
            if (System.Threading.Thread.CurrentThread.CurrentCulture.TwoLetterISOLanguageName != "ja" && System.Threading.Thread.CurrentThread.CurrentCulture.TwoLetterISOLanguageName != "en")
            {
                System.Threading.Thread.CurrentThread.CurrentCulture = new System.Globalization.CultureInfo("en-US");
                System.Threading.Thread.CurrentThread.CurrentUICulture = new System.Globalization.CultureInfo("en-US");
            }
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}
