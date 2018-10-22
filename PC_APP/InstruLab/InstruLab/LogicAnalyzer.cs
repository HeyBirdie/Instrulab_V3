using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading;
using System.Timers;
using System.Windows.Forms;
using ZedGraph;

namespace LEO
{
    public partial class LogicAnalyzer : Form
    {
        Device device;

        System.Timers.Timer GUITimer;
        System.Timers.Timer SignalTimer;
        static Semaphore logAnlysSemaphore = new Semaphore(1, 1);
        int semaphoreTimeout = 4000;

        Thread calcSignal_th;
        Measurements meas = new Measurements(5);

        private Queue<Message> logAnlys_q = new Queue<Message>();
        Message message;
        Message.MsgRequest req;
        //        double cntPaint;

        //        LineItem curve_ch1, curve_ch2, curve_ch3, curve_ch4, curve_ch5, curve_ch6, curve_ch7, curve_ch8;
        public GraphPane logAnlysPane;

        double[] timeAxis;
        double[] signal_ch1, signal_ch2, signal_ch3, signal_ch4, signal_ch5, signal_ch6, signal_ch7, signal_ch8;
        ushort[,] signals; //[chann,leng]

        private static string[] logAnlysPins = new string[8];
        private static uint posttrigPeriphClock; // 
        private static uint timeBasePeriphClock;
        private int triggerPointer;

        public enum math_def { NONE, ANALOG };

        math_def math = math_def.NONE;
        math_def last_math = math_def.NONE;

        private double last_sum1 = 0;
        //        private double last_sum2 = 0;
        public uint dataLength = 1000;
        public uint samplingFreq = 10000;
        public uint pretrig = 50; // [%]
        public uint lastPretrig = 50;
        private double realSamplingFreq = 10000;
        private ushort firstGpioPin;
        public uint syncPwmArr = 0;
        public uint syncPwmPsc = 0;

        bool showPoints = false;
        double scale = 1.0;
        double horPosition = 0.5;
        double maxX = 0;
        double minX = 0;

        public enum TRIGGER_MODE { AUTO, NORMAL, SINGLE, };
        TRIGGER_MODE triggerMode;

        double[] testArray = new double[1000];
        ushort[] testArrayRaw = new ushort[1000];
        private double last_maxX;
        private double last_minX;

        //cursors
        private int VerticalCursorSrc = 0;
        private int last_ver_cur_src = 0;
        private double VerticalCursorA = 0;
        private double VerticalCursorB = 0;
        double time_cur_A = 0;
        double time_cur_B = 0;
        double last_tA = 0;
        double last_tB = 0;

        private string timeDif;
        private string timeA;
        private string timeB;
        private string frequency;
        private string UA;
        private string UB;
        private string hexA;
        private string hexB;
        private string binA;
        private string binB;
        private bool last_showPoints;
        private int measChann = 1;

        private bool measValid = false;
        private bool last_measValid = false;
        private double[] signal_math;

        public double LastHorPosition { get; private set; }

        public LogicAnalyzer(Device dev)
        {
            InitializeComponent();
            InitializeZedGraphComponent();
            ShowPinsInComponent();
            /* PC Logic Analyzer app works only if all GPIO pins of LA are on the same port, 
            are next to each other on the port and also the MCU firmware sends pins ascending
            in config request.  */
            getFirstGPIOPin();
            device = dev;

            device.logAnlysCfg.triggerChannel = 1;

            SignalTimer = new System.Timers.Timer(100);
            SignalTimer.Elapsed += new ElapsedEventHandler(Update_signal);
            SignalTimer.Start();

            GUITimer = new System.Timers.Timer(20);
            GUITimer.Elapsed += new ElapsedEventHandler(Update_GUI);
            GUITimer.Start();

            meas.clearMeasurements();
            label_meas1.Text = "";
            label_meas2.Text = "";
            label_meas3.Text = "";
            label_meas4.Text = "";
            label_meas5.Text = "";

            LogAnlys_Init();
            Thread.Sleep(5);
            LogAnlys_Start();
        }

        public void ShowPinsInComponent()
        {
            this.label_color_ch1.Text = "Ch 1 (" + logAnlysPins[0] + ")";
            this.label_color_ch2.Text = "Ch 2 (" + logAnlysPins[1] + ")";
            this.label_color_ch3.Text = "Ch 3 (" + logAnlysPins[2] + ")";
            this.label_color_ch4.Text = "Ch 4 (" + logAnlysPins[3] + ")";
            this.label_color_ch5.Text = "Ch 5 (" + logAnlysPins[4] + ")";
            this.label_color_ch6.Text = "Ch 6 (" + logAnlysPins[5] + ")";
            this.label_color_ch7.Text = "Ch 7 (" + logAnlysPins[6] + ")";
            this.label_color_ch8.Text = "Ch 8 (" + logAnlysPins[7] + ")";
            //this.label_color_ch1.Text = device.logAnlysCfg.pins[0];
        }

        private void InitializeZedGraphComponent()
        {
            logAnlysPane = zedGraphControl_logAnlys.GraphPane;

            logAnlysPane.XAxis.Title.Text = "Time [s]";
            logAnlysPane.XAxis.Title.FontSpec.Size = this.Size.Width / 80;
            logAnlysPane.XAxis.Scale.FontSpec.Size = 10;

            zedGraphControl_logAnlys.MasterPane[0].IsFontsScaled = false;
            zedGraphControl_logAnlys.MasterPane[0].Title.IsVisible = false;

            zedGraphControl_logAnlys.MasterPane[0].XAxis.Title.IsVisible = true;
            zedGraphControl_logAnlys.MasterPane[0].YAxis.Title.IsVisible = false;

            zedGraphControl_logAnlys.MasterPane[0].XAxis.MajorGrid.IsVisible = true;
            zedGraphControl_logAnlys.MasterPane[0].YAxis.MajorGrid.IsVisible = false;

            zedGraphControl_logAnlys.MasterPane[0].XAxis.IsVisible = true;
            zedGraphControl_logAnlys.MasterPane[0].YAxis.IsVisible = false;
        }

