
namespace ReviewRecordedIQ
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.comboBoxWaveOut = new System.Windows.Forms.ComboBox();
            this.buttonPlay = new System.Windows.Forms.Button();
            this.buttonFile = new System.Windows.Forms.Button();
            this.buttonPause = new System.Windows.Forms.Button();
            this.trackBarTune = new System.Windows.Forms.TrackBar();
            this.trackBarBfo = new System.Windows.Forms.TrackBar();
            this.labelCenter = new System.Windows.Forms.Label();
            this.labelBfo = new System.Windows.Forms.Label();
            this.comboBoxBandwidth = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBoxPresets = new System.Windows.Forms.GroupBox();
            this.radioButtonLSB = new System.Windows.Forms.RadioButton();
            this.radioButtonUSB = new System.Windows.Forms.RadioButton();
            this.radioButtonCW = new System.Windows.Forms.RadioButton();
            this.labelDialFreq = new System.Windows.Forms.Label();
            this.labelFromSliceIQ = new System.Windows.Forms.Label();
            this.trackBarPlayPosition = new System.Windows.Forms.TrackBar();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.buttonForward = new System.Windows.Forms.Button();
            this.buttonBack = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarTune)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBfo)).BeginInit();
            this.groupBoxPresets.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarPlayPosition)).BeginInit();
            this.SuspendLayout();
            // 
            // comboBoxWaveOut
            // 
            this.comboBoxWaveOut.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxWaveOut.FormattingEnabled = true;
            this.comboBoxWaveOut.Location = new System.Drawing.Point(12, 271);
            this.comboBoxWaveOut.Name = "comboBoxWaveOut";
            this.comboBoxWaveOut.Size = new System.Drawing.Size(242, 21);
            this.comboBoxWaveOut.TabIndex = 0;
            this.comboBoxWaveOut.SelectedIndexChanged += new System.EventHandler(this.comboBoxWaveOut_SelectedIndexChanged);
            // 
            // buttonPlay
            // 
            this.buttonPlay.Enabled = false;
            this.buttonPlay.Location = new System.Drawing.Point(27, 102);
            this.buttonPlay.Name = "buttonPlay";
            this.buttonPlay.Size = new System.Drawing.Size(75, 23);
            this.buttonPlay.TabIndex = 1;
            this.buttonPlay.Text = "Play";
            this.buttonPlay.UseVisualStyleBackColor = true;
            this.buttonPlay.Click += new System.EventHandler(this.buttonPlay_Click);
            // 
            // buttonFile
            // 
            this.buttonFile.Location = new System.Drawing.Point(12, 12);
            this.buttonFile.Name = "buttonFile";
            this.buttonFile.Size = new System.Drawing.Size(121, 23);
            this.buttonFile.TabIndex = 2;
            this.buttonFile.Text = "Select WAV file...";
            this.buttonFile.UseVisualStyleBackColor = true;
            this.buttonFile.Click += new System.EventHandler(this.buttonFile_Click);
            // 
            // buttonPause
            // 
            this.buttonPause.Location = new System.Drawing.Point(128, 102);
            this.buttonPause.Name = "buttonPause";
            this.buttonPause.Size = new System.Drawing.Size(75, 23);
            this.buttonPause.TabIndex = 3;
            this.buttonPause.Text = "Pause";
            this.buttonPause.UseVisualStyleBackColor = true;
            this.buttonPause.Click += new System.EventHandler(this.buttonPause_Click);
            // 
            // trackBarTune
            // 
            this.trackBarTune.Location = new System.Drawing.Point(299, 78);
            this.trackBarTune.Maximum = 100000;
            this.trackBarTune.Minimum = -100000;
            this.trackBarTune.Name = "trackBarTune";
            this.trackBarTune.Size = new System.Drawing.Size(244, 45);
            this.trackBarTune.TabIndex = 4;
            this.trackBarTune.ValueChanged += new System.EventHandler(this.trackBarTune_ValueChanged);
            // 
            // trackBarBfo
            // 
            this.trackBarBfo.Location = new System.Drawing.Point(299, 239);
            this.trackBarBfo.Maximum = 100000;
            this.trackBarBfo.Minimum = -100000;
            this.trackBarBfo.Name = "trackBarBfo";
            this.trackBarBfo.Size = new System.Drawing.Size(244, 45);
            this.trackBarBfo.TabIndex = 5;
            this.trackBarBfo.ValueChanged += new System.EventHandler(this.trackBarBfo_ValueChanged);
            // 
            // labelCenter
            // 
            this.labelCenter.AutoSize = true;
            this.labelCenter.Location = new System.Drawing.Point(406, 116);
            this.labelCenter.Name = "labelCenter";
            this.labelCenter.Size = new System.Drawing.Size(60, 13);
            this.labelCenter.TabIndex = 6;
            this.labelCenter.Text = "labelCenter";
            // 
            // labelBfo
            // 
            this.labelBfo.AutoSize = true;
            this.labelBfo.Location = new System.Drawing.Point(429, 281);
            this.labelBfo.Name = "labelBfo";
            this.labelBfo.Size = new System.Drawing.Size(50, 13);
            this.labelBfo.TabIndex = 7;
            this.labelBfo.Text = "labelBFO";
            // 
            // comboBoxBandwidth
            // 
            this.comboBoxBandwidth.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxBandwidth.FormattingEnabled = true;
            this.comboBoxBandwidth.Location = new System.Drawing.Point(396, 189);
            this.comboBoxBandwidth.Name = "comboBoxBandwidth";
            this.comboBoxBandwidth.Size = new System.Drawing.Size(121, 21);
            this.comboBoxBandwidth.TabIndex = 8;
            this.comboBoxBandwidth.SelectedIndexChanged += new System.EventHandler(this.comboBoxBandwidth_SelectedIndexChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(330, 197);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(60, 13);
            this.label1.TabIndex = 9;
            this.label1.Text = "Bandwidth:";
            // 
            // groupBoxPresets
            // 
            this.groupBoxPresets.Controls.Add(this.radioButtonLSB);
            this.groupBoxPresets.Controls.Add(this.radioButtonUSB);
            this.groupBoxPresets.Controls.Add(this.radioButtonCW);
            this.groupBoxPresets.Location = new System.Drawing.Point(41, 140);
            this.groupBoxPresets.Name = "groupBoxPresets";
            this.groupBoxPresets.Size = new System.Drawing.Size(116, 103);
            this.groupBoxPresets.TabIndex = 10;
            this.groupBoxPresets.TabStop = false;
            this.groupBoxPresets.Text = "Decoding Presets";
            // 
            // radioButtonLSB
            // 
            this.radioButtonLSB.AutoSize = true;
            this.radioButtonLSB.Location = new System.Drawing.Point(44, 79);
            this.radioButtonLSB.Name = "radioButtonLSB";
            this.radioButtonLSB.Size = new System.Drawing.Size(45, 17);
            this.radioButtonLSB.TabIndex = 2;
            this.radioButtonLSB.TabStop = true;
            this.radioButtonLSB.Text = "LSB";
            this.radioButtonLSB.UseVisualStyleBackColor = true;
            this.radioButtonLSB.CheckedChanged += new System.EventHandler(this.radioButtonLSB_CheckedChanged);
            // 
            // radioButtonUSB
            // 
            this.radioButtonUSB.AutoSize = true;
            this.radioButtonUSB.Location = new System.Drawing.Point(44, 49);
            this.radioButtonUSB.Name = "radioButtonUSB";
            this.radioButtonUSB.Size = new System.Drawing.Size(47, 17);
            this.radioButtonUSB.TabIndex = 1;
            this.radioButtonUSB.TabStop = true;
            this.radioButtonUSB.Text = "USB";
            this.radioButtonUSB.UseVisualStyleBackColor = true;
            this.radioButtonUSB.CheckedChanged += new System.EventHandler(this.radioButtonUSB_CheckedChanged);
            // 
            // radioButtonCW
            // 
            this.radioButtonCW.AutoSize = true;
            this.radioButtonCW.Location = new System.Drawing.Point(44, 19);
            this.radioButtonCW.Name = "radioButtonCW";
            this.radioButtonCW.Size = new System.Drawing.Size(43, 17);
            this.radioButtonCW.TabIndex = 0;
            this.radioButtonCW.TabStop = true;
            this.radioButtonCW.Text = "CW";
            this.radioButtonCW.UseVisualStyleBackColor = true;
            this.radioButtonCW.CheckedChanged += new System.EventHandler(this.radioButtonCW_CheckedChanged);
            // 
            // labelDialFreq
            // 
            this.labelDialFreq.AutoSize = true;
            this.labelDialFreq.Location = new System.Drawing.Point(164, 149);
            this.labelDialFreq.Name = "labelDialFreq";
            this.labelDialFreq.Size = new System.Drawing.Size(35, 13);
            this.labelDialFreq.TabIndex = 11;
            this.labelDialFreq.Text = "label2";
            // 
            // labelFromSliceIQ
            // 
            this.labelFromSliceIQ.AutoSize = true;
            this.labelFromSliceIQ.Location = new System.Drawing.Point(153, 17);
            this.labelFromSliceIQ.Name = "labelFromSliceIQ";
            this.labelFromSliceIQ.Size = new System.Drawing.Size(35, 13);
            this.labelFromSliceIQ.TabIndex = 12;
            this.labelFromSliceIQ.Text = "label2";
            // 
            // trackBarPlayPosition
            // 
            this.trackBarPlayPosition.Enabled = false;
            this.trackBarPlayPosition.Location = new System.Drawing.Point(13, 42);
            this.trackBarPlayPosition.Name = "trackBarPlayPosition";
            this.trackBarPlayPosition.Size = new System.Drawing.Size(228, 45);
            this.trackBarPlayPosition.TabIndex = 13;
            this.trackBarPlayPosition.Scroll += new System.EventHandler(this.trackBarPlayPosition_Scroll);
            // 
            // timer1
            // 
            this.timer1.Interval = 1000;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // buttonForward
            // 
            this.buttonForward.Location = new System.Drawing.Point(245, 35);
            this.buttonForward.Name = "buttonForward";
            this.buttonForward.Size = new System.Drawing.Size(37, 23);
            this.buttonForward.TabIndex = 14;
            this.buttonForward.Text = ">>";
            this.buttonForward.UseVisualStyleBackColor = true;
            this.buttonForward.Click += new System.EventHandler(this.buttonForward_Click);
            // 
            // buttonBack
            // 
            this.buttonBack.Location = new System.Drawing.Point(247, 64);
            this.buttonBack.Name = "buttonBack";
            this.buttonBack.Size = new System.Drawing.Size(35, 23);
            this.buttonBack.TabIndex = 15;
            this.buttonBack.Text = "<<";
            this.buttonBack.UseVisualStyleBackColor = true;
            this.buttonBack.Click += new System.EventHandler(this.buttonBack_Click);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(608, 304);
            this.Controls.Add(this.buttonBack);
            this.Controls.Add(this.buttonForward);
            this.Controls.Add(this.trackBarPlayPosition);
            this.Controls.Add(this.labelFromSliceIQ);
            this.Controls.Add(this.labelDialFreq);
            this.Controls.Add(this.groupBoxPresets);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.comboBoxBandwidth);
            this.Controls.Add(this.labelBfo);
            this.Controls.Add(this.labelCenter);
            this.Controls.Add(this.trackBarBfo);
            this.Controls.Add(this.trackBarTune);
            this.Controls.Add(this.buttonPause);
            this.Controls.Add(this.buttonFile);
            this.Controls.Add(this.buttonPlay);
            this.Controls.Add(this.comboBoxWaveOut);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Review Sliced IQ";
            this.Load += new System.EventHandler(this.MainForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.trackBarTune)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBfo)).EndInit();
            this.groupBoxPresets.ResumeLayout(false);
            this.groupBoxPresets.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarPlayPosition)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox comboBoxWaveOut;
        private System.Windows.Forms.Button buttonPlay;
        private System.Windows.Forms.Button buttonFile;
        private System.Windows.Forms.Button buttonPause;
        private System.Windows.Forms.TrackBar trackBarTune;
        private System.Windows.Forms.TrackBar trackBarBfo;
        private System.Windows.Forms.Label labelCenter;
        private System.Windows.Forms.Label labelBfo;
        private System.Windows.Forms.ComboBox comboBoxBandwidth;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBoxPresets;
        private System.Windows.Forms.RadioButton radioButtonLSB;
        private System.Windows.Forms.RadioButton radioButtonUSB;
        private System.Windows.Forms.RadioButton radioButtonCW;
        private System.Windows.Forms.Label labelDialFreq;
        private System.Windows.Forms.Label labelFromSliceIQ;
        private System.Windows.Forms.TrackBar trackBarPlayPosition;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Button buttonForward;
        private System.Windows.Forms.Button buttonBack;
    }
}

