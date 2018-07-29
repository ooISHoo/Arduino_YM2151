using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ArduinoFileUploader
{
    public partial class AutoLoadSettings : Form
    {
        public string FolderName
        {
            get
            {
                return textBox1.Text;
            }
            set
            {
                textBox1.Text = value;
            }
        }
        public int IntervalTime
        {
            get
            {
                return Decimal.ToInt32(numericUpDown1.Value);
            }
            set
            {
                numericUpDown1.Value = value;
            }
        }
        public string FileFilter
        {
            get
            {
                return textBox2.Text;
            }
            set
            {
                textBox2.Text = value;
            }
        }
        public AutoLoadSettings()
        {
            InitializeComponent();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog diag = new FolderBrowserDialog();
            if (diag.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = diag.SelectedPath;
            }
        }
    }
}