        private void Update_signal(object sender, ElapsedEventArgs e)
        {
            double sum = samplingFreq.GetHashCode() + dataLength;

            bool update = false;

            if (sum != last_sum1)
            {
                last_sum1 = sum;
                calculateAndSend_AllParameters();
            }
            else if (pretrig != lastPretrig)
            {
                lastPretrig = pretrig;
                calculateAndSend_PretrigPosttrig();
            }

            if (showPoints != last_showPoints)
            {
                update = true;
                logAnlysPane.CurveList.Clear();
                paint_signals();
                paint_markers();
                paint_cursors();
                vertical_cursor_update();
                last_showPoints = showPoints;
            }


            if (maxX != last_maxX || minX != last_minX)
            {
                logAnlysPane.XAxis.Scale.MaxAuto = false;
                logAnlysPane.XAxis.Scale.MinAuto = false;

                logAnlysPane.XAxis.Scale.Max = maxX;
                logAnlysPane.XAxis.Scale.Min = minX;


                last_maxX = maxX;
                last_minX = minX;

                update = true;

                logAnlysPane.CurveList.Clear();
                paint_signals();
                paint_markers();
                vertical_cursor_update();
                paint_cursors();
                try
                {
                    logAnlysPane.AxisChange();
                }
                catch (Exception ex)
                {
                    //dont need to catch exception -> graph will be updated later
                }
            }
            if (horPosition != LastHorPosition)
            {

                logAnlysPane.YAxis.Scale.MaxAuto = false;
                logAnlysPane.YAxis.Scale.MinAuto = false;


                LastHorPosition = horPosition;
                update = true;
                //process_signals();
                logAnlysPane.CurveList.Clear();
                paint_signals();
                paint_markers();
                vertical_cursor_update();
                paint_cursors();
                try
                {
                    logAnlysPane.AxisChange();
                }
                catch (Exception ex)
                {
                    //dont need to catch exception -> graph will be updated later
                }
            }


            if (last_tA != time_cur_A || last_tB != time_cur_B || last_ver_cur_src != VerticalCursorSrc)
            {
                update = true;
                logAnlysPane.CurveList.Clear();
                paint_signals();
                paint_markers();
                vertical_cursor_update();
                paint_cursors();
                last_tA = time_cur_A;
                last_tB = time_cur_B;
                last_ver_cur_src = VerticalCursorSrc;
            }

            if (last_measValid != measValid)
            {
                update = true;
                meas.calculateMeasurements(signals, 1, 0, 8, (int)(realSamplingFreq), device.logAnlysCfg.samples.Length, 1);
                last_measValid = measValid;
            }


            if (last_math != math)
            {
                update = true;
                logAnlysPane.CurveList.Clear();
                retrieveData();
                paint_signals();
                paint_markers();
                vertical_cursor_update();
                paint_cursors();
                last_math = math;
            }

            if (update)
            {
                this.Invalidate();
            }
        }

        public void vertical_cursor_update()
        {
            if (VerticalCursorSrc != 0)
            {

                // double scale = ((double)device.scopeCfg.ranges[1, selectedRange] - (double)device.scopeCfg.ranges[0, selectedRange]) / 1000 / Math.Pow(2, (double)device.scopeCfg.actualRes);
                // double off = (double)device.scopeCfg.ranges[0, selectedRange] / 1000;

                //vypocet casu
                time_cur_A = VerticalCursorA * maxX + (1 - VerticalCursorA) * minX;
                time_cur_B = VerticalCursorB * maxX + (1 - VerticalCursorB) * minX;
                int indexUA = (int)(time_cur_A / device.logAnlysCfg.maxTime * signal_ch1.Length);
                int indexUB = (int)(time_cur_B / device.logAnlysCfg.maxTime * signal_ch1.Length);

                if (indexUA >= signal_ch1.Length)
                {
                    indexUA = signal_ch1.Length - 1;
                }
                if (indexUB >= signal_ch1.Length)
                {
                    indexUB = signal_ch1.Length - 1;
                }

                int logValueA = 0;
                logValueA += ((int)signal_ch1[indexUA] % 2) == 0 ? 1 : 0;
                logValueA += ((int)signal_ch2[indexUA] % 2) == 0 ? 2 : 0;
                logValueA += ((int)signal_ch3[indexUA] % 2) == 0 ? 4 : 0;
                logValueA += ((int)signal_ch4[indexUA] % 2) == 0 ? 8 : 0;
                logValueA += ((int)signal_ch5[indexUA] % 2) == 0 ? 16 : 0;
                logValueA += ((int)signal_ch6[indexUA] % 2) == 0 ? 32 : 0;
                logValueA += ((int)signal_ch7[indexUA] % 2) == 0 ? 64 : 0;
                logValueA += ((int)signal_ch8[indexUA] % 2) == 0 ? 128 : 0;

                int logValueB = 0;
                logValueB += ((int)signal_ch1[indexUB] % 2) == 0 ? 1 : 0;
                logValueB += ((int)signal_ch2[indexUB] % 2) == 0 ? 2 : 0;
                logValueB += ((int)signal_ch3[indexUB] % 2) == 0 ? 4 : 0;
                logValueB += ((int)signal_ch4[indexUB] % 2) == 0 ? 8 : 0;
                logValueB += ((int)signal_ch5[indexUB] % 2) == 0 ? 16 : 0;
                logValueB += ((int)signal_ch6[indexUB] % 2) == 0 ? 32 : 0;
                logValueB += ((int)signal_ch7[indexUB] % 2) == 0 ? 64 : 0;
                logValueB += ((int)signal_ch8[indexUB] % 2) == 0 ? 128 : 0;


                double td = time_cur_A - time_cur_B;
                double f = Math.Abs(1 / td);

                //formatovani stringu
                if (td >= 1 || td <= -1)
                {
                    this.timeDif = "dt " + (Math.Round(td, 3)).ToString() + " s";
                }
                else if (td >= 0.001 || td <= -0.001)
                {
                    this.timeDif = "dt " + (Math.Round(td * 1000, 3)).ToString() + " ms";
                }
                else
                {
                    this.timeDif = "dt " + (Math.Round(td * 1000000, 3)).ToString() + " us";
                }

                if (time_cur_A >= 1 || time_cur_A <= -1)
                {
                    this.timeA = "t " + (Math.Round(time_cur_A, 3)).ToString() + " s";
                }
                else
                {
                    this.timeA = "t " + (Math.Round(time_cur_A * 1000, 3)).ToString() + " ms";
                }
                if (time_cur_B >= 1 || time_cur_B <= -1)
                {
                    this.timeB = "t " + (Math.Round(time_cur_B, 3)).ToString() + " s";
                }
                else
                {
                    this.timeB = "t " + (Math.Round(time_cur_B * 1000, 3)).ToString() + " ms";
                }
                if (Double.IsInfinity(f))
                {
                    this.frequency = "f Inf";
                }
                else if (f >= 1000000)
                {
                    this.frequency = "f " + (Math.Round(f / 1000000, 3)).ToString() + " MHz";
                }
                else if (f >= 1000)
                {
                    this.frequency = "f " + (Math.Round(f / 1000, 3)).ToString() + " kHz";
                }
                else
                {
                    this.frequency = "f " + (Math.Round(f, 3)).ToString() + " Hz";
                }


                this.UA = "Dec " + logValueA.ToString();
                this.UB = "Dec " + logValueB.ToString();

                this.hexA = "Hex 0x" + logValueA.ToString("X");
                this.hexB = "Hex 0x" + logValueB.ToString("X");

                this.binA = "Bin b" + Convert.ToString(logValueA, 2);
                this.binB = "Bin b" + Convert.ToString(logValueA, 2);


            }
        }

