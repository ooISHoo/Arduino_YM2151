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
    public partial class AboutThisProgramForm : Form
    {
        public AboutThisProgramForm()
        {
            InitializeComponent();
            label2.Text = Resources.Strings.Version;
            linkLabel1.Text = Resources.Strings.HomePage;
            this.Refresh();
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            linkLabel1.LinkVisited = true;
            System.Diagnostics.Process.Start(Resources.Strings.HomePage);
        }
    }
}
