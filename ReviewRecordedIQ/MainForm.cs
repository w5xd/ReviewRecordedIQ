using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ReviewRecordedIQ
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private XD.WaveDeviceTx deviceTx = null;
        private int selectedDevice = 0;
        private void disposeTx()
        {
            if (null != deviceTx)
                deviceTx.Dispose();
            deviceTx = null;
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            var outDevices = XD.WaveDeviceEnumerator.waveOutDevices();
            foreach (string d in outDevices)
                comboBoxWaveOut.Items.Add(d);
            foreach (object o in System.Enum.GetNames((typeof(XDSdr.SdrDecodeBandwidth))))
                comboBoxBandwidth.Items.Add(o);
            comboBoxWaveOut.SelectedIndex = 0;
            comboBoxBandwidth.SelectedIndex = comboBoxBandwidth.Items.Count - 1;
            labelCenter.Text = "";
            labelBfo.Text = "";
            labelFromSliceIQ.Text = "";
            buttonPause.Enabled = buttonPlay.Enabled;
            radioButtonUSB.Checked = true;
        }
        XDSdr.SimpleSDR sdr = null;
 
        private void comboBoxWaveOut_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBoxWaveOut.SelectedIndex != selectedDevice)
            {
                disposeTx();
                selectedDevice = comboBoxWaveOut.SelectedIndex;
                buttonPlay.Enabled = selectedDevice >= 0;
                buttonPause.Enabled = buttonPlay.Enabled;
            }
        }

        private void buttonFile_Click(object sender, EventArgs e)
        {
            var fd = new OpenFileDialog();
            fd.Title = "Select .WAV file from SliceIQ";
            fd.Filter = "Wave Files (*.wav)|*.wav";
            if (fd.ShowDialog() == DialogResult.OK)
            {
                SliceIQCenterKHz = 0;
                disposeTx();
                deviceTx = new XD.WaveDeviceTx();
                deviceTx.SamplesPerSecond = 12000;
                deviceTx.ThrottleSource = true;
                deviceTx.Open((uint)selectedDevice, 2);

                sdr = new XDSdr.SimpleSDR(fd.FileName, deviceTx.GetRealTimeAudioSink());
                buttonPlay.Enabled = true;
                buttonPause.Enabled = buttonPlay.Enabled;
                var maxHz = sdr.IfBoundaryAbsHz;
                trackBarBfo.Minimum = (int)-maxHz;
                trackBarBfo.Maximum = (int)maxHz;
                trackBarTune.Minimum = (int)-maxHz;
                trackBarTune.Maximum = (int)maxHz;

                sdr.RxFrequencyCenterHz = trackBarTune.Value;
                sdr.RxFrequencyBfoOffsetHz = trackBarBfo.Value;
                sdr.Bandwidth = (XDSdr.SdrDecodeBandwidth)(comboBoxBandwidth.SelectedIndex);

                labelCenter.Text = String.Format("Center decode = {0} KHz", sdr.RxFrequencyCenterHz * .001);
                labelBfo.Text = String.Format("Weaver decode = {0} KHz", sdr.RxFrequencyBfoOffsetHz * .001);

                var fromSliceIQ = sdr.FromSliceIQ;
                labelFromSliceIQ.Text = fromSliceIQ;
                string outTag = "--outputCenterKHz=";
                var centerText = fromSliceIQ.IndexOf(outTag);
                if (centerText != -1)
                {
                    var equal = fromSliceIQ.IndexOf("=", centerText);
                    var endCenter = fromSliceIQ.IndexOf(" ", centerText);
                    if (endCenter < 0)
                        endCenter = fromSliceIQ.Length;
                    string toConvert = fromSliceIQ.Substring(equal + 1, endCenter - equal - 1);
                    SliceIQCenterKHz = Double.Parse(toConvert);
                }
                string timeTag = "--outputStartTime=";
                var startTimeText = fromSliceIQ.IndexOf(timeTag);
                if (startTimeText != -1)
                {
                    var equal = fromSliceIQ.IndexOf("=", startTimeText);
                    var endTag = fromSliceIQ.IndexOf(" ", startTimeText);
                    if (endTag < 0)
                        endTag = fromSliceIQ.Length;
                    SliceIQTimeTag = fromSliceIQ.Substring(equal + 1, endTag - equal - 1);
                }
                trackBarPlayPosition.Minimum = 0;
                trackBarPlayPosition.Maximum = 1 + (int)sdr.PlayLengthSeconds;
                trackBarPlayPosition.Enabled = false;
                UpdateDialFrequency();
            }
        }

        private double SliceIQCenterKHz = 0;
        private string SliceIQTimeTag;

        private void buttonPlay_Click(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.Play();
                trackBarPlayPosition.Enabled = false;
                timer1.Enabled = true;
            }
        }

        private void buttonPause_Click(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.Pause();
                trackBarPlayPosition.Enabled = true;
                timer1.Enabled = false;
            }
        }

        private void comboBoxBandwidth_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (sdr != null)
                sdr.Bandwidth = (XDSdr.SdrDecodeBandwidth)(comboBoxBandwidth.SelectedIndex);
            UpdateDialFrequency();
        }


        private void trackBarBfo_ValueChanged(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.RxFrequencyBfoOffsetHz = trackBarBfo.Value;
                labelBfo.Text = String.Format("Weaver shift = {0} KHz", sdr.RxFrequencyBfoOffsetHz * .001);
            }
            UpdateDialFrequency();
        }

        private void trackBarTune_ValueChanged(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.RxFrequencyCenterHz = trackBarTune.Value;
            }
            UpdateDialFrequency();
        }

        private void radioButtonUSB_CheckedChanged(object sender, EventArgs e)
        {
            trackBarBfo.Value = 1100;
            comboBoxBandwidth.SelectedIndex = 3;
        }

        private void radioButtonCW_CheckedChanged(object sender, EventArgs e)
        {
            trackBarBfo.Value = 500;
            comboBoxBandwidth.SelectedIndex = 1;
        }

        private void radioButtonLSB_CheckedChanged(object sender, EventArgs e)
        {
            trackBarBfo.Value = -1100;
            comboBoxBandwidth.SelectedIndex = 3;
        }

        private double tuneFrequencyKHz(int bfo = 0)
        {
            return SliceIQCenterKHz + (trackBarTune.Value + bfo) * .001;
        }

        private void UpdateDialFrequency()
        {
            labelCenter.Text = String.Format("Center of filter = {0} KHz", SliceIQCenterKHz + trackBarTune.Value * .001);
            if (radioButtonCW.Checked)
            {
                labelDialFreq.Text = String.Format("CW at {0} KHz, Pitch={1} ({2})",
                    tuneFrequencyKHz(), Math.Abs(trackBarBfo.Value),
                    trackBarBfo.Value > 0 ? "USB" : "LSB");
            }
            else if (radioButtonLSB.Checked)
            {
                labelDialFreq.Text = String.Format("LSB at {0} KHz", tuneFrequencyKHz(-trackBarBfo.Value));
            }
            else if (radioButtonUSB.Checked)
            {
                labelDialFreq.Text = String.Format("USB at {0} KHz", tuneFrequencyKHz(-trackBarBfo.Value));
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                trackBarPlayPosition.Value = (int)sdr.PlayPositionSeconds;
            }
        }

        private void trackBarPlayPosition_Scroll(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.PlayPositionSeconds = trackBarPlayPosition.Value;
            }
        }

        private void buttonForward_Click(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.PlayPositionSeconds += 5;
            }
        }

        private void buttonBack_Click(object sender, EventArgs e)
        {
            if (sdr != null)
            {
                sdr.PlayPositionSeconds -= 5;
            }
        }
    }
}