        public void paint_cursors()
        {

            if (VerticalCursorSrc != 0)
            {
                Color col = Color.Gray;
                LineItem curve;
                PointPairList list1 = new PointPairList();

                list1 = new PointPairList();
                list1.Add(time_cur_A, 0);
                list1.Add(time_cur_A, 18);
                curve = logAnlysPane.AddCurve("", list1, col, SymbolType.HDash);
                curve.Symbol.Size = 0;
                curve.Line.IsSmooth = true;


                list1 = new PointPairList();
                list1.Add(time_cur_B, 0);
                list1.Add(time_cur_B, 18);
                curve = logAnlysPane.AddCurve("", list1, col, SymbolType.HDash);
                curve.Symbol.Size = 0;
                curve.Line.IsSmooth = true;
            }


        }

        public void LogAnlys_Init()
        {
            sendCommand(Commands.LOG_ANLYS_INIT);
        }

        public void LogAnlys_Deinit()
        {
            sendCommand(Commands.LOG_ANLYS_DEINIT);
        }

        public void LogAnlys_Start()
        {
            sendCommand(Commands.LOG_ANLYS_START);
        }

        public void LogAnlys_Stop()
        {
            sendCommand(Commands.LOG_ANLYS_STOP);
        }

        void logAnlys_next()
        {
            LogAnlys_Start();
        }

        private void LogicAnalyzer_FormClosing(object sender, FormClosingEventArgs e)
        {
            device.takeCommsSemaphore(semaphoreTimeout + 110);
            device.send(Commands.LOG_ANLYS + ":" + Commands.LOG_ANLYS_STOP + ";");
            device.send(Commands.LOG_ANLYS + ":" + Commands.LOG_ANLYS_DEINIT + ";");
            device.giveCommsSemaphore();
        }

        void calculateAndSend_AllParameters()
        {
            /* Send number of samples to be taken */
            sendCommandNumber(Commands.LOG_ANLYS_SAMPLES_NUM, dataLength);
            /* Calculate and send sampling frequency */
            realSamplingFreq = processFrequency(samplingFreq, timeBasePeriphClock);
            sendCommandNumber(Commands.LOG_ANLYS_SAMPLING_FREQ, make32BitFromArrPsc((ushort)(syncPwmArr - 1), (ushort)(syncPwmPsc - 1)));
            /* Set pretrigger and posttrigger */
            calculateAndSend_PretrigPosttrig();
        }

        void calculateAndSend_PretrigPosttrig()
        {
            double samplingTime = dataLength / (double)samplingFreq;
            /* Calculate pretrigger in milliseconds */
            uint pretriggerTime = (uint)Math.Round(samplingTime * pretrig / 100 * 1000);
            double posttriggerFreq = 1 / (samplingTime * (1 - pretrig / (double)100));
            processFrequency(posttriggerFreq, posttrigPeriphClock);

            /* Send pretrigger */
            sendCommandNumber(Commands.LOG_ANLYS_PRETRIG, pretriggerTime);
            /* Send posttrigger */
            sendCommandNumber(Commands.LOG_ANLYS_POSTTRIG, make32BitFromArrPsc((ushort)(syncPwmArr - 1), (ushort)(syncPwmPsc - 1)));
        }

        public void sendCommand(string generalComm)
        {
            device.takeCommsSemaphore(semaphoreTimeout + 110);
            device.send(Commands.LOG_ANLYS + ":" + generalComm + ";");
            device.giveCommsSemaphore();
        }

        public void sendCommand(string generalComm, string specificComm)
        {
            device.takeCommsSemaphore(semaphoreTimeout + 110);
            device.send(Commands.LOG_ANLYS + ":" + generalComm + " ");
            device.send(specificComm + ";");
            device.giveCommsSemaphore();
        }

        public void sendCommandNumber(string generalComm, uint number)
        {
            device.takeCommsSemaphore(semaphoreTimeout + 110);
            device.send(Commands.LOG_ANLYS + ":" + generalComm + " ");
            device.send_int((int)number);
            device.send(";");
            device.giveCommsSemaphore();
        }

        public static void setPins(string[] pins)
        {
            for (int i = 0; i < pins.Length; i++)
            {
                logAnlysPins[i] = pins[i].Replace("_", "");
            }
        }

        public void getFirstGPIOPin()
        {
            string pinNumber = Regex.Match(logAnlysPins[0], @"\d+").Value;

            try
            {
                firstGpioPin = ushort.Parse(pinNumber);
            }
            catch (Exception ex)
            {
                device.logRecieved("Logic Analyzer - first GPIO pin not parsed.");
                device.logTextNL(ex.ToString());
            }
        }

        public static void setPosttrigPeriphClock(uint periphClock)
        {
            posttrigPeriphClock = periphClock;
        }

        public static void setTimeBasePeriphClock(uint periphClock)
        {
            timeBasePeriphClock = periphClock;
        }

