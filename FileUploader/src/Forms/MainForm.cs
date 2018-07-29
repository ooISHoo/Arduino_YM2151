using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Threading;

namespace ArduinoFileUploader
{
    public partial class MainForm : Form
    {
        STK500 STK500Ctrl = new STK500();
        System.Windows.Forms.Timer AutoLoaderTimer = new System.Windows.Forms.Timer();
        int AutoLoaderInterval = 60;
        string AutoLoaderPath = null;
        string AutoLoaderFilter = "*.mdx";
        string[] AutoLoaderFiles;
        int AutoLoaderIndex;
        List<Board> BoardList = new List<Board>();
        Board CurrentBord=null;
        List<ToolStripMenuItem> BoardListMenuItem = new List<ToolStripMenuItem>();
        public MainForm()
        {
            LoadBoardList();
            InitializeComponent();
            scanningSerialPort();
            careateBoardListMenuItem();
            restoreRegistry();
            updateStatus();
            comboBox2.SelectedIndex = 0;
            STK500Ctrl.StateCallback += new STK500.StateCallbackMethod(updateStatus);
            STK500Ctrl.ReciveCallback += new STK500.ReciveCallbackMethod(ReciveData);
            AutoLoaderTimer.Tick += new EventHandler(FormTimer_Tick);
        }

        void FormTimer_Tick(object sender, EventArgs e)
        {
            if (STK500Ctrl.IsRunningCritical || STK500Ctrl.CountMessage() > 0)
            {
                AutoLoaderTimer.Interval = AutoLoaderInterval * 1000;
                return;
            }
            System.Text.StringBuilder sb = new System.Text.StringBuilder();
            AutoLoaderTimer.Interval = AutoLoaderInterval * 1000;
            sb.Append("Loading..");
            sb.Append(AutoLoaderFiles[AutoLoaderIndex]);
            sb.Append(System.Environment.NewLine);
            textBox1.AppendText(sb.ToString());
            if (!sendImage(null, AutoLoaderFiles[AutoLoaderIndex], true))
            {
                AutoLoaderTimer.Stop();
                MessageBox.Show(Resources.Strings.ErrorMessage_AutoLoad);
                return;
            }
            AutoLoaderIndex++;
            if (AutoLoaderIndex >= AutoLoaderFiles.Length) AutoLoaderIndex = 0;
        }

        delegate void UpdateStatusDelegate();
        private void updateStatus()
        {
            if (InvokeRequired)
            {
                Invoke(new UpdateStatusDelegate(updateStatus));
                return;
            }
            if (AutoLoaderTimer.Enabled && STK500Ctrl.MainState == STK500.MainStateEnum.Abort)
            {
                AutoLoaderTimer.Stop();
            }
            if (STK500Ctrl.IsRunningCritical)
            {
                if(button3.Enabled)
                    button3.Enabled = false;
            }
            else
            {
                if(!button3.Enabled)
                    button3.Enabled = true;
            }
            if (AutoLoaderTimer.Enabled && !AutoLoadToolStripMenuItem.Checked)
            {
                button3.Enabled = false;
                AutoLoadToolStripMenuItem.Checked = true;
            }
            if (!AutoLoaderTimer.Enabled && AutoLoadToolStripMenuItem.Checked)
            {
                button3.Enabled = true;
                AutoLoadToolStripMenuItem.Checked = false;
            }
            switch (STK500Ctrl.MainState)
            {
                case STK500.MainStateEnum.Reset:
                    toolStripStatusLabel1.Text = STK500Ctrl.MainState.ToString();
                    break;
                case STK500.MainStateEnum.Send:
                    toolStripStatusLabel1.Text = STK500Ctrl.MainState.ToString() + String.Format(":{0}/{1}", STK500Ctrl.RecordIndex+1, STK500Ctrl.RecordCount);
                    toolStripProgressBar1.Value = (int)(STK500Ctrl.RecordProcessing * 100f);
                    break;
                case STK500.MainStateEnum.Verify:
                    toolStripStatusLabel1.Text = STK500Ctrl.MainState.ToString() + String.Format(":{0}/{1}", STK500Ctrl.RecordIndex+1, STK500Ctrl.RecordCount);
                    toolStripProgressBar1.Value = (int)(STK500Ctrl.RecordProcessing * 100f);
                    break;
                default:
                    toolStripStatusLabel1.Text = STK500Ctrl.MainState.ToString()+':'+STK500Ctrl.ErrorState.ToString();
                    if (AutoLoaderTimer.Enabled)
                    {
                        toolStripStatusLabel1.Text += String.Format(" ..AutoRun({0}/{1})",AutoLoaderIndex+1,AutoLoaderFiles.Length);
                    }
                    toolStripProgressBar1.Value = 0;
                    break;
            }
            foreach (ToolStripMenuItem item in BoardListMenuItem)
            {
                if (item.Tag == (object)CurrentBord)
                { item.Checked = true; }
                else { item.Checked = false; }
            }

            statusStrip1.Invalidate();
        }