        private void Update_GUI(object sender, ElapsedEventArgs e)
        {
            if (logAnlys_q.Count > 0)
            {
                message = logAnlys_q.Dequeue();
                if (message == null)
                {
                    return;
                }
                switch (req = message.GetRequest())
                {
                    case Message.MsgRequest.LOG_ANLYS_TRIGGER_POINTER:
                        triggerPointer = message.GetNum();
                        break;
                    case Message.MsgRequest.LOG_ANLYS_DATA:
                        retrieveData();
                        if (calcSignal_th != null && calcSignal_th.IsAlive)
                        {
                            calcSignal_th.Join();
                        }
                        calcSignal_th = new Thread(() => meas.calculateMeasurements(signals, 1, 0, 8, (int)(realSamplingFreq), device.logAnlysCfg.samples.Length, 1));
                        calcSignal_th.Start();
                        this.Invalidate();
                        break;
                }
            }
        }

        public void add_message(Message msg)
        {
            this.logAnlys_q.Enqueue(msg);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            try
            {
                if (req == Message.MsgRequest.LOG_ANLYS_DATA)
                {
                    /* There might be a data coming from MCU after MCU was stopped. This is to prevent asking for another data pack.*/
                    if (checkBox_trig_single.Text != "Single")
                    {
                        label_logAnlys_status.ForeColor = (label_logAnlys_status.ForeColor == Color.Red) ? Color.Blue : Color.Red;
                        label_logAnlys_status.Text = "Trig";
                        paint_signals();
                        paint_markers();
                        paint_cursors();
                        vertical_cursor_update();

                        if (calcSignal_th != null && calcSignal_th.IsAlive)
                        {
                            calcSignal_th.Join();
                        }


                        switch (triggerMode)
                        {
                            case TRIGGER_MODE.SINGLE:
                                checkBox_trig_single.Text = "Single";
                                break;
                            case TRIGGER_MODE.AUTO:
                                logAnlys_next();
                                Debug.WriteLine("AUTO_DATA_QUEST");
                                break;
                            case TRIGGER_MODE.NORMAL:
                                logAnlys_next();
                                Debug.WriteLine("NORMAL_DATA_QUEST");
                                break;
                        }
                    }
                }

                if (VerticalCursorSrc != 0)
                {
                    this.label_cur_freq.Text = this.frequency;
                    this.label_cur_time_a.Text = this.timeA;
                    this.label_cur_time_b.Text = this.timeB;
                    this.label_cur_ua.Text = this.UA;
                    this.label_cur_ub.Text = this.UB;
                    this.label_cur_hexa.Text = this.hexA;
                    this.label_cur_hexb.Text = this.hexB;
                    this.label_cur_bina.Text = this.binA;
                    this.label_cur_binb.Text = this.binB;
                    this.label_time_diff.Text = this.timeDif;

                }

                if (meas.getMeasCount() > 0)
                {
                    for (int i = 0; i < meas.getMeasCount(); i++)
                    {
                        switch (i)
                        {
                            case 0:
                                this.label_meas1.Text = meas.getMeas(0);
                                this.label_meas1.ForeColor = meas.getColor(0);
                                break;
                            case 1:
                                this.label_meas2.Text = meas.getMeas(1);
                                this.label_meas2.ForeColor = meas.getColor(1);
                                break;
                            case 2:
                                this.label_meas3.Text = meas.getMeas(2);
                                this.label_meas3.ForeColor = meas.getColor(2);
                                break;
                            case 3:
                                this.label_meas4.Text = meas.getMeas(3);
                                this.label_meas4.ForeColor = meas.getColor(3);
                                break;
                            case 4:
                                this.label_meas5.Text = meas.getMeas(4);
                                this.label_meas5.ForeColor = meas.getColor(4);
                                break;
                        }
                    }
                }

                base.OnPaint(e);
                req = Message.MsgRequest.NULL_MESSAGE;
            }
            catch (Exception ex)
            {
                device.logTextNL(ex.ToString());
                device.logTextNL(Environment.StackTrace.ToString());
                this.Close();
                throw new System.ArgumentException("Logic Analyzer painting went wrong");
            }

        }


        public void paint_signals()
        {
            logAnlysPane.CurveList.Clear();

            if (math != math_def.NONE)
            {
                LineItem logAnlysCurve;
                logAnlysCurve = logAnlysPane.AddCurve("", timeAxis, signal_math, Color.Gray, SymbolType.Diamond);
                logAnlysCurve.Line.IsSmooth = false;
                logAnlysCurve.Line.SmoothTension = 0.5F; ;
                logAnlysCurve.Line.IsAntiAlias = true;
                logAnlysCurve.Line.IsOptimizedDraw = true;
                logAnlysCurve.Symbol.Size = showPoints ? 5 : 0;
            }
            else
            {
                paint_one_signal(timeAxis, signal_ch1, 1, Color.Red);
                paint_one_signal(timeAxis, signal_ch2, 2, Color.Blue);
                paint_one_signal(timeAxis, signal_ch3, 3, Color.Green);
                paint_one_signal(timeAxis, signal_ch4, 4, Color.Magenta);
                paint_one_signal(timeAxis, signal_ch5, 5, Color.Black);
                paint_one_signal(timeAxis, signal_ch6, 6, Color.DarkOrange);
                paint_one_signal(timeAxis, signal_ch7, 7, Color.DarkTurquoise);
                paint_one_signal(timeAxis, signal_ch8, 8, Color.Maroon);
            }



            //set axis
            logAnlysPane.XAxis.MajorGrid.IsVisible = true;

            logAnlysPane.YAxis.Scale.MaxAuto = false;
            logAnlysPane.YAxis.Scale.MinAuto = false;

            logAnlysPane.YAxis.Scale.Max = 18;
            logAnlysPane.YAxis.Scale.Min = 0;

            logAnlysPane.XAxis.Scale.MaxAuto = false;
            logAnlysPane.XAxis.Scale.MinAuto = false;
            logAnlysPane.XAxis.Scale.Max = maxX;
            logAnlysPane.XAxis.Scale.Min = minX;

            //update
            try
            {
                zedGraphControl_logAnlys.AxisChange();
                zedGraphControl_logAnlys.Invalidate();
            }
            catch (Exception ex)
            {
                //dont need to catch exception -> graph will be updated later
            }
        }

        public void paint_markers()
        {

            //trigger time
            PointPairList list1 = new PointPairList();
            LineItem curve;
            Color col;

            list1 = new PointPairList();

            list1.Add(((device.logAnlysCfg.maxTime) * (pretrig) / 100 - (device.logAnlysCfg.maxTime / (device.logAnlysCfg.samples.Length)) + 1 / realSamplingFreq), 17.5 - device.logAnlysCfg.triggerChannel * 2);

            switch (device.logAnlysCfg.triggerChannel)
            {
                case 1:
                    col = Color.Red;
                    break;
                case 2:
                    col = Color.Blue;
                    break;
                case 3:
                    col = Color.Green;
                    break;
                case 4:
                    col = Color.Black;
                    break;
                case 5:
                    col = Color.Magenta;
                    break;
                case 6:
                    col = Color.DarkOrange;
                    break;
                case 7:
                    col = Color.Indigo;
                    break;
                case 8:
                    col = Color.Maroon;
                    break;
                default:
                    col = Color.Gray;
                    break;
            }

            if (this.checkBox_trig_rise.Checked)
            {
                curve = logAnlysPane.AddCurve("", list1, col, SymbolType.Triangle);
            }
            else
            {
                curve = logAnlysPane.AddCurve("", list1, col, SymbolType.TriangleDown);
            }

            curve.Symbol.Size = 8;
            curve.Symbol.Fill.Color = col;
            curve.Symbol.Fill.IsVisible = true;


            //zoom position
            list1 = new PointPairList();
            list1.Add((device.logAnlysCfg.maxTime) * horPosition, 18);
            curve = logAnlysPane.AddCurve("", list1, Color.Red, SymbolType.TriangleDown);
            curve.Symbol.Size = 15;
            curve.Symbol.Fill.Color = Color.Red;
            curve.Symbol.Fill.IsVisible = true;


        }

        public void paint_one_signal(double[] timeAxis, double[] valueAxis, uint channel, Color color)
        {
            LineItem logAnlysCurve;

            logAnlysCurve = logAnlysPane.AddCurve("", timeAxis, valueAxis, color, SymbolType.Diamond);
            logAnlysCurve.Line.StepType = StepType.ForwardStep;
            logAnlysCurve.Line.IsSmooth = false;
            logAnlysCurve.Line.Width = 1.5F;
            logAnlysCurve.Line.IsAntiAlias = true;
            logAnlysCurve.Line.IsOptimizedDraw = true;
            logAnlysCurve.Symbol.Size = showPoints ? 5 : 0; ;

        }

        void retrieveData()
        {
            uint length = (uint)device.logAnlysCfg.samples.Length;

            device.logAnlysCfg.maxTime = 1 / realSamplingFreq * (device.logAnlysCfg.samples.Length);

            timeAxis = timAxis(new double[length]);
            update_X_axe();

            signal_ch1 = valueAxis(new double[length], 8);
            testArray = signal_ch1;
            signal_ch2 = valueAxis(new double[length], 7);
            testArray = signal_ch2;
            signal_ch3 = valueAxis(new double[length], 6);
            testArray = signal_ch3;
            signal_ch4 = valueAxis(new double[length], 5);
            testArray = signal_ch4;
            signal_ch5 = valueAxis(new double[length], 4);
            testArray = signal_ch5;
            signal_ch6 = valueAxis(new double[length], 3);
            testArray = signal_ch6;
            signal_ch7 = valueAxis(new double[length], 2);
            testArray = signal_ch7;
            signal_ch8 = valueAxis(new double[length], 1);
            testArray = signal_ch8;

            signals = new ushort[8, length];

            for (int i = 0; i < length; i++)
            {
                signals[0, i] = (ushort)(signal_ch1[i] % 2 == 0 ? 1 : 0);
                signals[1, i] = (ushort)(signal_ch2[i] % 2 == 0 ? 1 : 0);
                signals[2, i] = (ushort)(signal_ch3[i] % 2 == 0 ? 1 : 0);
                signals[3, i] = (ushort)(signal_ch4[i] % 2 == 0 ? 1 : 0);
                signals[4, i] = (ushort)(signal_ch5[i] % 2 == 0 ? 1 : 0);
                signals[5, i] = (ushort)(signal_ch6[i] % 2 == 0 ? 1 : 0);
                signals[6, i] = (ushort)(signal_ch7[i] % 2 == 0 ? 1 : 0);
                signals[7, i] = (ushort)(signal_ch8[i] % 2 == 0 ? 1 : 0);
            }


            if (math != math_def.NONE)
            {
                this.signal_math = new double[length];

                switch (math)
                {
                    case math_def.ANALOG:
                        for (int i = 0; i < length; i++)
                        {
                            signal_math[i] = 0;

                            for (int j = 0; j < 8; j++)
                            {
                                signal_math[i] += signals[j, i] * Math.Pow(2, j);
                            }
                            signal_math[i] = signal_math[i] / 16;
                        }
                        break;
                }
            }
        }

        public double[] valueAxis(double[] array, uint channel)
        {
            //ushort chan = (ushort)(channel - 1);
            ushort chan = (ushort)(8 - channel);

            ushort[] tempArray = new ushort[array.Length];
            tempArray = device.logAnlysCfg.samples;

            ushort[] tempArray2 = new ushort[array.Length];


            for (int i = 0; i < array.Length; i++)
            {
                tempArray2[i] = tempArray[(i - triggerPointer + array.Length + array.Length - (int)(pretrig / (double)100 * array.Length)) % array.Length];
            }

            tempArray = tempArray2;


            /* Extract zeroes and ones of required GPIO pin from received buffer. */
            for (int j = 0; j < array.Length; j++)
            {
                /* Set the n-th element to predefined level for graph painting. */
                //array[j] = ((tempArray[j] & (ushort)(1 << (firstGpioPin + chan))) == 0) ? ((channel - 1) * 2) + 1 : ((channel - 1) * 2) + 2;
                array[j] = ((tempArray[j] & (ushort)(1 << (firstGpioPin + chan))) == 0) ? ((channel - 1) * 2) + 1 : ((channel - 1) * 2) + 2;
            }

            return array;
        }

        public double[] timAxis(double[] array)
        {
            double samplingPeriod;
            double tempSamplPeriod = samplingPeriod = 1 / realSamplingFreq;

            array[0] = 0;
            for (int j = 1; j < array.Length; j++)
            {
                array[j] = tempSamplPeriod;
                tempSamplPeriod += samplingPeriod;
            }

            return array;
        }