        delegate void ReciveDataDelegate(string str);
        private void ReciveData(string str){
            if (InvokeRequired)
            {
                Invoke(new ReciveDataDelegate(ReciveData),str);
                return;
            }
            textBox1.AppendText(str);
        }

        private void scanningSerialPort()
        {
            comboBox1.Items.Clear();
            string[] portnames = System.IO.Ports.SerialPort.GetPortNames();
            foreach (string str in portnames)
            {
                comboBox1.Items.Add(str);
            }
            if (portnames == null || portnames.Length ==0)
            {
                comboBox1.Enabled = false;
            }
            else
            {
                comboBox1.Enabled = true;
                comboBox1.SelectedIndex = 0;
            }
        }

        private void careateBoardListMenuItem()
        {
            foreach (Board board in BoardList)
            {
                ToolStripMenuItem menuitem = new ToolStripMenuItem(board.Name);
                menuitem.Tag = (object)board;
                menuitem.Click += new EventHandler(DeviceTypeMenuItem_Click);
                DeviceTypeToolStripMenuItem.DropDown.Items.Add(menuitem);
                BoardListMenuItem.Add(menuitem);
            }
        }

        private bool sendImage(string hexname, string upfilename,bool poperror)
        {
            if(CurrentBord==null){
                if (poperror) MessageBox.Show(Resources.Strings.ErrorMessage_BordNotSpecified);
                return false;
            }
            if (STK500Ctrl.IsRunningCritical)
            {
                if (poperror) MessageBox.Show(Resources.Strings.ErrorMessage_RunningWorkerThread);
                return false;
            }
            if (STK500Ctrl.CountMessage() > 0)
            {
                if (poperror) MessageBox.Show(Resources.Strings.ErrorMessage_RunningWorkerThread);
                return false;
            }
            button3.Enabled = false;
            STK500.DataRecord code_recoed = null;
            STK500.DataRecord data_recoed = null;
            if (hexname!=null && System.IO.File.Exists(hexname))
            {
                IntelHEX target_code = new IntelHEX(hexname);
                code_recoed = new STK500.DataRecord();
                code_recoed.Address = target_code.StartAddress;
                code_recoed.Data = target_code.Data;
                code_recoed.Verify = checkBox3.Checked;
                if (target_code.StartAddress + target_code.Data.Length > numericUpDown1.Value)
                {
                    if(poperror) MessageBox.Show(Resources.Strings.ErrorMessage_CodeTooLarge);
                    button3.Enabled = true;
                    return false;
                }
            }
            if (upfilename != null && System.IO.File.Exists(upfilename))
            {
                System.IO.FileStream fs = new System.IO.FileStream(upfilename, System.IO.FileMode.Open);
                int file_size = (int)fs.Length;
                byte[] file_image = new byte[file_size + 1];
                fs.Read(file_image, 0, (int)file_size);
                fs.Close();

                if (System.IO.Path.GetExtension(upfilename).ToLower() == ".mdx")
                { // If uploading file is mdx than getting file infomation.
                    MDXTinyParser mdx_parser = new MDXTinyParser(file_image);
                    toolStripStatusLabel2.Text = mdx_parser.GetTitle();
                }
                else
                {
                    toolStripStatusLabel2.Text = "--No MDX--";
                }

                data_recoed = new STK500.DataRecord();
                data_recoed.Address = Decimal.ToInt32(numericUpDown1.Value);
                data_recoed.Data = file_image;
                data_recoed.Verify = checkBox3.Checked;
                if (numericUpDown1.Value + file_size > CurrentBord.BootLoaderAddress)
                {
                    if (poperror) MessageBox.Show(Resources.Strings.ErrorMessage_UploadTooLarge);
                    button3.Enabled = true;
                    return false;
                }
            }
            else
            {
                toolStripStatusLabel2.Text = "--No MDX--";
            }
            if (code_recoed == null && data_recoed == null)
            {
                if(poperror) MessageBox.Show(Resources.Strings.ErrorMessage_DataNotExist);
                button3.Enabled = true;
                return false;
            }
            pushSerialSetup();
            if (code_recoed != null)
                STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetRecord, (object)code_recoed));
            if (data_recoed != null)
                STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetRecord, (object)data_recoed));

            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.Start));
            STK500Ctrl.AwakeWorker();
            updateStatus();
            return true;
        }

        private void pushSerialSetup()
        {
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.Clear));
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetPortName, (object)comboBox1.SelectedItem.ToString()));
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetDevice, (object)CurrentBord));
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetAfterWait, (object)checkBox4.Checked));
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.SetBaud, (object)Convert.ToInt32(comboBox2.SelectedItem)));
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            storeRegistry();
            STK500Ctrl.AbortThread();
        }

        // UI callback
        private void ExitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void TargetResetToolStripMenuItem_Click(object sender, EventArgs e)
        {
            pushSerialSetup();
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.Start));
            STK500Ctrl.AwakeWorker();
            updateStatus();
        }

        private void RescanSerialPortToolStripMenuItem_Click(object sender, EventArgs e)
        {
            scanningSerialPort();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog fd = new System.Windows.Forms.OpenFileDialog();
            fd.CheckFileExists = true;
            fd.Filter = "TargetCode HEX file (*.hex)|*.hex";
            if (fd.ShowDialog() == DialogResult.OK && fd.CheckFileExists)
            {
                textBox_TargetCode.Text = fd.FileName;
                checkBox1.Checked = true;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog fd = new System.Windows.Forms.OpenFileDialog();
            fd.CheckFileExists = true;
            fd.Filter = "Upload file (*.*)|*.*";
            if (fd.ShowDialog() == DialogResult.OK && fd.CheckFileExists)
            {
                textBox_UploadFile.Text = fd.FileName;
                checkBox2.Checked = true;
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            string codefile = null;
            string mdxfile = null;
            if (textBox_TargetCode.Text.Length > 0 && checkBox1.Checked)
            {
                codefile = textBox_TargetCode.Text;
            }
            if (textBox_UploadFile.Text.Length > 0 && checkBox2.Checked)
            {
                mdxfile = textBox_UploadFile.Text;
            }
            if (sendImage(codefile, mdxfile, true))
                updateStatus();
        }

        private void ExitSerialMonitorToolStripMenuItem_Click(object sender, EventArgs e)
        {
            STK500Ctrl.PushMessage(new STK500.MessageRecord(STK500.MessageRecord.MsgType.Clear));
        }

        private void AutoLoadSettingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AutoLoadSettings setting = new AutoLoadSettings();
            setting.IntervalTime = AutoLoaderInterval;
            setting.FolderName = AutoLoaderPath;
            setting.FileFilter = AutoLoaderFilter;
            if (setting.ShowDialog() == DialogResult.OK)
            {
                AutoLoaderInterval = setting.IntervalTime;
                AutoLoaderPath = setting.FolderName;
                AutoLoaderFilter = setting.FileFilter;
            }
        }

        private void AutoLoadToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AutoLoaderFiles = System.IO.Directory.GetFiles(AutoLoaderPath, "*.mdx", System.IO.SearchOption.TopDirectoryOnly);
            if (AutoLoaderFiles == null || AutoLoaderFiles.Length == 0)
            {
                MessageBox.Show(Resources.Strings.ErrorMessage_DirectoryNotFound);
                return;
            }
            if (AutoLoaderTimer.Enabled)
            {
                AutoLoaderTimer.Stop();
            }
            else
            {
                AutoLoaderTimer.Interval = 1000;
                AutoLoaderIndex = 0;
                AutoLoaderTimer.Start();
            }
            updateStatus();
        }

        private void AboutThisProgramToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutThisProgramForm form = new AboutThisProgramForm();
            form.ShowDialog();
        }

        private void DeviceTypeMenuItem_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            CurrentBord = (Board)item.Tag;
            updateStatus();
        }

        // D&D
        private void MainForm_DragEnter(object sender, DragEventArgs e)
        {
            if (AutoLoaderTimer.Enabled)
            {
                return;
            }
            e.Effect = DragDropEffects.Copy;
        }

        private void textBox_TargetCode_DragDrop(object sender, DragEventArgs e)
        {
            if (AutoLoaderTimer.Enabled)
            {
                return;
            }
            foreach (string fname in (string[])e.Data.GetData(DataFormats.FileDrop))
            {
                string ext = System.IO.Path.GetExtension(fname);
                ext.ToLower();
                if (ext == ".hex")
                {
                    textBox_TargetCode.Text = fname;
                    checkBox1.Checked = true;
                    break;
                }
            }
        }

        private void textBox_UploadFile_DragDrop(object sender, DragEventArgs e)
        {
            if (AutoLoaderTimer.Enabled)
            {
                return;
            }
            foreach (string fname in (string[])e.Data.GetData(DataFormats.FileDrop))
            {
                textBox_UploadFile.Text = fname;
                checkBox2.Checked = true;
                break;
            }
        }

        private void textBox1_DragDrop(object sender, DragEventArgs e)
        {
            if (AutoLoaderTimer.Enabled)
            {
                return;
            }
            textBox_UploadFile_DragDrop(sender, e);
            button3_Click(null, null);
        }

        // store&restore registry
        private void storeRegistry()
        {
            Microsoft.Win32.RegistryKey regkey;
            regkey = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(Resources.Strings.RegKey_RootDir);
            regkey.SetValue(Resources.Strings.RegKey_TargetAddress, Decimal.ToInt32(numericUpDown1.Value),Microsoft.Win32.RegistryValueKind.DWord);
            if (CurrentBord != null)
            {
                regkey.SetValue(Resources.Strings.RegKey_BoardName, CurrentBord.Name, Microsoft.Win32.RegistryValueKind.String);
            }
        }

        private void restoreRegistry()
        {
            Microsoft.Win32.RegistryKey regkey;
            regkey = Microsoft.Win32.Registry.CurrentUser.OpenSubKey(Resources.Strings.RegKey_RootDir);
            if (regkey != null)
            {
                if (regkey.GetValue(Resources.Strings.RegKey_TargetAddress, null) != null)
                    numericUpDown1.Value = new Decimal((int)regkey.GetValue(Resources.Strings.RegKey_TargetAddress));
                if (regkey.GetValue(Resources.Strings.RegKey_BoardName, null) != null)
                {
                    string str = (string)regkey.GetValue(Resources.Strings.RegKey_BoardName);
                    foreach (Board board in BoardList)
                    {
                        if (str == board.Name)
                        {
                            CurrentBord = board;
                            break;
                        }
                    }
                }
            }
        }

        // loading board list
        public void LoadBoardList()
        {
            // 
            System.IO.StreamReader sr;

            if (System.IO.File.Exists("boards.xml"))
            {
                sr = new System.IO.StreamReader("boards.xml");
            }
            else
            {
                System.Reflection.Assembly asm = System.Reflection.Assembly.GetExecutingAssembly();
                sr = new System.IO.StreamReader(
                    asm.GetManifestResourceStream("ArduinoFileUploader.Resources.boards.xml")
                    );
            }
            System.Xml.XmlTextReader reader = new System.Xml.XmlTextReader(sr.BaseStream);

            string lasttext = null;
            Board lastboard = null;
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case System.Xml.XmlNodeType.Element:
                        if (reader.Name == "board")
                        {
                            lastboard = new Board();
                        }
                        lasttext = null;
                        break;
                    case System.Xml.XmlNodeType.Text:
                        lasttext = reader.Value;
                        break;
                    case System.Xml.XmlNodeType.EndElement:
                        if (reader.Name == "name")
                        {
                            lastboard.Name = lasttext;
                        }
                        else if (reader.Name == "bootloader_address")
                        {
                            lastboard.BootLoaderAddress = int.Parse(lasttext);
                        }
                        else if (reader.Name == "upload_speed")
                        {
                            lastboard.UpLoadSpeed = int.Parse(lasttext);
                        }
                        else if (reader.Name == "dtr_control")
                        {
                            if (lasttext == "Enable1000mSecAndDisable")
                            {
                                lastboard.DTRControl = Board.DTRControlTypeEnum.Enable1000mSecAndDisable;
                            }
                            else if (lasttext == "EnableAndThrough")
                            {
                                lastboard.DTRControl = Board.DTRControlTypeEnum.EnableAndThrough;
                            }
                            else
                            {   // perse error
                                lastboard.DTRControl = Board.DTRControlTypeEnum.Unknown;
                            }
                        }
                        else if (reader.Name == "software_major_version")
                        {
                            lastboard.SoftwareMajorVersion = byte.Parse(lasttext);
                        }
                        else if (reader.Name == "software_minor_version")
                        {
                            lastboard.SoftwareMinorVersion = byte.Parse(lasttext);
                        }
                        else if (reader.Name == "board")
                        {
                            BoardList.Add(lastboard);
                        }
                        else
                        {
                            // perse error
                        }
                        break;
                }

            }
        }
    }
}