        private double processFrequency(double freq, uint periphClock)
        {
            UInt32 psc = 0, arr = 1;  // MCU TIM Prescaler and Auto Reload Register
            UInt32 arrMultipliedByPsc = 0;
            arrMultipliedByPsc = (uint)Math.Round(periphClock / freq);

            if (arrMultipliedByPsc <= 65536)
            {
                arr = arrMultipliedByPsc;
                psc = 1;
            }
            else
            {
                /* Test how the inserted frequency can be devided to set ARR and PSC registers. */
                for (; psc == 0; arrMultipliedByPsc--)
                {
                    for (UInt32 pscTemp = 65536; pscTemp > 1; pscTemp--)
                    {
                        if ((arrMultipliedByPsc % pscTemp) == 0)
                        {
                            if (arrMultipliedByPsc / pscTemp <= 65536)
                            {
                                psc = pscTemp;
                                break;
                            }
                        }
                    }

                    if (psc != 0)
                    {
                        if (arrMultipliedByPsc / psc <= 65536)
                        {
                            break;
                        }
                    }
                }

                arr = arrMultipliedByPsc / psc;

                if (arr < psc)
                {
                    UInt32 swapVar = arr;
                    arr = psc;
                    psc = swapVar;
                }
            }

            syncPwmArr = (ushort)arr;
            syncPwmPsc = (ushort)psc;

            double realPwmFreq = periphClock / (double)(arr * psc);
            return realPwmFreq;
        }

        public uint make32BitFromArrPsc(ushort arr, ushort psc)
        {
            uint arrPsc = 0;
            arrPsc = (uint)(psc << 16) | arr;

            return arrPsc;
        }

        private static bool isPrimeNumber(UInt32 number)
        {
            if ((number & 1) == 0)
            {
                return (number == 2) ? true : false;
            }

            for (int i = 3; (i * i) <= number; i = i + 2)
            {
                if ((number % i) == 0)
                {
                    return false;
                }
            }
            return true;
        }

        /* ------------------------------------------------------------------------------------------------------------------ */
        /* ---------------------------------------- LOGIC ANALYZER EVENTS HANDLERS ------------------------------------------ */
        /* ------------------------------------------------------------------------------------------------------------------ */
        private void trackBar_pretrig_Scroll(object sender, EventArgs e)
        {
            pretrig = (uint)trackBar_pretrig.Value;
            this.maskedTextBox_pretrig.Text = pretrig.ToString();
        }

        private void maskedTextBox_pretrig_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == Convert.ToChar(Keys.Enter))
            {
                validate_pretrigger();
            }
        }

        private void maskedTextBox_pretrig_Leave(object sender, EventArgs e)
        {
            validate_pretrigger();
        }

        private void validate_pretrigger()
        {
            /* In Auto trigger mode the trigger may not be received. In that case its value is
               given by user settings as the data is sent to PC app after a period. Moreover, triggerPointer
               value represents CNDTR register of DMA in the time of trigger, i.e. pointer is decreasing
               from initial value. */
            triggerPointer = (int)dataLength - (int)dataLength * (int)(trackBar_pretrig.Value / (double)100);

            try
            {
                ushort val = ushort.Parse(this.maskedTextBox_pretrig.Text);
                if (val > 100)
                {
                    throw new System.ArgumentException("Parameter cannot be greather then 100", "original");
                }
                this.trackBar_pretrig.Value = (int)(val);
                pretrig = val;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.maskedTextBox_pretrig.Text = pretrig.ToString();
            }
        }

        public void update_X_axe()
        {
            double maxTime = 1 / realSamplingFreq * device.logAnlysCfg.samples.Length;
            double interval = scale * maxTime;
            double posmin = (interval / 2);
            double posScale = (maxTime - interval) / maxTime;
            maxX = (double)(maxTime) * horPosition * posScale + posmin + interval / 2;
            minX = (double)(maxTime) * horPosition * posScale + posmin - interval / 2;

        }




        /* ------------------------------------------------------------------------------------------------------------------ */
        /* ---------------------------------------- below just callbacks from GUI ------------------------------------------ */
        /* ------------------------------------------------------------------------------------------------------------------ */

        private void radioButton_trig_ch1_CheckedChanged(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 1);
            device.logAnlysCfg.triggerChannel = 1;
        }

        private void radioButton_trig_ch2_CheckedChanged(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 2);
            device.logAnlysCfg.triggerChannel = 2;
        }

        private void radioButton_trig_ch3_CheckedChanged(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 3);
            device.logAnlysCfg.triggerChannel = 3;
        }

        private void radioButton_trig_ch4_Click(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 4);
            device.logAnlysCfg.triggerChannel = 4;
        }

        private void radioButton_trig_c5_Click(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 5);
            device.logAnlysCfg.triggerChannel = 5;
        }

        private void radioButton_trig_ch6_Click(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 6);
            device.logAnlysCfg.triggerChannel = 6;
        }

        private void radioButton_trig_ch7_Click(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 7);
            device.logAnlysCfg.triggerChannel = 7;
        }

        private void radioButton_trig_ch8_Click(object sender, EventArgs e)
        {
            sendCommandNumber(Commands.LOG_ANLYS_TRIGGER_CHANNEL, 8);
            device.logAnlysCfg.triggerChannel = 8;
        }

        private void checkBox_trig_rise_CheckedChanged(object sender, EventArgs e)
        {
            sendCommand(Commands.LOG_ANLYS_TRIGGER_EVENT, Commands.LOG_ANLYS_TRIGGER_EDGE_RISING);
            checkBox_trig_rise.Checked = true;
            checkBox_trig_fall.Checked = false;
        }

        private void checkBox_trig_fall_CheckedChanged(object sender, EventArgs e)
        {
            sendCommand(Commands.LOG_ANLYS_TRIGGER_EVENT, Commands.LOG_ANLYS_TRIGGER_EDGE_FALLING);
            checkBox_trig_fall.Checked = true;
            checkBox_trig_rise.Checked = false;
        }

        private void checkBox_trig_single_CheckedChanged(object sender, EventArgs e)
        {
            checkBox_trig_single.Checked = true;
            if (checkBox_trig_single.Text.Equals("Stop"))
            {
                LogAnlys_Stop();
                checkBox_trig_single.Text = "Single";
                label_logAnlys_status.Text = "";
            }
            else if (checkBox_trig_single.Text.Equals("Single"))
            {
                triggerMode = TRIGGER_MODE.SINGLE;
                sendCommand(Commands.LOG_ANLYS_TRIGGER_MODE, Commands.LOG_ANLYS_TRIGGER_MODE_SINGLE);
                logAnlys_next();
                Debug.WriteLine("SINGLE_CLICK_QUEST");
                checkBox_trig_single.Text = "Stop";
                label_logAnlys_status.Text = "Wait";
            }
            this.checkBox_trig_auto.Checked = false;
            this.checkBox_trig_normal.Checked = false;
        }

        private void checkBox_trig_normal_CheckedChanged(object sender, EventArgs e)
        {
            if (this.checkBox_trig_normal.Checked)
            {
                triggerMode = TRIGGER_MODE.NORMAL;
                sendCommand(Commands.LOG_ANLYS_TRIGGER_MODE, Commands.LOG_ANLYS_TRIGGER_MODE_NORMAL);                
                Debug.WriteLine("NORMAL_CLICK_QUEST");
                this.checkBox_trig_auto.Checked = false;
                this.checkBox_trig_single.Checked = false;
                this.checkBox_trig_single.Text = "Stop";
                label_logAnlys_status.Text = "";
            }
        }

        private void checkBox_trig_auto_CheckedChanged(object sender, EventArgs e)
        {
            if (this.checkBox_trig_auto.Checked)
            {
                triggerMode = TRIGGER_MODE.AUTO;
                sendCommand(Commands.LOG_ANLYS_TRIGGER_MODE, Commands.LOG_ANLYS_TRIGGER_MODE_AUTO);                 
                Debug.WriteLine("AUTO_CLICK_QUEST");
                this.checkBox_trig_normal.Checked = false;
                this.checkBox_trig_single.Checked = false;
                this.checkBox_trig_single.Text = "Stop";
                label_logAnlys_status.Text = "";
            }
        }

        /* ---------------------------------------- DATA LENGTH EVENTS HANDLERS ------------------------------------------ */
        private void radioButton_100_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_100.Checked)
            {
                dataLength = 100;
            }
        }

        private void radioButton_200_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_200.Checked)
            {
                dataLength = 200;
            }
        }

        private void radioButton_500_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_500.Checked)
            {
                dataLength = 500;
            }
        }

        private void radioButton_1x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_1x.Checked)
            {
                dataLength = 1000;
            }
        }

        private void radioButton_2x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_2x.Checked)
            {
                dataLength = 2000;
            }
        }

        private void radioButton_5x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_5x.Checked)
            {
                dataLength = 5000;
            }
        }

        private void radioButton_10x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_10x.Checked)
            {
                dataLength = 10000;
            }
        }

        private void radioButton_20x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_20x.Checked)
            {
                dataLength = 20000;
            }
        }

        private void radioButton_50x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_50x.Checked)
            {
                dataLength = 50000;
            }
        }

        private void radioButton_100x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_100x.Checked)
            {
                dataLength = 100000;
            }
        }

        private void radioButton_200x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_200x.Checked)
            {
                dataLength = 200000;
            }
        }

        private void radioButton_500x_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_500x.Checked)
            {
                dataLength = 500000;
            }
        }

        private void button_max_possible_Click(object sender, EventArgs e)
        {
            radioButton_500x.Checked = true;
        }

        private void showPointsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            showPoints = this.showPointsToolStripMenuItem.Checked;
        }

        private void trackBar_zoom_ValueChanged(object sender, EventArgs e)
        {
            scale = 1.0 - (double)(this.trackBar_zoom.Value) / (this.trackBar_zoom.Maximum - this.trackBar_zoom.Minimum + 10);
            update_X_axe();
        }

        private void trackBar_position_ValueChanged(object sender, EventArgs e)
        {
            horPosition = (double)(this.trackBar_position.Value) / (this.trackBar_position.Maximum - this.trackBar_position.Minimum);
            update_X_axe();
        }

        private void radioButton_ver_cur_off_CheckedChanged(object sender, EventArgs e)
        {
            if (this.radioButton_ver_cur_off.Checked)
            {
                VerticalCursorSrc = 0;
                vertical_cursor_update();
                validate_vertical_curr();
            }
        }

        private void radioButton_ver_cur_ch1_CheckedChanged(object sender, EventArgs e)
        {
            if (this.radioButton_ver_cur_ch1.Checked)
            {
                VerticalCursorSrc = 1;
                vertical_cursor_update();
                validate_vertical_curr();
            }
        }

        public void validate_vertical_curr()
        {
            this.trackBar_ver_cur_a.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.trackBar_ver_cur_b.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_freq.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_time_a.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_time_b.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_ua.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_ub.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_hexa.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_hexb.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_bina.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_cur_binb.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label_time_diff.Enabled = VerticalCursorSrc == 0 ? false : true;
            //this.label_ver_cur_du.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label5.Enabled = VerticalCursorSrc == 0 ? false : true;
            this.label6.Enabled = VerticalCursorSrc == 0 ? false : true;
            if (VerticalCursorSrc == 0)
            {
                time_cur_A = 0;
                time_cur_B = 0;
            }
        }

        private void trackBar_ver_cur_a_ValueChanged(object sender, EventArgs e)
        {
            VerticalCursorA = (double)(this.trackBar_ver_cur_a.Value) / (this.trackBar_ver_cur_a.Maximum - this.trackBar_ver_cur_a.Minimum);
            vertical_cursor_update();
        }

        private void trackBar_ver_cur_b_ValueChanged(object sender, EventArgs e)
        {
            VerticalCursorB = (double)(this.trackBar_ver_cur_b.Value) / (this.trackBar_ver_cur_b.Maximum - this.trackBar_ver_cur_b.Minimum);
            vertical_cursor_update();
        }

        private void clearAllToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.clearMeasurements();
            this.label_meas1.Text = "";
            this.label_meas2.Text = "";
            this.label_meas3.Text = "";
            this.label_meas4.Text = "";
            this.label_meas5.Text = "";
        }

        private void toolStripMenuItem_ch1_meas_Click(object sender, EventArgs e)
        {
            measChann = 1;
            validate_meas_chann();
        }

        private void toolStripMenuItem_ch2_meas_Click(object sender, EventArgs e)
        {
            measChann = 2;
            validate_meas_chann();
        }

        private void toolStripMenuItem_ch3_meas_Click(object sender, EventArgs e)
        {
            measChann = 3;
            validate_meas_chann();
        }

        private void toolStripMenuItem_ch4_meas_Click(object sender, EventArgs e)
        {
            measChann = 4;
            validate_meas_chann();
        }

        private void channel5ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            measChann = 5;
            validate_meas_chann();
        }

        private void channel6ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            measChann = 6;
            validate_meas_chann();
        }

        private void channel7ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            measChann = 7;
            validate_meas_chann();
        }

        private void validate_meas_chann()
        {
            this.toolStripMenuItem_ch1_meas.Checked = measChann == 1 ? true : false;
            this.toolStripMenuItem_ch2_meas.Checked = measChann == 2 ? true : false;
            this.toolStripMenuItem_ch3_meas.Checked = measChann == 3 ? true : false;
            this.toolStripMenuItem_ch4_meas.Checked = measChann == 4 ? true : false;
            this.toolStripMenuItem_ch5_meas.Checked = measChann == 5 ? true : false;
            this.toolStripMenuItem_ch6_meas.Checked = measChann == 6 ? true : false;
            this.toolStripMenuItem_ch7_meas.Checked = measChann == 7 ? true : false;
            this.toolStripMenuItem_ch8_meas.Checked = measChann == 8 ? true : false;
        }

        private void channel8ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            measChann = 8;
            validate_meas_chann();
        }

        private void frequencyToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.FREQUENCY);
            measValid = !measValid;
        }

        private void periodToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.PERIOD);
            measValid = !measValid;
        }

        private void phaseToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.PHASE);
            measValid = !measValid;
        }

        private void dutyToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.DUTY);
            measValid = !measValid;
        }

        private void highToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.HIGH);
            measValid = !measValid;
        }

        private void lowToolStripMenuItem_Click(object sender, EventArgs e)
        {
            meas.addMeasurement(measChann - 1, Measurements.MeasurementTypes.LOW);
            measValid = !measValid;
        }

        private void AnalogToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (AnalogToolStripMenuItem.Checked)
            {
                math = math_def.ANALOG;
            }
            else
            {
                math = math_def.NONE;
            }
            AnalogToolStripMenuItem.Checked = math == math_def.ANALOG ? true : false;
        }

        private void toolStripMenuItem5_Click(object sender, EventArgs e)
        {

            StreamWriter signalWriter;
            SaveFileDialog saveSignal = new SaveFileDialog();

            // Set filter options and filter index.
            saveSignal.Filter = "CSV Files (.csv)|*.csv|Text Files (.txt)|*.txt|All Files (*.*)|*.*";
            saveSignal.FilterIndex = 1;
            saveSignal.FileName = "signal_1";

            // Call the ShowDialog method to show the dialog box.
            bool done = false;
            while (!done)
            {
                DialogResult userClickedOK = saveSignal.ShowDialog();
                if (userClickedOK.Equals(DialogResult.OK))
                {
                    if (File.Exists(saveSignal.FileName))
                    {
                        try
                        {
                            File.Delete(saveSignal.FileName);
                            done = true;
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show("Cannot overwrite selected file \r\n", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            saveSignal.FileName = "signal_1";
                            continue;
                        }

                    }
                    else
                    {
                        done = true;
                    }
                }
                else
                {
                    done = true;
                }
            }



            signalWriter = File.AppendText(saveSignal.FileName);
            string tmp;
            char separator = ';';


            signalWriter.WriteLine("time" + separator + "signal1" + separator + "signal2" + separator + "signal3" + separator + "signal4" + separator + "signal5" + separator + "signal6" + separator + "signal7" + separator + "signal8");


            for (int i = 0; i < signal_ch1.Length; i++)
            {

                tmp = device.scopeCfg.timeBase[i].ToString();



                tmp += separator + signals[0, i].ToString();
                tmp += separator + signals[1, i].ToString();
                tmp += separator + signals[2, i].ToString();
                tmp += separator + signals[3, i].ToString();
                tmp += separator + signals[4, i].ToString();
                tmp += separator + signals[5, i].ToString();
                tmp += separator + signals[6, i].ToString();
                tmp += separator + signals[7, i].ToString();

                signalWriter.WriteLine(tmp);
            }

            signalWriter.Close();

        }

        private void saveSignalToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.zedGraphControl_logAnlys.SaveAsBitmap();
        }

        private void exitOscilloscopeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        /* ------------------------------ TIME BASE (SAMPLING FREQUENCY) EVENTS HANDLERS -------------------------------- */
        private void radioButton_1k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_1k.Checked)
            {
                samplingFreq = 1000;
                this.label_freq.Text = "1 kS";
            }
        }

        private void radioButton_2k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_2k.Checked)
            {
                samplingFreq = 2000;
                this.label_freq.Text = "2 kS";
            }
        }

        private void radioButton_5k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_5k.Checked)
            {
                samplingFreq = 5000;
                this.label_freq.Text = "5 kS";
            }
        }

        private void radioButton_10k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_10k.Checked)
            {
                samplingFreq = 10000;
                this.label_freq.Text = "10 kS";
            }
        }

        private void radioButton_20k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_20k.Checked)
            {
                samplingFreq = 20000;
                this.label_freq.Text = "20 kS";
            }
        }

        private void radioButton_50k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_50k.Checked)
            {
                samplingFreq = 50000;
                this.label_freq.Text = "50 kS";
            }
        }

        private void radioButton_100k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_100k.Checked)
            {
                samplingFreq = 100000;
                this.label_freq.Text = "100 kS";
            }
        }

        private void radioButton_200k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_200k.Checked)
            {
                samplingFreq = 200000;
                this.label_freq.Text = "200 kS";
            }
        }

        private void radioButton_500k_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_500k.Checked)
            {
                samplingFreq = 500000;
                this.label_freq.Text = "500 kS";
            }
        }

        private void radioButton_1m_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_1m.Checked)
            {
                samplingFreq = 1000000;
                this.label_freq.Text = "1 MS";
            }
        }

        private void radioButton_2m_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_2m.Checked)
            {
                samplingFreq = 2000000;
                this.label_freq.Text = "2 MS";
            }
        }

        private void radioButton_5m_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_5m.Checked)
            {
                samplingFreq = 5000000;
                this.label_freq.Text = "5 MS";
            }
        }

        private void radioButton_10m_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_10m.Checked)
            {
                samplingFreq = 10000000;
                this.label_freq.Text = "10 MS";
            }
        }

        private void radioButton_20m_CheckedChanged(object sender, EventArgs e)
        {
            if (radioButton_20m.Checked)
            {
                samplingFreq = 20000000;
                this.label_freq.Text = "20 MS";
            }
        }
    }
}

