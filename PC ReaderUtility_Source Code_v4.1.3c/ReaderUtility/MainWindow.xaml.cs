using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Text.RegularExpressions;
using System.Windows.Threading;
using System.Threading;
using System.Collections;
using System.IO.Ports;
using System.IO;
using System.Diagnostics;
using System.Globalization;
using System.ComponentModel;
using RFID.Utility.VM;
using RFID.Utility.IClass;
using System.Collections.ObjectModel;
using log4net;
using log4net.Config;
using RFID.Service;
using RFID.Service.IInterface.COM;
using RFID.Service.IInterface.NET;
using RFID.Service.IInterface.USB;
using RFID.Service.IInterface.BLE;
using System.Linq;
using Microsoft.Win32;
using static RFID.Utility.EditCommandDialog;
using RFID.Service.IInterface.COM.Events;
using RFID.Service.IInterface.USB.Events;
using RFID.Service.IInterface.NET.Events;

using System.Resources;
using System.Net.Sockets;
using System.Xml.XPath;
using System.Reflection;
using RFID.Service.IInterface.BLE.Events;

namespace RFID.Utility
{
    public partial class MainWindow : Window, IDisposable {

		enum ValidationStates { DEFAULF, OK, ERROR, WARNING, FOCUS };

		public enum CommandStatus : Int32 {
            REGULATION,
            DEFAULT, EPC, TID, SELECT, INFO,
            PASSWORD, READ, WRITE, LOCK, KILL,//MULTI, 
            CUSTOM, CUSTOMEM,
            GPIOCONFIG,
            GPIOCONFIG10C, GPIOCONFIG10UC,
            GPIOCONFIG11C, GPIOCONFIG11UC,
            GPIOCONFIG14C, GPIOCONFIG14UC,
            GPIOPINS,
            GPIOGETPINS,
            GPIOPIN10C, GPIOPIN10UC,
            GPIOPIN11C, GPIOPIN11UC,
            GPIOPIN14C, GPIOPIN14UC,

            B02T, B02P,
            B02URSLOTQ, B02UR, B02USLOTQ, B02U,
            B02QR, B02Q,

            B02ITEM02R, B02ITEM02W, B02ITEM02P, B02ITEM02T, B02ITEM02L,
            B02ITEM02URSLOTQ, B02ITEM02UR, B02ITEM02USLOTQ, B02ITEM02U,
            B02ITEM02QR, B02ITEM02Q, B02ITEM02CUSTOMIZE,

            B03SELECT, B03WRITE, B03READ,
            B03GET, B03TAGRUN,

            B04GPIOPINS,
            B04ANTENNA01, B04ANTENNA02, B04ANTENNA03, B04ANTENNA04,
        };

        /// <summary>
        /// 
        /// </summary>
		enum GroupStatus {
            ALL, BORDER_PAGES, BORDER_CULTURE, BORDER_LIGHT, BORDER_SET,
            GB01PRESET_SELECT, 
            GB01GPIO,
            GB01MSG,
            GB01CUSTOM,
            GB02PRESET_SELECT, GB02PRESET_ACCESS, GB02READCTRL,
            GB02BTN_U, GB02SlotQ, GB02BTN_Q,
            GB02BTN_CLR, GB02BTN_SAVE,
            GB02PRESET_CONTINUE_U, GB02PRESET_CONTINUE_Q,
            GB02TABCONTROLITEM, GB02TABBTN,

            GB03, GB04, GB04ReadCtrl, GB04SlotQ, GB04AntennaCtrl
        };

		private CommandStatus					DoProcess, DoFakeProcess, DoOldProcess;
		private ReaderService					ReaderService = null;
        private ICOM                            _ICOM;
        private INET                            _INet;
        private INETInfo                        _INetInfo;
        private IUSB                            _IUSB;
        private IBLE                            _IBLE;
        private ICOM.CombineDataEventHandler    _CombineDataHandler;
        private INET.NetTCPDataEventHandler     _NetTCPDataHandler;
        private IUSB.USBDataEventHandler        _USBDataHandler;
        private IBLE.BLEDataEventHandler        _BLEDataHandler;
        private ReaderModule.Version	        _VersionFW;
        private ReaderModule.BaudRate           _BaudRate = ReaderModule.BaudRate.B38400;
        private ReaderService.ConnectType       _ConnectType = ReaderService.ConnectType.DEFAULT;
        private ConnectDialog					_ConnectDialog = null;
		private RegulationDialog				_RegulationDialog = null;
        private EditCommandDialog               _EditCommandDialog = null;
        private SerialPort						_SerialPort;

        //Menu, Common
        private Hashtable                       ValidationState = new Hashtable();
        private CultureInfo                     Culture;
        private List<UIControl>                 UIControPackets = new List<UIControl>();
        private List<UIControl>                 UITempControPackets = new List<UIControl>();
        private Boolean                         IsReceiveDataWork = false;// false to receive data is completed, otherwise is true 
        private Boolean                         IsMenuRecordMode = false;
        private Boolean                         IsReceiveSubDataWork = false;
        private Boolean                         IsDateTimeStamp = false;
        private Boolean                         IsFocus = false;
        private Int32                           TabCtrlIndex = 0;

        //Page01
        private Thread                          SettingThread;
        private Int32                           MaskField, ActionField;
        private Int32                           GPIOConfigurMask = 0;
        private Boolean                         IsGetGPIOConfiguration10 = false,
                                                IsGetGPIOConfiguration11 = false,
                                                IsGetGPIOConfiguration14 = false;
        private Boolean                         IsGetGPIOPin10 = false,
                                                IsGetGPIOPin11 = false,
                                                IsGetGPIOPin14 = false;


        //Page02
        public ObservableCollection<B02Item02Command> B02Item02Commands { get; private set; }
        private readonly DispatcherTimer        B02Item02Process = new DispatcherTimer();
        private List<CommandStatus>             B02Item02ProcessItem = new List<CommandStatus>();
        private List<String>                    B02Item02CommandSets = new List<String>();
        private Int32                           B02Item02ProcessCounts = 0, 
                                                B02Item02ProcessIdx = 0,
                                                B02Item02SelectIdx = 0,
                                                B02TabCtrlIndex = 0,
                                                B02ListViewRunCount = 0,
                                                B02ListViewTagCount = 0;  
		private Boolean                         IsB02Repeat = false;
        private Boolean                         IsB02ThreadRunCtrl = false;
        private Boolean                         IsB02Item02OnBtnClick = false;
        private DateTime                        B02Item02CommandStartTime;
        private double                          B02Item02CommandRunTimesCount = 0.0d;
        private static ILog                     B02MessageLogger;

        //Page03
        private String                          EMTemp;
        private Int32                           EMFlashTime = 0;
        private Boolean                         IsB03EMWork = false;
        private Boolean                         IsB03EMVoltTempWork = false;
        private Thread                          B03Thread;
        private System.Timers.Timer             B03LightTimeEvent = new System.Timers.Timer();


        //Page04
        private Byte                            B04AntennaItems = 0, B04AntennaItemsTemp = 0;
        private Int32                           B04AntennaRunIndex = 0, B04AntennaLoopIndex = 0;
        private readonly DispatcherTimer        B04AntennaTestStart = new DispatcherTimer();
        private readonly DispatcherTimer        B04Process = new DispatcherTimer();
        private readonly DispatcherTimer        B04ProcessDelay = new DispatcherTimer();
        private List<CommandStatus>             B04ProcessItem = new List<CommandStatus>();
        private Boolean                         IsB04AntennaSetPinWork = false; 
        private Boolean                         IsB04ChangeAndRun = false;// false to deal with antenna change, otherwise is antenna test run
        private Boolean                         IsB04RepeatRunEnd = false;
        private Byte                            B04AntennaTempData = 0x0;
        private Int32                           B04AntennaDelayTimes = 0;
        private Int32                           B04AntennaDelayTimesIdx = 0;
        private List<Int32>                     B04AntennaTargetRunTimes = new List<Int32>();
        private Int32                           B04AntennaRunCount = 0;
        private Int32                           B04AntennaTagIncreaseCount = 0;
        private Boolean                         IsOnB04BtnAntennaClick = false;
        private Boolean                         IsB04BtnAntennaRun = false;


        private static ILog                     RawLogger, FragmentSummaryLogger;
        
        private XmlFormat                       ProfileXml;
        private String                          ProfileXmlName;
        private MainWindowVM                    VM = new MainWindowVM();

        private Boolean                         IsRunning = false;

        private ResourceManager                 stringManager;

        class UIControl
        {
			public GroupStatus Group { get; set; }
			public Boolean Status { get; set; }

			public UIControl(GroupStatus g, Boolean s) { Group = g; Status = s; }
		}



		public MainWindow()
        {
			InitializeComponent();

            stringManager =
            new ResourceManager("en-US", Assembly.GetExecutingAssembly());

            VM.B01GroupPreSetSelectMemBank = VM.MemBank[1];
            VM.B01GroupRWComboBoxMemBank = VM.MemBank[1];
            VM.B01GroupGPIOComboBoxConfigur = VM.GPIOConfig[0];
            VM.B01GroupGPIOComboBoxPins = VM.GPIOConfig[0];

            VM.B02GroupPreSetSelectMemBank = VM.MemBank[1];
            VM.B02GroupPreSetReadMemBank = VM.MemBank[1];
            VM.B02GroupUSlotQComboBox = VM.SlotQ[4];
            DataContext = VM;
            this.Culture = new CultureInfo("en-US", false);

            this.ReaderService = new ReaderService();
			this._ConnectDialog = new ConnectDialog();
			this._ConnectDialog.ShowDialog();

			if (_ConnectDialog.DialogResult.HasValue && !_ConnectDialog.DialogResult.Value)
                this.Close();
			else if (_ConnectDialog.DialogResult.HasValue && _ConnectDialog.DialogResult.Value)
            {
                VM.BorderCheckBoxStatusTag = "True";

                this.ReaderService = _ConnectDialog.GetService();

                switch (this._ConnectDialog.GetIType())
                {
                    case ReaderService.ConnectType.COM:
                        this._ConnectType = ReaderService.ConnectType.COM;
                        this._ICOM = this._ConnectDialog.GetICOM();
                        this._SerialPort = this._ICOM.GetSerialPort();
                        this._BaudRate = ICOM.GetBaudRate(this._SerialPort.BaudRate);
                        this._CombineDataHandler = new ICOM.CombineDataEventHandler(DoReceiveDataWork);
                        this._ICOM.CombineDataReceiveEventHandler += this._CombineDataHandler;
                        this.ReaderService.COM = this._ICOM;
                        VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0} ({1},{2},{3},{4})",
                                                                    this._SerialPort.PortName,
                                                                    this._SerialPort.BaudRate,
                                                                    this._SerialPort.DataBits,
                                                                    this._SerialPort.Parity,
                                                                    this._SerialPort.StopBits);
                        break;
                    case ReaderService.ConnectType.NET:
                        this._ConnectType = ReaderService.ConnectType.NET;
                        this._INet = this._ConnectDialog.GetINET();
                        this._NetTCPDataHandler = new INET.NetTCPDataEventHandler(DoReceiveDataWork);
                        this._INet.NetTCPDataReceiveEventHandler += _NetTCPDataHandler;
                        this.ReaderService.NET = this._INet;
                        VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}:{1}", this._INet.IP(), this._INet.Port());

                        _INetInfo = new INETInfo
                        {
                            IP = this._INet.IP(),
                            Port = this._INet.Port()
                        };
                        break;
                    case ReaderService.ConnectType.USB:
                        this._ConnectType = ReaderService.ConnectType.USB;
                        this._IUSB = _ConnectDialog.GetIUSB();
                        this._USBDataHandler = new IUSB.USBDataEventHandler(DoReceiveDataWork);
                        this._IUSB.USBDataReceiveEvent += this._USBDataHandler;
                        this.ReaderService.USB = this._IUSB;
                        VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}, VID:{1},PID:{2}", this._IUSB.ProductName, this._IUSB.VendorId, this._IUSB.ProductID);
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._ConnectType = ReaderService.ConnectType.BLE;
                        this._IBLE = _ConnectDialog.GetIBLE();
                        this._BLEDataHandler = new IBLE.BLEDataEventHandler(DoReceiveDataWork);
                        this._IBLE.BLEDataReceiveEvent += this._BLEDataHandler;
                        this.ReaderService.BLE = this._IBLE;
                        VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}/ {1}", this._IBLE.DeviceName, this._IBLE.DeviceId);
                        break;
                } 

                this.BorderComboBoxCulture.SelectedIndex = 0;


                this.SettingThread = new Thread(DoInfoWork) {
                    IsBackground = true
                };
                this.SettingThread.Start();

				((INotifyCollectionChanged)B02ListBox.Items).CollectionChanged += B02ListBoxCollectionChanged;
                ((INotifyCollectionChanged)B03ListBox.Items).CollectionChanged += B03ListBoxCollectionChanged;
            }
            this._ConnectDialog.Close();
            //pager01
            InitializeB01();
            //pager02
            InitializeB02();
            //pager04
            InitializeB04();
        }





        #region === #Interface Function ===
        /// <summary>
        /// item enable or disable
        /// </summary>
        /// <param name="g"></param>
        /// <param name="b"></param>
        private void GroupStatusControl(GroupStatus g, Boolean b)
        {
            switch (g)
            {
                case GroupStatus.BORDER_PAGES: VM.BorderSelectedPageIsEnabled = b; break;
                case GroupStatus.BORDER_CULTURE: VM.BorderComboBoxCultureIsEnabled = b; break;
                case GroupStatus.BORDER_LIGHT: VM.BorderCheckBoxStatusIsEnabled = b; break;
                case GroupStatus.BORDER_SET: VM.BorderSettingButtonIsEnabled = b; break;
                case GroupStatus.GB01PRESET_SELECT: VM.B01GroupPreSetSelectIsEnabled = b; break;
                case GroupStatus.GB01GPIO: VM.B01GroupGPIOIsEnabled = b; break;
                case GroupStatus.GB01MSG: VM.B01GroupMsgIsEnabled = b; break;
                case GroupStatus.GB01CUSTOM: VM.B01GroupMsgPopIsEnabled = b; break;

                case GroupStatus.GB02PRESET_SELECT: VM.B02GroupPreSetSelectIsEnabled = b; break;
                case GroupStatus.GB02PRESET_ACCESS: VM.B02GroupPreSetAccessIsEnabled = b; break;
                case GroupStatus.GB02READCTRL: VM.B02GroupReadCtrlIsEnabled = b; break;
                case GroupStatus.GB02PRESET_CONTINUE_U: VM.B02GroupPreSetRepeatUIsEnabled = b; break;
                case GroupStatus.GB02PRESET_CONTINUE_Q: VM.B02GroupPreSetRepeatQIsEnabled = b; break;
                case GroupStatus.GB02TABCONTROLITEM: VM.B02TabControlItemIsEnabled = b; break;
                case GroupStatus.GB02TABBTN: VM.B02Item02BtnIsEnabled = b; VM.B02Item02BtnOpenIsEnabled = b; VM.B02Item02BtnSaveIsEnabled = b; break;
                case GroupStatus.GB02SlotQ: VM.B02GroupUSlotQIsEnabled = b; break;
                case GroupStatus.GB02BTN_U: VM.B02GroupUButtonIsEnabled = b; break;
                case GroupStatus.GB02BTN_Q: VM.B02GroupQButtonIsEnabled = b; break;
                case GroupStatus.GB02BTN_CLR: VM.B02ButtonClearIsEnabled = b; break;
                case GroupStatus.GB02BTN_SAVE: VM.B02ButtonSaveIsEnabled = b; break;
                case GroupStatus.GB03: VM.B03IsEnabled = b; break;
                case GroupStatus.GB04: VM.B04IsEnabled = b; break;
                case GroupStatus.GB04ReadCtrl: VM.B04GroupReadCtrlIsEnabled = b; break;
                case GroupStatus.GB04SlotQ: VM.B04GroupUSlotQIsEnabled = b; break;
                case GroupStatus.GB04AntennaCtrl:
                    VM.B04GroupAntennaCtrlIsEnabled = b;
                    break;


                case GroupStatus.ALL:
                    VM.BorderComboBoxCultureIsEnabled = b;
                    VM.BorderSelectedPageIsEnabled = b;
                    VM.BorderSettingButtonIsEnabled = b;

                    VM.B01GroupPreSetSelectIsEnabled = b;
                    VM.B01GroupEPCIsEnabled = b;
                    VM.B01GroupRWIsEnabled = b;
                    VM.B01GroupLockIsEnabled = b;
                    VM.B01GroupKillIsEnabled = b;
                    VM.B01GroupGPIOIsEnabled = b;
                    VM.B01GroupMsgIsEnabled = b;
                    VM.B01GroupMsgPopIsEnabled = b;

                    VM.B02GroupPreSetSelectIsEnabled = b;
                    VM.B02GroupPreSetAccessIsEnabled = b;
                    VM.B02GroupReadCtrlIsEnabled = b;

                    VM.B02GroupUSlotQIsEnabled = b;
                    VM.B02GroupPreSetRepeatUIsEnabled = b;
                    VM.B02GroupPreSetRepeatQIsEnabled = b;
                    VM.B02TabControlItemIsEnabled = b;
                    VM.B02Item02BtnIsEnabled = b;
                    VM.B02Item02BtnOpenIsEnabled = b;
                    VM.B02Item02BtnSaveIsEnabled = b;
                    VM.B02GroupUButtonIsEnabled = b;
                    VM.B02GroupQButtonIsEnabled = b;
                    VM.B02ButtonClearIsEnabled = b;
                    VM.B02ButtonSaveIsEnabled = b;

                    VM.B03GroupTagWindowButtonGetIsEnabled = b;
                    VM.B03GroupTagWindowButtonRunIsEnabled = b;
                    VM.B03GroupTagWindowButtonBattAlarmTempIsEnabled = b;
                    VM.B03GroupTagWindowButtonBattVoltIsEnabled = b;

                    VM.B03IsEnabled = b;
                    VM.B04IsEnabled = b;
                    VM.B04GroupAntennaCtrlIsEnabled = b;
                    VM.B04GroupReadCtrlIsEnabled = b;
                    VM.B04GroupUSlotQIsEnabled = b;
                    break;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="list"></param>
        /// <param name="b"></param>
		private void UIControlStatus(List<UIControl> list, Boolean b)
        {
            for (Int32 i = 0; i < list.Count; i++)
            {
                if (b) GroupStatusControl(list[i].Group, list[i].Status);
                else GroupStatusControl(list[i].Group, !list[i].Status);
            }
        }

        /// <summary>
        /// command code error message
        /// </summary>
        /// <param name="process"></param>
        /// <param name="s"></param>
		private void ErrorCodeCheck(CommandStatus process, String s)
        {
            switch (process)
            {
                case CommandStatus.EPC: break;
                case CommandStatus.TID: break;
                //case CommandStatus.MULTI: break;
                case CommandStatus.READ:
                case CommandStatus.KILL:
                case CommandStatus.LOCK:
                case CommandStatus.WRITE:
                case CommandStatus.B02URSLOTQ:
                case CommandStatus.B02UR:
                case CommandStatus.B02USLOTQ:
                case CommandStatus.B02U:
                    if (String.IsNullOrEmpty(s)) break;
                    if (s == "0") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "0: other error." : "0: 其他未知的錯誤", true);
                    else if (s == "3") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "3: The specified memory location does not exist or the EPC length field is not supported." : "3: 寫入的記憶體位置不存在或內容長度超出範圍", true);
                    else if (s == "4") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "4: The specified memory location is locked and/or per-malocked." : "4: 此標籤記憶體已鎖住或永久鎖住", true);
                    else if (s == "B") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "B: The Tag has insufficient power." : "B: 標籤Power不足，無法進行寫入操作", true);
                    else if (s == "F") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "F: Non-specific error." : "F: 此標籤不支援錯誤規範碼(Error-specific)", true);
                    else if (s[0] == 'Z') MessageShow((this.Culture.IetfLanguageTag == "en-US") ? String.Format(CultureInfo.CurrentCulture, "{0}: {1} chars written to the memory.", s, s.Substring(1, 2)) : String.Format(CultureInfo.CurrentCulture, "{0}: {1} 字元已寫入", s, s.Substring(1, 2)), true);
                    else if (s == "W") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "W: The Tag does not exist or multiple-tags interference or no reply." : "W: 在Reader作用範圍內沒有標籤或標籤干擾", false);
                    else if (s == "L") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "L: The Tag does not exist or multiple-tags interference or no reply." : "L: 在Reader作用範圍內沒有標籤或標籤干擾", false);
                    else if (s == "K") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "K: The Tag does not exist or multiple-tags interference or no reply." : "K: 在Reader作用範圍內沒有標籤或標籤干擾", false);
                    else if (s == "E") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "E: Error." : "E: 執行指令過程標籤回覆錯誤", true);
                    else if (s == "X") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "X: Command error or not support." : "X: 傳送指令格式錯誤或未支援", true);
                    else if (s == "L<OK>") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "L<OK>: The Tag has been successfully lock." : "L<OK>: 標籤Lock成功", false);
                    else if (s == "W<OK>") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "W<OK>: The Tag has been successfully written." : "W<OK>: 標籤寫入成功", false);
                    else if (s.Length >= 2 && s.Substring(0, 2) == "3Z") MessageShow((this.Culture.IetfLanguageTag == "en-US") ? String.Format(CultureInfo.CurrentCulture, "{0}: error code and {1} chars has been written.", s, s.Substring(2, 2)) : String.Format(CultureInfo.CurrentCulture, "{0}: 錯誤碼且 {1} 字元已寫入", s, s.Substring(2, 2)), true);
                    else MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "N/A: Not applicable." : "N/A: 不適用此版本Utility", false);
                    break;
                case CommandStatus.DEFAULT:
                default: break;
            }
        }

        /// <summary>
        /// Reveice combine data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void ReceiveDataWork(ReadStatus status, String s_crlf)
        {
            Byte[] b_crlf;

            if (status == ReadStatus.WaitTimedOut)
            {
                IsReceiveDataWork = false;
                return;
            }

            if (status != ReadStatus.Success)
            {
                switch (_ConnectType)
                {
                    case ReaderService.ConnectType.COM:
                        this._ICOM.Close();
                        break;
                    case ReaderService.ConnectType.NET:
                        this._INet.Close();
                        break;
                    case ReaderService.ConnectType.USB:
                        this._IUSB.Close();
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._IBLE.Close();
                        break;
                }

                this.IsB02ThreadRunCtrl = false;
                this.IsReceiveDataWork = false;


                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? s_crlf : s_crlf, false);
                GroupStatusControl(GroupStatus.ALL, false);

                VM.BorderCheckBoxStatusTag = "False";
                VM.BorderTextBlockStatus = String.Empty;
                VM.BorderFirmwareVersion = String.Empty;
                VM.BorderTBReaderID = String.Empty;
                DoProcess = CommandStatus.DEFAULT;

                var _thread = new Thread(new ThreadStart(() =>
                {
                    Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                    {
                        if (this._RegulationDialog != null && this._RegulationDialog.IsActive)
                            this._RegulationDialog.Close();
                        if (this._EditCommandDialog != null && this._EditCommandDialog.IsActive)
                        {
                            this.B02Item02Commands = _EditCommandDialog.GetSequences();
                            this.B02Item02ListBox.Items.Refresh();
                            this._EditCommandDialog.Close();
                        }

                        OpenConnectDialog();
                    }));
                }))
                {
                    IsBackground = true
                };
                _thread.Start();
                return;
            }

            switch (DoProcess)
            {
                case CommandStatus.CUSTOM:
                case CommandStatus.SELECT:
                case CommandStatus.PASSWORD:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    
                    break;
                case CommandStatus.EPC:
                    VM.B01GroupEPCTextBoxEPC = Format.RemoveCRLFandTarget(s_crlf, 'Q');
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.TID:
                    VM.B01GroupEPCTextBoxTID = Format.RemoveCRLFandTarget(s_crlf, 'R');
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.READ:
                    VM.B01GroupRWTextBoxRead = Format.RemoveCRLFandTarget(s_crlf, 'R');
                    ErrorCodeCheck(CommandStatus.READ, (s_crlf.IndexOf('R') != -1) ? String.Empty : VM.B01GroupRWTextBoxRead);
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.WRITE:
                case CommandStatus.LOCK:
                case CommandStatus.KILL:
                    ErrorCodeCheck(CommandStatus.KILL, Format.RemoveCRLF(s_crlf));
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.GPIOCONFIG:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    
                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf != null && b_crlf.Length == 5)
                    {
                        Int32 val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        Int32 mask = GPIOConfigurMask;
                        if (mask != val)
                        {
                            if (((val ^ mask) & 0x04) == 0x04)
                            {
                                IsGetGPIOConfiguration10 = true;
                                if ((val & (byte)0x04) > 0) VM.B01GroupGPIOCheckBoxConfigur10IsChecked = true;
                                else VM.B01GroupGPIOCheckBoxConfigur10IsChecked = false;
                            }

                            if (((val ^ mask) & 0x02) == 0x02)
                            {
                                IsGetGPIOConfiguration11 = true;
                                if ((val & (byte)0x02) > 0) VM.B01GroupGPIOCheckBoxConfigur11IsChecked = true;
                                else VM.B01GroupGPIOCheckBoxConfigur11IsChecked = false;
                            }

                            if (((val ^ mask) & 0x01) == 0x01)
                            {
                                IsGetGPIOConfiguration14 = true;
                                if ((val & (byte)0x01) > 0) VM.B01GroupGPIOCheckBoxConfigur14IsChecked = true;
                                else VM.B01GroupGPIOCheckBoxConfigur14IsChecked = false;
                            }
                        }
                    }
                    else
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Set GPIO configuration error." : "設定GPIO configuration失敗", false);
                    }
                    break;
                case CommandStatus.GPIOCONFIG10C:
                case CommandStatus.GPIOCONFIG10UC:
                case CommandStatus.GPIOCONFIG11C:
                case CommandStatus.GPIOCONFIG11UC:
                case CommandStatus.GPIOCONFIG14C:
                case CommandStatus.GPIOCONFIG14UC:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;

                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf.Length == 5)
                    {
                        Int32 val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        VM.B01GroupGPIOComboBoxConfigur = VM.GPIOConfig[val];
                    }
                    break;
                case CommandStatus.GPIOPINS:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf != null && b_crlf.Length == 5)
                    {
                        int val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        if ((val & (byte)0x04) > 0)
                        {
                            if (!VM.B01GroupGPIOCheckBoxPin10IsChecked)
                            {
                                IsGetGPIOPin10 = true;
                                VM.B01GroupGPIOCheckBoxPin10IsChecked = true;
                            }
                        }
                        else
                        {
                            if (VM.B01GroupGPIOCheckBoxPin10IsChecked)
                            {
                                IsGetGPIOPin10 = true;
                                VM.B01GroupGPIOCheckBoxPin10IsChecked = false;
                            }
                        }
                        if ((val & (byte)0x02) > 0)
                        {
                            if (!VM.B01GroupGPIOCheckBoxPin11IsChecked)
                            {
                                IsGetGPIOPin11 = true;
                                VM.B01GroupGPIOCheckBoxPin11IsChecked = true;
                            }
                        }
                        else
                        {
                            if (VM.B01GroupGPIOCheckBoxPin11IsChecked)
                            {
                                IsGetGPIOPin11 = true;
                                VM.B01GroupGPIOCheckBoxPin11IsChecked = false;
                            }
                        }
                        if ((val & (byte)0x01) > 0)
                        {
                            if (!VM.B01GroupGPIOCheckBoxPin14IsChecked)
                            {
                                IsGetGPIOPin14 = true;
                                VM.B01GroupGPIOCheckBoxPin14IsChecked = true;
                            }
                        }
                        else
                        {
                            if (VM.B01GroupGPIOCheckBoxPin14IsChecked)
                            {
                                IsGetGPIOPin14 = true;
                                VM.B01GroupGPIOCheckBoxPin14IsChecked = false;
                            }
                        }
                    }
                    else
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Set GPIO Pins error." : "設定GPIO Pins失敗", false);
                    }
                    break;
                
                case CommandStatus.GPIOGETPINS:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf.Length == 5)
                    {
                        int val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        if ((val & (byte)0x04) > 0)
                        {
                            VM.B01GroupGPIOCheckBoxStatus10IsChecked = true;
                        }
                        else
                        {
                            VM.B01GroupGPIOCheckBoxStatus10IsChecked = false;
                        }
                        if ((val & (byte)0x02) > 0)
                        {
                            VM.B01GroupGPIOCheckBoxStatus11IsChecked = true;
                        }
                        else
                        {
                            VM.B01GroupGPIOCheckBoxStatus11IsChecked = false;
                        }
                        if ((val & (byte)0x01) > 0)
                        {
                            VM.B01GroupGPIOCheckBoxStatus14IsChecked = true;
                        }
                        else
                        {
                            VM.B01GroupGPIOCheckBoxStatus14IsChecked = false;
                        }
                    }
                    break;

                case CommandStatus.GPIOPIN10C:
                case CommandStatus.GPIOPIN10UC:
                case CommandStatus.GPIOPIN11C:
                case CommandStatus.GPIOPIN11UC:
                case CommandStatus.GPIOPIN14C:
                case CommandStatus.GPIOPIN14UC:
                    B01DisplayInfoMsg("RX", s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;

                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf.Length == 5)
                    {
                        Int32 val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        VM.B01GroupGPIOComboBoxPins = VM.GPIOConfig[val];
                    }
                    break;


                case CommandStatus.B02T:
                case CommandStatus.B02ITEM02T:
                case CommandStatus.B02P:
                case CommandStatus.B02ITEM02P:
                case CommandStatus.B02ITEM02L:
                case CommandStatus.B02ITEM02W:
                case CommandStatus.B02ITEM02R:
                case CommandStatus.B02ITEM02CUSTOMIZE:
                    B02DisplayInfoMsg("RX", s_crlf, false, DateTime.Now);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.B02URSLOTQ:
                case CommandStatus.B02ITEM02URSLOTQ:
                case CommandStatus.B02UR:
                case CommandStatus.B02ITEM02UR:
                case CommandStatus.B02USLOTQ:
                case CommandStatus.B02ITEM02USLOTQ:
                case CommandStatus.B02U:
                case CommandStatus.B02ITEM02U:
                    DateTime dtU = DateTime.Now;
                    B02DisplayInfoMsg("RX", s_crlf, true, dtU);
                    B02DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'U'));
                    IsReceiveSubDataWork = true;
                    
                    if (s_crlf.Equals("\nU\r\n", StringComparison.CurrentCulture) || s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                    {

                        var ts2 = new TimeSpan(dtU.Ticks);
                        var ts1 = new TimeSpan(B02Item02CommandStartTime.Ticks);
                        var ts = ts1.Subtract(ts2).Duration();
                        
                        this.B02Item02CommandRunTimesCount += ts.TotalSeconds;
                        var avg1 = (Double)B02ListViewTagCount / B02Item02CommandRunTimesCount;
                        VM.B02GroupRecordTextBlockTimeAvgCount = avg1.ToString("0.0", CultureInfo.CurrentCulture);

                        if (s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                            ErrorCodeCheck(CommandStatus.B02URSLOTQ, "X");
                        DoProcess = CommandStatus.DEFAULT;
                        IsReceiveDataWork = false;
                    }
                    break;
                case CommandStatus.B02QR:
                case CommandStatus.B02ITEM02QR:
                case CommandStatus.B02Q:
                case CommandStatus.B02ITEM02Q:
                    DateTime dtQ = DateTime.Now;
                    B02DisplayInfoMsg("RX", s_crlf, false, dtQ);
                    B02DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'Q'));

                    var ts22 = new TimeSpan(dtQ.Ticks);
                    var ts11 = new TimeSpan(B02Item02CommandStartTime.Ticks);
                    var tss = ts11.Subtract(ts22).Duration();

                    this.B02Item02CommandRunTimesCount += tss.TotalSeconds;
                    var avg11 = (Double)B02ListViewTagCount / B02Item02CommandRunTimesCount;
                    VM.B02GroupRecordTextBlockTimeAvgCount = avg11.ToString("0.0", CultureInfo.CurrentCulture);

                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;

                case CommandStatus.B03SELECT:
                    B03DisplayInfoMsg("RX", s_crlf, false);
                    EMTemp = s_crlf;
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.B03WRITE:
                    B03DisplayInfoMsg("RX", s_crlf, false);
                    EMTemp = Format.RemoveCRLF(s_crlf);
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.B03READ:
                    B03DisplayInfoMsg("RX", s_crlf, false);
                    EMTemp = Format.RemoveCRLFandTarget(s_crlf, 'R');
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                
                case CommandStatus.B03GET:
                    B03DisplayInfoMsg("RX", s_crlf, false);
                    B03DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'U'));
                    IsReceiveSubDataWork = true;
                    if (s_crlf.Equals("\nU\r\n", StringComparison.CurrentCulture) || s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                    {
                        if (s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                            ErrorCodeCheck(CommandStatus.B02URSLOTQ, "X");
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Process end" : "執行結束", false);
                    }
                    break;
                case CommandStatus.CUSTOMEM:
                    B03DisplayInfoMsg("RX", s_crlf, false);
                    if (s_crlf.IndexOf('Y') != -1)
                    {
                        EMTemp = Format.RemoveCRLFandTarget(s_crlf, 'Y');
                    }
                    else
                    {
                        EMTemp = String.Empty;
                    }
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;

                case CommandStatus.B04GPIOPINS:
                    B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(s_crlf)));
                    b_crlf = Format.StringToBytes(s_crlf);
                    if (b_crlf != null && b_crlf.Length == 5)
                    {
                        Int32 val = Int32.Parse(Format.ByteToString(b_crlf[2]), CultureInfo.CurrentCulture);
                        if (B04AntennaTempData.Equals((Byte)val))
                            IsB04AntennaSetPinWork = true;
                        else IsB04AntennaSetPinWork = false;
                    }
                    else
                    {
                        IsB04AntennaSetPinWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Set GPIO Pins error." : "設定GPIO Pins失敗", false);
                    }
                    DoProcess = CommandStatus.DEFAULT;
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.B04ANTENNA01:
                case CommandStatus.B04ANTENNA02:
                case CommandStatus.B04ANTENNA03:
                case CommandStatus.B04ANTENNA04:
                    B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(s_crlf)));
                    B04DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'U'));
                    IsReceiveSubDataWork = true;
                    if (s_crlf.Equals("\nU\r\n", StringComparison.CurrentCulture) || s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                    {
                        if (s_crlf.Equals("\nX\r\n", StringComparison.CurrentCulture))
                            ErrorCodeCheck(CommandStatus.B02URSLOTQ, "X");
                        DoProcess = CommandStatus.DEFAULT;
                        IsReceiveDataWork = false;
                    }
                    break;

                case CommandStatus.INFO:
                    //B01DisplayInfoMsg("RX", s_crlf);
                    IsReceiveDataWork = false;
                    break;
                case CommandStatus.REGULATION:
                    break;

                case CommandStatus.DEFAULT:
                    if (TabCtrlIndex == 0)
                    {
                        B01DisplayInfoMsg("RX", s_crlf);
                    }
                    else if (TabCtrlIndex == 1)
                    {
                        B02DisplayInfoMsg("RX", s_crlf, false, DateTime.Now);
                        if (s_crlf.IndexOf("U", StringComparison.CurrentCulture) != -1)
                        {
                            if (s_crlf.Equals("\nU\r\n", StringComparison.CurrentCulture)) this.B02ListViewRunCount++;
                            B02DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'U'));
                        }
                        if (s_crlf.IndexOf("Q", StringComparison.CurrentCulture) != -1)
                        {
                            B02DisplayStatisticsMsg(Format.RemoveCRLFandTarget(s_crlf, 'Q'));
                        }
                    }
                    break;
                default:
                    B01DisplayInfoMsg("RX", s_crlf);
                    break;
            }
        }


        /// <summary>
        /// receive combne data event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">combne data argument class</param>
		private void DoReceiveDataWork(object sender, CombineDataReceiveArgumentEventArgs e)
        {
            ReceiveDataWork(e.Status, e.Data);
        }

        /// <summary>
        /// receive combne data event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">combne data argument class</param>
        private void DoReceiveDataWork(object sender, NETDataReceiveEventArgs e)
        {
            ReceiveDataWork(e.Status, e.Data);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DoReceiveDataWork(object sender, USBDataReceiveEventArgs e)
        {
            ReceiveDataWork(e.Status, e.Data);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DoReceiveDataWork(object sender, BLEDataReceiveEventArgs e)
        {
            ReceiveDataWork(e.Status, e.Data);
        }

        #endregion


        #region === #UI Element Function ===
        #region === PreviewKeyDown ===
        private void TextBoxPreviewKeyDownValidation(object sender, KeyEventArgs e)
        {
            TextBox _tbox = (TextBox)sender;

            if ((e.Key >= Key.D0 && e.Key <= Key.D9) || (e.Key >= Key.NumPad0 && e.Key <= Key.NumPad9)
                || (e.Key >= Key.A && e.Key <= Key.F)
                || e.Key == Key.Back || e.Key == Key.Left || e.Key == Key.Right)
            {
                switch (_tbox.Name)
                {
                    case "B01GroupRWTextBoxWrite":
                        if (String.IsNullOrEmpty(VM.B01GroupRWTextBoxLength))
                        {
                            e.Handled = true;
                            B01GroupRWTextBoxLength.Focus();
                            var _result = MessageBox.Show("請先編輯[Length]。", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture), MessageBoxButton.OK, MessageBoxImage.Information);
                        }
                        break;
                    case "B01GroupPreSetSelectBitData":
                        if (String.IsNullOrEmpty(VM.B01GroupPreSetSelectBitLength))
                        {
                            e.Handled = true;
                            B01GroupPreSetSelectBitLength.Focus();
                            var _result = MessageBox.Show("請先編輯[Length]。",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture), MessageBoxButton.OK, MessageBoxImage.Information);
                        }
                        break;
                }
            }
            else
            {
                e.Handled = true;
            }
        }

        private void TextBoxPreviewKeyDownDecValidation(object sender, KeyEventArgs e)
        {
            TextBox _tbox = (TextBox)sender;

            if ((e.Key >= Key.D0 && e.Key <= Key.D9) || (e.Key >= Key.NumPad0 && e.Key <= Key.NumPad9)
                || e.Key == Key.Back || e.Key == Key.Left || e.Key == Key.Right)
            {
                //e.Handled = false;
            }
            else
            {
                e.Handled = true;
            }
        }

        private void TextBoxPreviewKeyDownDecDotValidation(object sender, KeyEventArgs e)
        {
            TextBox _tbox = (TextBox)sender;

            if ((e.Key >= Key.D0 && e.Key <= Key.D9) || (e.Key >= Key.NumPad0 && e.Key <= Key.NumPad9)
                || e.Key == Key.Decimal
                || e.Key == Key.Back || e.Key == Key.Left || e.Key == Key.Right)
            {
                //e.Handled = false;
            }
            else
            {
                e.Handled = true;
            }
        }

        #endregion

        #region == KeyUp ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxKeyUpValidation(object sender, KeyEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[0-9a-fA-F]+\b\Z"); //@"[0-9a-fA-F]"

            if (regex.IsMatch(tbox.Text)) //e.Key.ToString(CultureInfo.CurrentCulture)
            {
                if ((ValidationStates)ValidationState[tbox.Name] != ValidationStates.OK)
                    tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[tbox.Name] = ValidationStates.OK;

                switch (tbox.Name)
                {
                    case "B01GroupPreSetSelectBitAddress":
                    case "B02GroupPreSetSelectBitAddress":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x3FFF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupPreSetSelectBitLength":
                    case "B02GroupPreSetSelectBitLength":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x60 || Convert.ToInt32(tbox.Text, 16) < 1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupPreSetSelectBitData":
                    case "B02GroupPreSetSelectBitData":
                        try
                        {
                            //Int32 nLength = tbox.Text.Length;
                            Int32 nBitsLength = (tbox.Name == "B01GroupPreSetSelectBitData") ?
                                Convert.ToInt32(VM.B01GroupPreSetSelectBitLength, 16) :
                                Convert.ToInt32(VM.B02GroupPreSetSelectBitLength, 16);
                            Int32 nMax = tbox.Text.Length * 4;
                            Int32 nMin = tbox.Text.Length * 4 - 3;
                            if (String.IsNullOrEmpty(tbox.Text) || nBitsLength < nMin || nBitsLength > nMax)
                            {
                                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                                ValidationState[tbox.Name] = ValidationStates.ERROR;
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "Bit length and data field isn't match, data={0}, the length value range is: 0x{1} ~ 0x{2}",
                                    tbox.Text,
                                    nMin.ToString("X2", new CultureInfo("en-us")),
                                    nMax.ToString("X2", new CultureInfo("en-us"))) :
                                    String.Format(CultureInfo.CurrentCulture, "位元長度與資料不符合,資料={0},對應的長度值範圍應為: 0x{1} ~ 0x{2}",
                                    tbox.Text,
                                    nMin.ToString("X2", new CultureInfo("en-us")),
                                    nMax.ToString("X2", new CultureInfo("en-us"))), true);
                            }
                            else
                            {
                                tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                                ValidationState[tbox.Name] = ValidationStates.OK;
                                MessageShow(String.Empty, false);
                            }
                        }
                        catch (ArgumentNullException ex)
                        {
                            MessageShow(ex.Message, false);
                        }
                        break;
                    case "B01GroupRWTextBoxAddress":
                    case "B02GroupPreSetReadAddress":
					case "B04GroupPreSetReadAddress":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x3FFF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupRWTextBoxLength":
                    case "B02GroupPreSetReadLength":
                    case "B01GroupEPCTextBoxTIDLength":
					case "B04GroupPreSetReadLength":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x20 || Convert.ToInt32(tbox.Text, 16) < 1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupRWTextBoxWrite":
                        Int32 nWordsLength = Convert.ToInt32(VM.B01GroupRWTextBoxLength, 16);
                        if (nWordsLength * 4 != tbox.Text.Length)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupLockTextBoxMask":
                    case "B01GroupLockTextBoxAction":
                        if (String.IsNullOrEmpty(tbox.Text) || tbox.Text.Length != 3 || Convert.ToInt32(tbox.Text, 16) > 0x3FF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;

                            NoName();
                        }
                        break;
                    case "B01TextBoxKillPassword":
                    case "B01GroupPreSetAccessPassword":
                    case "B02GroupPreSetAccessPassword":
                        if (String.IsNullOrEmpty(tbox.Text))
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B03GroupTagWindowTime":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0xFF || Convert.ToInt32(tbox.Text, 16) == 0x0)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    
                }
            }
            else
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);
                e.Handled = true;
            }
            tbox.UpdateLayout();
        }

        /// <summary>
        /// Decimal Validation
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxKeyUpDecValidation(object sender, KeyEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"^[0-9]+$");

            if (regex.IsMatch(tbox.Text))
            {
                if ((ValidationStates)ValidationState[tbox.Name] != ValidationStates.OK)
                    tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[tbox.Name] = ValidationStates.OK;

                switch (tbox.Name)
                {
                    case "B04Antenna1RunTimes":
                    case "B04Antenna2RunTimes":
                    case "B04Antenna3RunTimes":
                    case "B04Antenna4RunTimes":
                    case "B04Antenna5RunTimes":
                    case "B04Antenna6RunTimes":
                    case "B04Antenna7RunTimes":
                    case "B04Antenna8RunTimes":
                    case "B04AntennaLoopTimes":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, CultureInfo.CurrentCulture) > 1000 || Convert.ToInt32(tbox.Text, CultureInfo.CurrentCulture) <= 0)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                }
            }
            else
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);
                e.Handled = true;
            }
            tbox.UpdateLayout();
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxKeyUpDecDotValidation(object sender, KeyEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[.0-9]+\b\Z"); //@"[0-9a-fA-F]"

            if (regex.IsMatch(tbox.Text))
            {
                if ((ValidationStates)ValidationState[tbox.Name] != ValidationStates.OK)
                    tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[tbox.Name] = ValidationStates.OK;

                switch (tbox.Name)
                {
                    case "B04AntennaLoopDelayTime":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToDouble(tbox.Text, CultureInfo.CurrentCulture) > 86400 || Convert.ToDouble(tbox.Text, CultureInfo.CurrentCulture) < 0.1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                }
            }
            else
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);
                e.Handled = true;
            }
            tbox.UpdateLayout();
        }

        #endregion

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxLostFocusValidation(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;

            switch (tbox.Name)
            {
                case "B04Antenna1RunTimes":
                case "B04Antenna2RunTimes":
                case "B04Antenna3RunTimes":
                case "B04Antenna4RunTimes":
                case "B04Antenna5RunTimes":
                case "B04Antenna6RunTimes":
                case "B04Antenna7RunTimes":
                case "B04Antenna8RunTimes":
                case "B04AntennaLoopTimes":
                case "B04AntennaLoopDelayTime":
                    tbox.Style = (Style)FindResource("TextBoxDefaultDStyle");
                    break;
                default:
                    tbox.Style = (Style)FindResource("TextBoxDefaultStyle");
                    break;

            }
            
            //ValidationState[tbox.Name] = ValidationStates.DEFAULF;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void TextBoxGotFocusValidation(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[0-9a-fA-F]+\b\Z");
            if (!regex.IsMatch(tbox.Text))
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                tbox.UpdateLayout();
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);
                e.Handled = true;
                goto GotFocus;
            }

            if (this.IsFocus)
            {
                this.IsFocus = false;
                tbox.Style = (Style)FindResource("TextBoxFocusStyle");
                ValidationState[tbox.Name] = ValidationStates.FOCUS;
            }
            else
            {
                switch (tbox.Name)
                {
                    case "B01GroupPreSetSelectBitAddress":
                    case "B02GroupPreSetSelectBitAddress":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x3FFF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupPreSetSelectBitLength":
                    case "B02GroupPreSetSelectBitLength":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x60 || Convert.ToInt32(tbox.Text, 16) < 1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupPreSetSelectBitData":
                    case "B02GroupPreSetSelectBitData":
                        try
                        {
                            Int32 nLength = tbox.Text.Length;
                            Int32 nBitsLengthLength = (nLength == 0) ? 0 :
                                (tbox.Name == "B01GroupPreSetSelectBitData") ?
                                Convert.ToInt32(VM.B01GroupPreSetSelectBitLength, 16) :
                                Convert.ToInt32(VM.B02GroupPreSetSelectBitLength, 16);
                            Int32 nMax = nLength * 4;
                            Int32 nMin = nLength * 4 - 3;
                            
                            if (String.IsNullOrEmpty(tbox.Text) || nBitsLengthLength < nMin || nBitsLengthLength > nMax)
                            {
                                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                                ValidationState[tbox.Name] = ValidationStates.ERROR;
                            }
                            else
                            {
                                tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                                ValidationState[tbox.Name] = ValidationStates.OK;
                            }
                        }
                        catch (ArgumentNullException ex)
                        {
                            MessageShow(ex.Message, false);
                        }
                        break;
                    case "B01GroupRWTextBoxAddress":
                    case "B02GroupPreSetReadAddress":
					case "B04GroupPreSetReadAddress":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x3FFF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupRWTextBoxLength":
                    case "B02GroupPreSetReadLength":
                    case "B01GroupEPCTextBoxTIDLength":
					case "B04GroupPreSetReadLength":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x20 || Convert.ToInt32(tbox.Text, 16) < 1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01GroupRWTextBoxWrite":
                        Int32 nDataLength = tbox.Text.Length;
                        if (String.IsNullOrEmpty(VM.B01GroupRWTextBoxLength)) { B01GroupRWTextBoxLength.Focus(); goto GotFocus; }
                        Int32 nWordsLength = Convert.ToInt32(VM.B01GroupRWTextBoxLength, 16);
                        
                        if (nWordsLength * 4 == nDataLength)
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                            goto GotFocus;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        break;
                    case "B01GroupLockTextBoxMask":
                    case "B01GroupLockTextBoxAction":
                        if (String.IsNullOrEmpty(tbox.Text) || tbox.Text.Length != 3 || Convert.ToInt32(tbox.Text, 16) > 0x3FF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B01TextBoxKillPassword":
                    case "B01GroupPreSetAccessPassword":
                    case "B02GroupPreSetAccessPassword":
                        if (String.IsNullOrEmpty(tbox.Text))
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B03GroupTagWindowTime":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0xFF || Convert.ToInt32(tbox.Text, 16) == 0)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "B04Antenna1RunTimes":
                    case "B04Antenna2RunTimes":
                    case "B04Antenna3RunTimes":
                    case "B04Antenna4RunTimes":
                    case "B04Antenna5RunTimes":
                    case "B04Antenna6RunTimes":
                    case "B04Antenna7RunTimes":
                    case "B04Antenna8RunTimes":
                    case "B04AntennaLoopTimes":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, CultureInfo.CurrentCulture) > 1000 || Convert.ToInt32(tbox.Text, CultureInfo.CurrentCulture) <= 0)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                }
            }
            

            GotFocus:;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxGotFocusValidation1(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[.0-9a-fA-F]+\b\Z");
            if (!regex.IsMatch(tbox.Text))
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                tbox.UpdateLayout();
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);
                e.Handled = true;
                goto GotFocus1;
            }

            if (this.IsFocus)
            {
                this.IsFocus = false;
                tbox.Style = (Style)FindResource("TextBoxFocusStyle");
                ValidationState[tbox.Name] = ValidationStates.FOCUS;
            }
            else
            {
                switch (tbox.Name)
                {
                    case "B04AntennaLoopDelayTime":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToDouble(tbox.Text, CultureInfo.CurrentCulture) > 86400 || Convert.ToDouble(tbox.Text, CultureInfo.CurrentCulture) < 0.1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                }
            }
        GotFocus1:;
        }
        #endregion


        #region === #Border ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.MinWidth = this.ActualWidth;
            this.MinHeight = this.ActualHeight;
            this.MaxHeight = this.ActualHeight;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnBorderTitleMouseLeftDown(object sender, MouseButtonEventArgs e) { this.DragMove(); }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="s"></param>
        /// <param name="err">if true, show red foreground color</param>
		private void MessageShow(String s, Boolean err)
        {
            if (err)
                VM.BorderLabelMessageForegroud = Brushes.DarkRed;
            else
                VM.BorderLabelMessageForegroud = Brushes.Black;
            VM.BorderLabelMessage = s;
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnSettingButtonClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            switch (this._ConnectType)
            {
                case ReaderService.ConnectType.COM:
                    this._ICOM.CombineDataReceiveEventHandler -= this._CombineDataHandler;
                    break;
                case ReaderService.ConnectType.USB:
                    this._IUSB.USBDataReceiveEvent -= this._USBDataHandler;
                    break;
                case ReaderService.ConnectType.NET:
                    this._INet.NetTCPDataReceiveEventHandler -= this._NetTCPDataHandler;
                    break;
                case ReaderService.ConnectType.BLE:
                    this._IBLE.BLEDataReceiveEvent -= this._BLEDataHandler;
                    break;
            }


            this.DoProcess = CommandStatus.REGULATION;
            try
            {
                this._RegulationDialog = new RegulationDialog(ReaderService, this._ConnectType, this._VersionFW, this._BaudRate, this.BorderComboBoxCulture.SelectedItem as CultureInfo);
                Nullable<bool> b = this._RegulationDialog.ShowDialog();
                

                switch (this._ConnectType)
                {
                    case ReaderService.ConnectType.COM:
                        if (!this._BaudRate.Equals(this._RegulationDialog.GetBaudRate()))
                        {
                            this._BaudRate = this._RegulationDialog.GetBaudRate();
                            this.ReaderService = this._RegulationDialog.GetService();
                            this._ICOM = this._RegulationDialog.GetICOM();
                            this._SerialPort = this._ICOM.GetSerialPort();

                            this.ReaderService.COM = this._ICOM;
                        }
                        this._CombineDataHandler = new ICOM.CombineDataEventHandler(DoReceiveDataWork);
                        this._ICOM.CombineDataReceiveEventHandler += this._CombineDataHandler;
                        break;
                    case ReaderService.ConnectType.USB:
                        this._BaudRate = this._RegulationDialog.GetBaudRate();
                        this._USBDataHandler = new IUSB.USBDataEventHandler(DoReceiveDataWork);
                        this._IUSB.USBDataReceiveEvent += this._USBDataHandler;
                        break;
                    case ReaderService.ConnectType.NET:
                        this._NetTCPDataHandler = new INET.NetTCPDataEventHandler(DoReceiveDataWork);
                        this._INet.NetTCPDataReceiveEventHandler += _NetTCPDataHandler;
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._BaudRate = this._RegulationDialog.GetBaudRate();
                        this._BLEDataHandler = new IBLE.BLEDataEventHandler(DoReceiveDataWork);
                        this._IBLE.BLEDataReceiveEvent += this._BLEDataHandler;
                        break;
                }
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(ex.ToString());
            }
            
            this.DoProcess = CommandStatus.DEFAULT;
            this.SettingThread = new Thread(DoInfoWork) {
                IsBackground = true
            };
            this.SettingThread.Start();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnCloseConnectClick(object sender, RoutedEventArgs e)
        {
            if (VM.BorderCheckBoxStatusTag == "True")
            {
                switch(_ConnectType)
                {
                    case ReaderService.ConnectType.COM:
                        this._ICOM.Close();
                        break;
                    case ReaderService.ConnectType.NET:
                        this._INet.Close();
                        break;
                    case ReaderService.ConnectType.USB:
                        this._IUSB.Close();
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._IBLE.Close();
                        this._IBLE.Dispose();
                        break;
                }

                VM.BorderCheckBoxStatusTag = "False";
                GroupStatusControl(GroupStatus.ALL, false);
                //MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Disconnected" : "已中斷連線", false);
                MessageShow(Properties.Resources.MSG_DeviceDisconnected, false);
                VM.BorderTextBlockStatus = String.Empty;
                VM.BorderFirmwareVersion = String.Empty;
                VM.BorderTBReaderID = String.Empty;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnOpenConnectClick(object sender, RoutedEventArgs e)
        {
            OpenConnectDialog();
        }

        private void OpenConnectDialog()
        {
            if (VM.BorderCheckBoxStatusTag == "False")
            {
                this.IsReceiveDataWork = false;
                this.IsB02Repeat = false;
                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                B04AntennaTestEndWork();

                switch (_ConnectType)
                {
                    case ReaderService.ConnectType.COM:
                        this._ConnectDialog = new ConnectDialog(this._BaudRate);
                        break;
                    case ReaderService.ConnectType.USB:
                        this._ConnectDialog = new ConnectDialog(this._BaudRate, this._IUSB);
                        break;
                    case ReaderService.ConnectType.NET:
                        this._ConnectDialog = new ConnectDialog(this._BaudRate, this._INetInfo);
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._ConnectDialog = new ConnectDialog(this._BaudRate, this._IBLE);
                        break;
                }

                this._ConnectDialog.ShowDialog();
                if (_ConnectDialog.DialogResult.HasValue && !_ConnectDialog.DialogResult.Value) this.Close();
                else if (_ConnectDialog.DialogResult.HasValue && _ConnectDialog.DialogResult.Value)
                {
                    VM.BorderCheckBoxStatusTag = "True";

                    switch (this._ConnectDialog.GetIType())
                    {
                        case ReaderService.ConnectType.COM:
                            _ConnectType = ReaderService.ConnectType.COM;
                            this.ReaderService = this._ConnectDialog.GetService();
                            this._ICOM = this._ConnectDialog.GetICOM();
                            this._SerialPort = this._ICOM.GetSerialPort();
                            this._BaudRate = ICOM.GetBaudRate(this._SerialPort.BaudRate);
                            this._CombineDataHandler = new ICOM.CombineDataEventHandler(DoReceiveDataWork);
                            this._ICOM.CombineDataReceiveEventHandler += this._CombineDataHandler;
                            this.ReaderService.COM = this._ICOM;
                            VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0} ({1},{2},{3},{4})",
                                                                        this._SerialPort.PortName,
                                                                        this._SerialPort.BaudRate,
                                                                        this._SerialPort.DataBits,
                                                                        this._SerialPort.Parity,
                                                                        this._SerialPort.StopBits);
                            break;
                        case ReaderService.ConnectType.NET:
                            _ConnectType = ReaderService.ConnectType.NET;
                            this._INet = this._ConnectDialog.GetINET();
                            this._NetTCPDataHandler = new INET.NetTCPDataEventHandler(DoReceiveDataWork);
                            this._INet.NetTCPDataReceiveEventHandler += _NetTCPDataHandler;
                            this.ReaderService.NET = this._INet;
                            VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}:{1}", this._INet.IP(), this._INet.Port());

                            _INetInfo.IP = this._INet.IP();
                            _INetInfo.Port = this._INet.Port();
                            break;
                        case ReaderService.ConnectType.USB:
                            _ConnectType = ReaderService.ConnectType.USB;
                            this._IUSB = this._ConnectDialog.GetIUSB();
                            this._USBDataHandler = new IUSB.USBDataEventHandler(DoReceiveDataWork);
                            this._IUSB.USBDataReceiveEvent += this._USBDataHandler;
                            this.ReaderService.USB = this._IUSB;
                            VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}, VID:{1},PID:{2}", this._IUSB.ProductName, this._IUSB.VendorId, this._IUSB.ProductID);
                            break;
                        case ReaderService.ConnectType.BLE:
                            this._ConnectType = ReaderService.ConnectType.BLE;
                            this._IBLE = _ConnectDialog.GetIBLE();
                            this._BLEDataHandler = new IBLE.BLEDataEventHandler(DoReceiveDataWork);
                            this._IBLE.BLEDataReceiveEvent += this._BLEDataHandler;
                            this.ReaderService.BLE = this._IBLE;
                            VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0}/ {1}", this._IBLE.DeviceName, this._IBLE.DeviceId);
                            break;
                    }

                    this.SettingThread = new Thread(DoInfoWork)
                    {
                        IsBackground = true
                    };
                    this.SettingThread.Start();

                    GroupStatusControl(GroupStatus.ALL, true);
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Reconnect" : "已重新連線", false);
                }
                this._ConnectDialog.Close();
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnComboBoxCultureSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            CultureInfo selected_culture = this.BorderComboBoxCulture.SelectedItem as CultureInfo;
            this.Culture = selected_culture;

            if (Properties.Resources.Culture != null && !Properties.Resources.Culture.Equals(selected_culture))
                CulturesHelper.ChangeCulture(selected_culture);

            B04AntennaRunButton.Content = Properties.Resources.B04AntennaRun;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnRadioButtonTitleChecked(object sender, RoutedEventArgs e)
        {
            var radioButton = sender as RadioButton;

            if (TabControlPage == null) return;

            switch (Convert.ToInt32(radioButton.Tag.ToString(), CultureInfo.CurrentCulture))
            {
                case 1:
                    TabControlPage.SelectedIndex = 0;
                    break;
                case 2:
                    TabControlPage.SelectedIndex = 1;
                    break;
                case 3:
                    TabControlPage.SelectedIndex = 2;
                    break;
                case 4:
                    TabControlPage.SelectedIndex = 3;
                    break;
            }
            MessageShow(String.Empty, false);
            DoProcess = CommandStatus.DEFAULT;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnButtonCloseClick(object sender, RoutedEventArgs e)
        {
            
            switch (_ConnectType)
            {
                default:
                case ReaderService.ConnectType.DEFAULT:
                case ReaderService.ConnectType.COM:
                    if (this._ICOM.IsOpen()) this._ICOM.Close();
                    break;
                case ReaderService.ConnectType.USB:
                    if (this._IUSB.IsOpen) this._IUSB.Close();
                    break;
                case ReaderService.ConnectType.NET:
                    if (this._INet.IsConnected()) this._INet.Close();
                    break;
                case ReaderService.ConnectType.BLE:
                    if (this._IBLE.IsConnected) this._IBLE.Close();
                    break;
            }
            this.Close();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TabControlPageSelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            if (sender is TabControl tc)
            {
                TabCtrlIndex = tc.SelectedIndex;
            }
        }

        /// <summary>
        /// Read module information
        /// </summary>
		private void DoInfoWork()
        {
            Byte[] b;
            String s;

            try
            {
                this.DoProcess = CommandStatus.INFO;
                switch (_ConnectType)
                {
                    default:
                    case ReaderService.ConnectType.DEFAULT:
                    case ReaderService.ConnectType.COM:
                        this._ICOM.Send(this.ReaderService.CommandV(), ReaderModule.CommandType.Normal);
                        b = this._ICOM.Receive();
                        break;
                    case ReaderService.ConnectType.USB:
                        this._IUSB.Send(this.ReaderService.CommandV(), ReaderModule.CommandType.Normal);
                        b = _IUSB.Receive();
                        break;
                    case ReaderService.ConnectType.NET:
                        this._INet.Send(this.ReaderService.CommandV(), ReaderModule.CommandType.Normal);
                        b = this._INet.Receive();
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._IBLE.Send(this.ReaderService.CommandV(), ReaderModule.CommandType.Normal);
                        b = this._IBLE.Receive();
                        break;
                }

                if (b == null)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "The Reader not response." : "Reader沒有回應", true);
                    VM.BorderFirmwareVersion = "N/A";
                }
                else
                {
                    s = Format.RemoveCRLF(Format.BytesToString(b));
                    VM.BorderFirmwareVersion = s;
                    this._VersionFW = ReaderModule.Check(Format.HexStringToInt(s.Substring(1, 4)));

                    switch (this._VersionFW)
                    {
                        case ReaderModule.Version.FIR3008:
                            UIControPackets.Clear();
                            UIControPackets.Add(new UIControl(GroupStatus.GB01PRESET_SELECT, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB01GPIO, false));
                            UIControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            VM.B02GroupUSlotQCheckBoxIsChecked = false;
                            UIControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            VM.B02GroupReadCtrlCheckBoxIsChecked = false;
                            UIControPackets.Add(new UIControl(GroupStatus.GB03, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04ReadCtrl, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04SlotQ, false));

                            UIControlStatus(UIControPackets, true);

                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "the Reader version isn't support Select, compound multi command" : "此版本Reader不支援Select, 複合multi指令操作", false);
                            break;
                        case ReaderModule.Version.FIR300AC1:
                            UIControPackets.Clear();
                            UIControPackets.Add(new UIControl(GroupStatus.GB01GPIO, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            VM.B02GroupUSlotQCheckBoxIsChecked = false;
                            UIControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            VM.B02GroupReadCtrlCheckBoxIsChecked = false;
                            UIControPackets.Add(new UIControl(GroupStatus.GB03, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04ReadCtrl, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04SlotQ, false));
                            UIControlStatus(UIControPackets, true);
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "the Reader version isn't support compound multi command" : "此版本Reader不支援複合multi指令操作", false);
                            break;
                        case ReaderModule.Version.FIR300AC2:
                        case ReaderModule.Version.FIR300TD1:
                        case ReaderModule.Version.FIR300TD2:
                        case ReaderModule.Version.FIA300S:
                            UIControPackets.Clear();
                            UIControPackets.Add(new UIControl(GroupStatus.GB01GPIO, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB03, false));
                            UIControPackets.Add(new UIControl(GroupStatus.GB04, false));
                            UIControlStatus(UIControPackets, true);
                            break;
                        case ReaderModule.Version.FIR300AC2C4:
                        case ReaderModule.Version.FIR300AC3:
                        case ReaderModule.Version.FIR300AC2C5:
                        case ReaderModule.Version.FIR300AC2C6:
                        case ReaderModule.Version.FIR300AC3C5:
                        case ReaderModule.Version.FIR300TD204:
                        case ReaderModule.Version.FIR300TD205:
                        case ReaderModule.Version.FIR300TD206:
                        case ReaderModule.Version.FIR300SD305:
                        case ReaderModule.Version.FIR300SD306:
                        case ReaderModule.Version.FIR300VD406:
                        case ReaderModule.Version.FIR300S:
                        case ReaderModule.Version.FIR300AH:
                        //case ReaderModule.Version.FIR300SH:
                        //case ReaderModule.Version.FIR300TH:
                        ///case ReaderModule.Version.FIR300VH:
                            GroupStatusControl(GroupStatus.ALL, true);
                            break;
                        case ReaderModule.Version.FIRXXXX:
                            GroupStatusControl(GroupStatus.ALL, false);
                            UIControPackets.Clear();
                            UIControPackets.Add(new UIControl(GroupStatus.GB01CUSTOM, false));
                            UIControlStatus(UIControPackets, true);
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                String.Format(CultureInfo.CurrentCulture, "Unknown the Reader module version: {0}", s.Substring(1, 4)) :
                                String.Format(CultureInfo.CurrentCulture, "未知的Reader版本: {0}", s.Substring(1, 4)), false);
                            break;
                        case ReaderModule.Version.UNKNOW:
                            GroupStatusControl(GroupStatus.ALL, true);
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                               String.Format(CultureInfo.CurrentCulture, "more recent the Reader module version: {0}", s.Substring(1, 4)) :
                               String.Format(CultureInfo.CurrentCulture, "此裝置是較新的的Reader版本: {0}", s.Substring(1, 4)), false);
                            break;
                    }
                }

                this.DoProcess = CommandStatus.INFO;
                switch (_ConnectType)
                {
                    default:
                    case ReaderService.ConnectType.DEFAULT:
                    case ReaderService.ConnectType.COM:
                        this._ICOM.Send(this.ReaderService.CommandS(), ReaderModule.CommandType.Normal);
                        b = this._ICOM.Receive();
                        VM.BorderTextBlockStatus = String.Format(CultureInfo.CurrentCulture, "{0} ({1},{2},{3},{4})",
                                                    this._SerialPort.PortName,
                                                    this._SerialPort.BaudRate,
                                                    this._SerialPort.DataBits,
                                                    this._SerialPort.Parity,
                                                    this._SerialPort.StopBits);
                        break;
                    case ReaderService.ConnectType.USB:
                        this._IUSB.Send(this.ReaderService.CommandS(), ReaderModule.CommandType.Normal);
                        b = this._IUSB.Receive();
                        break;
                    case ReaderService.ConnectType.NET:
                        this._INet.Send(this.ReaderService.CommandS(), ReaderModule.CommandType.Normal);
                        b = this._INet.Receive();
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._IBLE.Send(this.ReaderService.CommandS(), ReaderModule.CommandType.Normal);
                        b = this._IBLE.Receive();
                        break;
                }
                if (b == null)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "The Reader not response." : "Reader沒有回應", false);
                    VM.BorderTBReaderID = "N/A";
                }
                else
                    VM.BorderTBReaderID = Format.RemoveCRLF(Format.BytesToString(b));

                this.DoProcess = CommandStatus.DEFAULT;

            }
            catch (InvalidOperationException ex)
            {
                MessageShow(ex.Message, false);
            }
            catch (ArgumentNullException argumentNullException)
            {
                MessageShow(argumentNullException.Message, false);
            }
            catch (SocketException socketException)
            {
                MessageShow(socketException.Message, false);
            }

            
        }


        #region === Menu ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnBorderMenuMouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            var menu = sender as TextBlock;
            /*if (CurrentPage == 2)
            {
                MenuPopCustom.IsEnabled = false;
                MenuLockGroup.IsEnabled = false;
            }
            else
            {
                MenuPopCustom.IsEnabled = true;
                MenuLockGroup.IsEnabled = true;
            }*/
            //this.MenuReaderModuleSet.Header = (this.Culture.IetfLanguageTag == "en-US") ? "Reader Module Set" : "Reader模組設定";

            /*this.MenuLockGroup.Header = (this.Culture.IetfLanguageTag == "en-US") ?
                (this.IsMenuLockGroup == false) ? "Set Lock to Engineering Mode" : "Set Lock to User Mode" :
                (this.IsMenuLockGroup == false) ? "切換Lock設定為工程模式" : "切換Lock設定為使用者模式";*/

            this.MenuTagCount.Header = (this.Culture.IetfLanguageTag == "en-US") ?
                (this.IsMenuRecordMode == true) ? "Normal Mode for Tag Record" : "Debug mode for Tag Record(Show Error)" :
                (this.IsMenuRecordMode == true) ? "切換至正常模式" : "切換至除錯模式";

            /*this.MenuPopCustom.Header = (this.Culture.IetfLanguageTag == "en-US") ?
                (this.IsMenuCustomPop == true) ? "Close Custom Command" : "Open Custom Command" :
                (this.IsMenuCustomPop == true) ? "關閉自訂指令" : "開啟自訂指令";*/

            /*this.MenuGCommand.Header = (this.Culture.IetfLanguageTag == "en-US") ?
                (this.IsMenuRecordMode == true) ? "G Command Receive Record" : "Normal Mode for Record" :
                (this.IsMenuRecordMode == true) ? "G指令接收模式" : "正常接收模式";*/

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnMenuBugRepoetClick(object sender, RoutedEventArgs e)
        {
            MessageShow(this.Culture.IetfLanguageTag == "en-US" ? "Not Implemented" : "未實作", false);
            //Process[] process = Process.GetProcessesByName("notepad");
            //Rectangle bounds;
            //GetWindowRect(process[0].MainWindowHandle, ref bounds);

            /*Bitmap bm = new Bitmap(
				//System.Windows.Forms.Screen.PrimaryScreen.Bounds.Width, 
				//System.Windows.Forms.Screen.PrimaryScreen.Bounds.Height
				bounds.Width,
				bounds.Height,
				System.Drawing.Imaging.PixelFormat.Format32bppArgb
				);
			System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(bm);
			g.CopyFromScreen(new System.Drawing.Point(0, 0),
				new System.Drawing.Point(0, 0),
				new System.Drawing.Size(
					System.Windows.Forms.Screen.PrimaryScreen.Bounds.Width, 
					System.Windows.Forms.Screen.PrimaryScreen.Bounds.Height));

			string strFilePath = AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "report";
			string strFileName = "\\ScreenShot_" + DateTime.Now.ToString("yyyyMMdd_hhmmss") + ".jpg";
			if (!Directory.Exists(strFilePath))
				Directory.CreateDirectory(strFilePath);
			string strFullPath = strFilePath + strFileName;
			bm.Save(strFullPath);*/
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnMenuRecordModeClick(object sender, RoutedEventArgs e)
        {
            if (IsMenuRecordMode) IsMenuRecordMode = false;
            else IsMenuRecordMode = true;
        }



        #endregion

        #endregion


        #region === #Pager01 ===

        private void InitializeB01()
        {
            ValidationState["B01GroupPreSetSelectBitAddress"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupPreSetSelectBitLength"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupPreSetSelectBitData"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupPreSetAccessPassword"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupEPCTextBoxTIDLength"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupRWTextBoxAddress"] = ValidationStates.OK;
            ValidationState["B01GroupRWTextBoxLength"] = ValidationStates.OK;
            ValidationState["B01GroupRWTextBoxWrite"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupLockTextBoxMask"] = ValidationStates.DEFAULF;
            ValidationState["B01GroupLockTextBoxAction"] = ValidationStates.DEFAULF;
            ValidationState["B01TextBoxKillPassword"] = ValidationStates.DEFAULF;
        }


        /// <summary>
        /// 2017.6.30 modify return parameter
        /// </summary>
        /// <param name="command"></param>
        /// <param name="process"></param>
        private Boolean DoB01SendWork(Byte[] command, CommandStatus process)
        {
            if (!IsReceiveDataWork)
            {
                if (process != CommandStatus.CUSTOM)
                    IsReceiveDataWork = true;

                this.DoProcess = process;
                try
                {
                    B01DisplayInfoMsg("TX", Format.BytesToString(command));
                    switch (_ConnectType)
                    {
                        default:
                        case ReaderService.ConnectType.DEFAULT:
                            break;
                        case ReaderService.ConnectType.COM:
                            this._ICOM.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.USB:
                            this._IUSB.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.NET:
                            this._INet.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.BLE:
                            this._IBLE.Send(command, ReaderModule.CommandType.Normal);
                            break;
                    }
                }
                catch (InvalidOperationException ex)
                {
                    MessageShow(ex.Message, true);
                    return false;
                }
                catch (ArgumentNullException ane)
                {
                    MessageShow(ane.Message, false);
                    return false;
                }
                catch (SocketException se)
                {
                    MessageShow(se.Message, false);
                    return false;
                }
                return true;
            }
            return false;
        }

        /// <summary>
        /// Check B01 select parameter information
        /// </summary>
        /// <returns></returns>
        private Boolean DoB01SelectInfoCheck()
        {
            if ((ValidationStates)ValidationState["B01GroupPreSetSelectBitAddress"] != ValidationStates.OK)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Bit address is null or value over range: 0 ~ 0x3FFF" : "位元位址為空或超出規範值: 0 ~ 0x3FFF", true);
                this.IsFocus = true;
                Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                {
                    B01GroupPreSetSelectBitAddress.Focus();
                }));
                goto SELECTINFO_EXIT;
            }
            
            if ((ValidationStates)ValidationState["B01GroupPreSetSelectBitLength"] != ValidationStates.OK )
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Bit length is null or  value over range: 0x01 ~ 0x60." :
                    "位元長度為空或超出規範值: 0x01 ~ 0x60", true);
                this.IsFocus = true;
                Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                {
                    B01GroupPreSetSelectBitLength.Focus();
                }));
                goto SELECTINFO_EXIT;
            }
            
            if ((ValidationStates)ValidationState["B01GroupPreSetSelectBitData"] != ValidationStates.OK)
            {
                Int32 nLength = VM.B01GroupPreSetSelectBitData.Length;
                if (nLength == 0)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Data is null." : "資料內容為空.", true);
                    this.IsFocus = true;
                    Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                    {
                        B01GroupPreSetSelectBitData.Focus();
                    }));
                    goto SELECTINFO_EXIT;
                }
                else
                {
                    Int32 nBitsLength = Convert.ToInt32(VM.B01GroupPreSetSelectBitLength, 16);
                    Int32 nMax = nLength * 4;
                    Int32 nMin = nLength * 4 - 3;
                    if ((nBitsLength < nMin) || (nBitsLength > nMax))
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Bit length and data field isn't match, data={0}, the length value range is: 0x{1} ~ 0x{2}",
                            VM.B01GroupPreSetSelectBitData,
                            nMin.ToString("X2", new CultureInfo("en-us")),
                            nMax.ToString("X2", new CultureInfo("en-us"))) :
                            String.Format(CultureInfo.CurrentCulture, "位元長度與資料不符合,資料={0},對應的長度值範圍應為: 0x{1} ~ 0x{2}",
                            VM.B01GroupPreSetSelectBitData,
                            nMin.ToString("X2", new CultureInfo("en-us")),
                            nMax.ToString("X2", new CultureInfo("en-us"))), true);
                        this.IsFocus = true;
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                        {
                            B01GroupPreSetSelectBitLength.Focus();
                        }));
                        goto SELECTINFO_EXIT;
                    }
                } 
            }

            return true;

            SELECTINFO_EXIT:return false;
        }


        /// <summary>
        /// Select command
        /// </summary>
        private Boolean DoB01SelectWork(CommandStatus cs)
        {
            int _index = 0;
            bool _f = false;

            if (DoB01SendWork(this.ReaderService.CommandT(VM.B01GroupPreSetSelectMemBank.Tag, VM.B01GroupPreSetSelectBitAddress, VM.B01GroupPreSetSelectBitLength, VM.B01GroupPreSetSelectBitData), cs))
            {
                _f = true;
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            return _f;
           
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
		private Boolean DoB01SelectCheckWork()
        {
            if (DoB01SelectInfoCheck())
            {
                if (DoB01SelectWork(CommandStatus.SELECT)) return true;
                else return false;
            }
            else return false;
        }

        #region === #Group Pre-Process ===
        private void OnB01ComboBoxMemBankSelectDownClosed(object sender, EventArgs e) { }
        private void OnB01ComboBoxMemBankSelectChanged(object sender, SelectionChangedEventArgs e) { }
        #endregion

        #region === #Group EPC/TID ===
        private void DoSinglePreSelectAccessEPCWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                IsReceiveDataWork = false;
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto SinglePreSelectAccessEPCEXIT;
            }

            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto SinglePreSelectAccessEPCEXIT;
                    }
                }

            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandQ(), CommandStatus.EPC))
            {
                while (IsReceiveDataWork) 
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Q command timeout(2000ms)." : "Q指令超時(2000ms)", true);
                        break;
                    }
                }
            }
            SinglePreSelectAccessEPCEXIT: IsRunning = false;
        }

        private void DoSinglePreSelectEPCWork()
        {
            Int32 _index = 0;
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto SinglePreSelectEPCEXIT;
            }
            else
            {

                if (DoB01SendWork(this.ReaderService.CommandQ(), CommandStatus.EPC))
                    while (IsReceiveDataWork)   
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Q command timeout(2000ms)." : "Q指令超時(2000ms)", true);
                            break;
                        }
                    }
            }
            SinglePreSelectEPCEXIT: IsRunning = false;
        }

        private void DoSinglePreAccessEPCWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD);
            while (IsReceiveDataWork)
            {
                Thread.Sleep(10);
                _index++;
                if (_index >= 200)
                {
                    IsReceiveDataWork = false;
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                    goto SinglePreAccessEPCEXIT;
                }
            }

            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandQ(), CommandStatus.EPC))
            {
                while (IsReceiveDataWork)  
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Q command timeout(2000ms)." : "Q指令超時(2000ms)", true);
                        break;
                    }
                }
            }
            SinglePreAccessEPCEXIT: IsRunning = false;
        }

        private void DoSingleEPCWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandQ(), CommandStatus.EPC))
            {
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Q command timeout(2000ms)." : "Q指令超時(2000ms)", true);
                        break;
                    }
                }
            }
            IsRunning = false;
        }

        private void DoSinglePreSelectAccessTIDWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto SinglePreSelectAccessTIDEXIT;
            }
            else
            {
                Int32 _index = 0;
                VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
                DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD);
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto SinglePreSelectAccessTIDEXIT;
                    }
                }

                if (DoB01SendWork(this.ReaderService.CommandR("2", "0", VM.B01GroupEPCTextBoxTIDLength), CommandStatus.TID))
                {
                    _index = 0;
                    while (IsReceiveDataWork) 
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "TID command timeout(2000ms)." : "TID指令超時(2000ms)", true);
                            break;
                        }
                    }
                }
            }
            SinglePreSelectAccessTIDEXIT: IsRunning = false;
        }

        private void DoSinglePreSelectTIDWork()
        {
            Int32 _index = 0;

            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto SinglePreSelectTIDEXIT;
            }
            else
            {
                if (DoB01SendWork(this.ReaderService.CommandR("2", "0", VM.B01GroupEPCTextBoxTIDLength), CommandStatus.TID))
                {
                    while (IsReceiveDataWork)
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "TID command timeout(2000ms)." : "TID指令超時(2000ms)", true);
                            break;
                        }
                    }
                }   
            }
            SinglePreSelectTIDEXIT: IsRunning = false;
        }

        private void DoSinglePreAccessTIDWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD);
            while (IsReceiveDataWork)
            {
                Thread.Sleep(10);
                _index++;
                if (_index >= 200)
                {
                    IsReceiveDataWork = false;
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                    goto SinglePreAccessTIDEXIT;
                }
            }

            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR("2", "0", VM.B01GroupEPCTextBoxTIDLength), CommandStatus.TID))
            {
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "TID command timeout(2000ms)." : "TID指令超時(2000ms)", true);
                        break;
                    }
                }
            }
                

            SinglePreAccessTIDEXIT: IsRunning = false;
        }

        private void DoSingleTIDWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR("2", "0", VM.B01GroupEPCTextBoxTIDLength), CommandStatus.TID))
            {
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Q command timeout(2000ms)." : "Q指令超時(2000ms)", true);
                        break;
                    }
                }
            }
            IsRunning = false;
        }

        private void OnB01CheckBoxRepeatChecked(object sender, RoutedEventArgs e) { }
        private void OnB01CheckBoxRepeatTIDChecked(object sender, RoutedEventArgs e) { }

        /// <summary>
        /// EPC Button
        /// </summary>
		private void OnB01GroupEPCButtonReadEPCClick(object sender, RoutedEventArgs e)
        {
            if (!IsRunning)
            {
                IsRunning = true;
                MessageShow(String.Empty, false);
                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.B01GroupPreSetAccessPassword.Focus();
                                IsRunning = false;
                                goto EPCEXIT;
                            }
                            this.SettingThread = new Thread(DoSinglePreSelectAccessEPCWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoSinglePreSelectEPCWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.B01GroupPreSetAccessPassword.Focus();
                            IsRunning = false;
                            goto EPCEXIT;
                        }
                        this.SettingThread = new Thread(DoSinglePreAccessEPCWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoSingleEPCWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                }
                EPCEXIT: ;
            }   
        }

        /// <summary>
        /// TID Button
        /// </summary>
        private void OnB01GroupEPCButtonReadTIDClick(object sender, RoutedEventArgs e)
        {
            if (!this.IsRunning)
            {
                this.IsRunning = true;
                MessageShow(String.Empty, false);

                if ((ValidationStates)ValidationState["B01GroupEPCTextBoxTIDLength"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Length is null" : "讀取長度不得為空", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupEPCTextBoxTIDLength.Focus();
                    return;
                }

                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if (String.IsNullOrEmpty(VM.B01GroupPreSetAccessPassword))
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.IsRunning = false;
                                this.B01GroupPreSetAccessPassword.Focus();
                                goto TIDEXIT;
                            }
                            this.SettingThread = new Thread(DoSinglePreSelectAccessTIDWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoSinglePreSelectTIDWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else this.IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupPreSetAccessPassword.Focus();
                            goto TIDEXIT;
                        }
                        this.SettingThread = new Thread(DoSinglePreAccessTIDWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoSingleTIDWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();

                    }
                }
                TIDEXIT:;

            }
                

            

            
        }
        #endregion

        #region === #Group Read/Write ===
        private void DoPreSelectAccessReadWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto PreSelectAccessReadEXIT;
            }
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto PreSelectAccessReadEXIT;
                    }
                }
            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR(VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress, VM.B01GroupRWTextBoxLength), CommandStatus.READ))
                while (IsReceiveDataWork) 
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Read(R) command timeout(2000ms)." : "Read(R)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreSelectAccessReadEXIT: this.IsRunning = false;
        }

        private void DoPreSelectReadWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto PreSelectReadEXIT;
            }
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR(VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress, VM.B01GroupRWTextBoxLength), CommandStatus.READ))
                while (IsReceiveDataWork)   
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Read(R) command timeout(2000ms)." : "Read(R)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreSelectReadEXIT: this.IsRunning = false;
        }

        private void DoPreAccessReadWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto PreAccessReadEXIT;
                    }
                }
            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR(VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress, VM.B01GroupRWTextBoxLength), CommandStatus.READ))
                while (IsReceiveDataWork)  
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Read(R) command timeout(2000ms)." : "Read(R)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreAccessReadEXIT: this.IsRunning = false;
        }

        private void DoPreSelectAccessWriteWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto PreSelectAccessWriteEXIT;
            }
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto PreSelectAccessWriteEXIT;
                    }
                }
            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandW(
                VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress,
                VM.B01GroupRWTextBoxLength,
                VM.B01GroupRWTextBoxWrite), CommandStatus.WRITE))
                while (IsReceiveDataWork)   
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Write(W) command timeout(2000ms)." : "Write(W)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreSelectAccessWriteEXIT: this.IsRunning = false;
        }

        private void DoPreSelectWriteWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." : "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto PreSelectWriteEXIT;
            }
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandW(
                VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress,
                VM.B01GroupRWTextBoxLength,
                VM.B01GroupRWTextBoxWrite), CommandStatus.WRITE))
                while (IsReceiveDataWork)   
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Write(W) command timeout(2000ms)." : "Write(W)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreSelectWriteEXIT: this.IsRunning = false;
        }

        private void DoPreAccessWriteWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto PreAccessWriteEXIT;
                    }
                }
            _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandW(
                VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress,
                VM.B01GroupRWTextBoxLength,
                VM.B01GroupRWTextBoxWrite), CommandStatus.WRITE))
                while (IsReceiveDataWork)   
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Write(W) command timeout(2000ms)." : "Write(W)指令超時(2000ms)", true);
                        break;
                    }
                }

            PreAccessWriteEXIT: this.IsRunning = false;
        }

        private void DoWriteWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandW(
                VM.B01GroupRWComboBoxMemBank.Tag,
                VM.B01GroupRWTextBoxAddress,
                VM.B01GroupRWTextBoxLength,
                VM.B01GroupRWTextBoxWrite), CommandStatus.WRITE))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Write(W) command timeout(2000ms)." : "Write(W)指令超時(2000ms)", true);
                        break;
                    }
                }
            this.IsRunning = false;
        }

        private void DoReadWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandR(VM.B01GroupRWComboBoxMemBank.Tag, VM.B01GroupRWTextBoxAddress, 
                VM.B01GroupRWTextBoxLength), CommandStatus.READ))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Read(R) command timeout(2048ms)." : "Read(R)指令超時(2048ms)", true);
                        break;
                    }
                }
            this.IsRunning = false;
        }

        private void OnB01GroupRWMemBankDownClosed(object sender, EventArgs e)
        {
            switch ((sender as ComboBox).SelectedIndex)
            {
                case 0: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "0"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "4"); 
                    break;
                case 1: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "2"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "6"); 
                    break;
                case 2: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "0"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "4"); 
                    break;
                case 3: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "0"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "1"); 
                    break;
                case 4: 
                    B01GroupRWTextBoxAddress.Text = ""; 
                    B01GroupRWTextBoxLength.Text = ""; 
                    break;
                case 5: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "0"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "2"); 
                    break;
                case 6: 
                    B01GroupRWTextBoxAddress.Text = String.Format(CultureInfo.CurrentCulture, "2"); 
                    B01GroupRWTextBoxLength.Text = String.Format(CultureInfo.CurrentCulture, "2"); 
                    break;
            }
        }

        /// <summary>
        /// Write Button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnB01GroupRWButtonWriteClick(object sender, RoutedEventArgs e)
        {
            if (!this.IsRunning)
            {
                this.IsRunning = true;
                MessageShow(String.Empty, false);
                if (VM.B01GroupRWComboBoxMemBank.Tag.Equals("X", StringComparison.CurrentCulture))
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Bank choice error" :
                            "Bank選擇錯誤", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWComboBoxMemBank.Focus();
                    goto WRITEEXIT;
                }
                if ((ValidationStates)ValidationState["B01GroupRWTextBoxAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Address is null or value over range: 0 ~ 0x3FFF." :
                            "寫入位址為空或超出規範值: 0 ~ 0x3FFF", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWTextBoxAddress.Focus();
                    goto WRITEEXIT;
                }

                if ((ValidationStates)ValidationState["B01GroupRWTextBoxLength"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Length is null or value over range: 1 ~ 0x20" :
                        "位元組長度為空或超出規範值:1 ~ 0x20", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWTextBoxLength.Focus();
                    goto WRITEEXIT;
                }

                //if ((ValidationStates)ValidationState["B01GroupRWTextBoxWrite"] != ValidationStates.OK)
                //{
                    Int32 nDataLength = this.B01GroupRWTextBoxWrite.Text.Length;
                    if (nDataLength == 0)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Data field is null" : "寫入資料不得為空", true);
                        this.IsFocus = true;
                        this.IsRunning = false;
                        this.B01GroupRWTextBoxWrite.Focus();
                        goto WRITEEXIT;
                    }
                    else
                    {
                        Int32 nWordsLength = Convert.ToInt32(this.B01GroupRWTextBoxLength.Text, 16);
                        if (nWordsLength * 4 != nDataLength)
                        {
                            if (nWordsLength * 4 > nDataLength)
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "Length and data field is not match, data field must be fill {0} char.", (nWordsLength * 4 - nDataLength)) :
                                    String.Format(CultureInfo.CurrentCulture, "位元組長度值與資料內容不匹配，資料欄位應再填入{0}字元", (nWordsLength * 4 - nDataLength)), true);
                            else
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "Length and data field is not match, data field must be remove {0} char.", (nDataLength - nWordsLength * 4)) :
                                    String.Format(CultureInfo.CurrentCulture, "位元組長度值與資料內容不匹配，資料欄位應再移除{0}字元", (nDataLength - nWordsLength * 4)), true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupRWTextBoxWrite.Focus();
                            goto WRITEEXIT;
                        }
                    }
                //}

                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.IsRunning = false;
                                this.B01GroupPreSetAccessPassword.Focus();
                                goto WRITEEXIT;
                            }
                            this.SettingThread = new Thread(DoPreSelectAccessWriteWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoPreSelectWriteWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else this.IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupPreSetAccessPassword.Focus();
                            goto WRITEEXIT;
                        }
                        this.SettingThread = new Thread(DoPreAccessWriteWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoWriteWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                }
            }
            WRITEEXIT:;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupRWButtonReadClick(object sender, RoutedEventArgs e)
        {
            if (!this.IsRunning)
            {
                this.IsRunning = true;
                MessageShow(String.Empty, false);

                if (VM.B01GroupRWComboBoxMemBank.Tag.Equals("X", StringComparison.CurrentCulture))
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Bank choice error" :
                        "Bank選擇錯誤", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWComboBoxMemBank.Focus();
                    goto READEXIT;
                }

                if ((ValidationStates)ValidationState["B01GroupRWTextBoxAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Address is null or value over range: 0 ~ 0x3FFF." :
                            "讀取位址為空或超出規範值: 0 ~ 0x3FFF", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWTextBoxAddress.Focus();
                    goto READEXIT;
                }

                if ((ValidationStates)ValidationState["B01GroupRWTextBoxLength"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Length is null or value over range: 1 ~ 0x20" :
                        "位元組長度為空或超出規範值:1 ~ 0x20", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupRWTextBoxLength.Focus();
                    goto READEXIT;
                }


                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.IsRunning = false;
                                this.B01GroupPreSetAccessPassword.Focus();
                                goto READEXIT;
                            }
                            this.SettingThread = new Thread(DoPreSelectAccessReadWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoPreSelectReadWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else this.IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupPreSetAccessPassword.Focus();
                            goto READEXIT;
                        }
                        this.SettingThread = new Thread(DoPreAccessReadWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoReadWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                }
                READEXIT:;
            } 

        }
        #endregion

        #region === #Group Lock ===

		private Int32 LockPayloadMask(Int32 mask, Int32 index)
        {
            if (mask == 0) return 0x0;
            else
            {
                this.ActionField |= (mask - 1) << index;
                if (((mask - 1) & 1) == 0) mask = 2;
                else mask = 3;
                return mask << index;
            }
        }

        private void NoName()
        {
            if (String.IsNullOrEmpty(VM.B01GroupLockTextBoxMask) || String.IsNullOrEmpty(VM.B01GroupLockTextBoxAction))
                return;

            var mask = Format.HexStringToInt(VM.B01GroupLockTextBoxMask);
            var action = Format.HexStringToInt(VM.B01GroupLockTextBoxAction);
            for (Int32 i = 0; i < 10; i = i + 2)
            {
                var mask_ = (mask & (3 << i)) >> i;
                var action_ = (action & (3 << i)) >> i;

                switch (i)
                {
                    case 0:
                        B01ComboBoxLockUser.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 2:
                        B01ComboBoxLockTID.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 4:
                        B01ComboBoxLockEPC.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 6:
                        B01ComboBoxLockAccessPwd.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 8:
                        B01ComboBoxLockKillPwd.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                }

            }
        }

        private void OnB01GroupLockComboBoxDownClosed(object sender, EventArgs e)
        {
            this.MaskField = 0;
            this.ActionField = 0;

            this.MaskField |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)B01ComboBoxLockKillPwd.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 8);
            this.MaskField |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)B01ComboBoxLockAccessPwd.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 6);
            this.MaskField |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)B01ComboBoxLockEPC.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 4);
            this.MaskField |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)B01ComboBoxLockTID.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 2);
            this.MaskField |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)B01ComboBoxLockUser.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 0);
            B01GroupLockTextBoxMask.Text = this.MaskField.ToString("X3", CultureInfo.CurrentCulture);
            TextBoxGotFocusValidation(B01GroupLockTextBoxMask, null);
            TextBoxKeyUpValidation(B01GroupLockTextBoxMask, null);

            B01GroupLockTextBoxAction.Text = this.ActionField.ToString("X3", CultureInfo.CurrentCulture);
            TextBoxGotFocusValidation(B01GroupLockTextBoxAction, null);
            TextBoxKeyUpValidation(B01GroupLockTextBoxAction, null);
        }


        private void DoPreSelectAccessLockWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2048ms) or parameter error." :
                        "Select(T)指令超時(2048ms)或參數錯誤。", true);
                goto PreSelectAccessLockEXIT;
            }
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);
            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2048ms)." : "Access(P)指令超時(2048ms)", true);
                        goto PreSelectAccessLockEXIT;
                    }
                }
            _index = 0;

            if (DoB01SendWork(this.ReaderService.CommandL(VM.B01GroupLockTextBoxMask, VM.B01GroupLockTextBoxAction), CommandStatus.LOCK))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Lock(L) command timeout(2048ms)." : "Lock(L)指令超時(2048ms)", true);
                        break;
                    }
                }

            PreSelectAccessLockEXIT: this.IsRunning = false;
        }

        private void DoPreSelectLockWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2048ms) or parameter error." :
                        "Select(T)指令超時(2048ms或參數錯誤。", true);
                goto PreSelectLockEXIT;
            }
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandL(VM.B01GroupLockTextBoxMask, VM.B01GroupLockTextBoxAction), CommandStatus.LOCK))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Lock(L) command timeout(2048ms)." : "Lock(L)指令超時(2048ms)", true);
                        break;
                    }
                }
            PreSelectLockEXIT: this.IsRunning = false;
        }

        private void DoPreAccessLockWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);

            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2048ms)." : "Access(P)指令超時(2048ms)", true);
                        goto PreAccessLockEXIT;
                    }
                }

            if (DoB01SendWork(this.ReaderService.CommandL(VM.B01GroupLockTextBoxMask, VM.B01GroupLockTextBoxAction), CommandStatus.LOCK))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Lock(L) command timeout(2048ms)." : "Lock(L)指令超時(2048ms)", true);
                        break;
                    }
                }
            PreAccessLockEXIT: this.IsRunning = false;
        }

        private void DoLockWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.CommandL(VM.B01GroupLockTextBoxMask, VM.B01GroupLockTextBoxAction), CommandStatus.LOCK))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Lock(L) command timeout(2048ms)." : "Lock(L)指令超時(2048ms)", true);
                        break;
                    }
                }

            this.IsRunning = false;
        }

        /// <summary>
        /// Lock Button
        /// </summary>
		private void OnB01GroupLockButtonClick(object sender, RoutedEventArgs e)
        {
            if (!this.IsRunning)
            {
                this.IsRunning = true;
                MessageShow(String.Empty, false);

                Boolean _af = false, _am = false;
                if ((ValidationStates)ValidationState["B01GroupLockTextBoxAction"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Lock Action is null or content error." :
                        "Action設定不得為空或內容錯誤", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupLockTextBoxAction.Focus();
                    _af = true;
                }
                if ((ValidationStates)ValidationState["B01GroupLockTextBoxMask"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Lock Mask is null or content error." :
                        "Mask設定不得為空或內容錯誤", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B01GroupLockTextBoxMask.Focus();
                    _am = true;
                }
                if (_af || _am) goto LOCKEXIT;

                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.IsRunning = false;
                                this.B01GroupPreSetAccessPassword.Focus();
                                goto LOCKEXIT;
                            }
                            this.SettingThread = new Thread(DoPreSelectAccessLockWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoPreSelectLockWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else this.IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupPreSetAccessPassword.Focus();
                            goto LOCKEXIT;
                        }
                        this.SettingThread = new Thread(DoPreAccessLockWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoLockWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                }
                LOCKEXIT:;
            }

            
        }
        #endregion

        #region === #Group Kill ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01ButtonKillClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if ((ValidationStates)ValidationState["B01TextBoxKillPassword"] != ValidationStates.OK)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Kill password is null." :
                    "寫入銷毀密碼不得為空", true);
                this.B01TextBoxKillPassword.Focus();
                goto KILLEXIT;
            }


            DoB01SendWork(this.ReaderService.CommandK(VM.B01TextBoxKillPassword, "0"), CommandStatus.KILL);

            KILLEXIT:;
        }
        #endregion

        #region === #Group GPIO ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur10Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if (!IsGetGPIOConfiguration10)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("44"), CommandStatus.GPIOCONFIG10C);
            }
            IsGetGPIOConfiguration10 = false;
            GPIOConfigurMask |= 0x04;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur10UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if (!IsGetGPIOConfiguration10)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("40"), CommandStatus.GPIOCONFIG10UC);
            }
            IsGetGPIOConfiguration10 = false;
            GPIOConfigurMask &= 0xFB;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur11Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOConfiguration11)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("22"), CommandStatus.GPIOCONFIG11C);
            }
            IsGetGPIOConfiguration11 = false;
            GPIOConfigurMask |= 0x02;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur11UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOConfiguration11)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("20"), CommandStatus.GPIOCONFIG11UC);
            }
            IsGetGPIOConfiguration11 = false;
            GPIOConfigurMask &= 0xFD;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur14Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOConfiguration14)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("11"), CommandStatus.GPIOCONFIG14C);
            }
            IsGetGPIOConfiguration14 = false;
            GPIOConfigurMask |= 0x01;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOConfigur14UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOConfiguration14)
            {
                DoB01SendWork(this.ReaderService.SetGPIOConfiguration("10"), CommandStatus.GPIOCONFIG14UC);
            }
            IsGetGPIOConfiguration14 = false;
            GPIOConfigurMask &= 0xFE;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOComboBoxConfigurDownClosed(object sender, EventArgs e)
        {
            String gval = VM.B01GroupGPIOComboBoxConfigur.Tag;
            String str = "7" + gval;

            MessageShow(String.Empty, false);
            DoB01SendWork(this.ReaderService.SetGPIOConfiguration(str), CommandStatus.GPIOCONFIG);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOButtonGetConfigurationClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            DoB01SendWork(this.ReaderService.GetGPIOConfiguration(), CommandStatus.GPIOCONFIG);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins10Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin10)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("44"), CommandStatus.GPIOPIN10C);
            }
            IsGetGPIOPin10 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins10UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin10)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("40"), CommandStatus.GPIOPIN10UC);
            }
            IsGetGPIOPin10 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins11Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin11)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("22"), CommandStatus.GPIOPIN11C);
            }
            IsGetGPIOPin11 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins11UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin11)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("20"), CommandStatus.GPIOPIN11UC);
            }
            IsGetGPIOPin11 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins14Checked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin14)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("11"), CommandStatus.GPIOPIN14C);
            }
            IsGetGPIOPin14 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOPins14UnChecked(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            if (!IsGetGPIOPin14)
            {
                DoB01SendWork(this.ReaderService.SetGPIOPins("10"), CommandStatus.GPIOPIN14UC);
            }
            IsGetGPIOPin14 = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOComboBoxPinsDownClosed(object sender, EventArgs e)
        {
            String gval = VM.B01GroupGPIOComboBoxPins.Tag;
            String str = "7" + gval;

            MessageShow(String.Empty, false);
            DoB01SendWork(this.ReaderService.SetGPIOPins(str), CommandStatus.GPIOPINS);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupGPIOButtonGetPortClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            DoB01SendWork(this.ReaderService.GetGPIOPins(), CommandStatus.GPIOGETPINS);

        }


        #endregion

        #region === #Group Custom ===
        private void DoPreSelectAccessCustomWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
					"Select(T) command timeout(2048ms) or parameter error." :
                    "Select(T)指令超時(2048ms)或參數錯誤。", true);
                goto PreSelectAccessCustomEXIT;
            }
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);

            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2048ms)." : "Access(P)指令超時(2048ms)", true);
                        goto PreSelectAccessCustomEXIT;
                    }
                }

            _index = 0;
            if (DoB01SendWork(this.ReaderService.Custom(VM.B01GroupMsgTextBoxCustom), CommandStatus.CUSTOM))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Custom command timeout(2048ms)." : "Custom指令超時(2048ms)", true);
                        break;
                    }
                }
            PreSelectAccessCustomEXIT: this.IsRunning = false;
        }

        private void DoPreSelectCustomWork()
        {
            if (!DoB01SelectWork(CommandStatus.SELECT))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
					"Select(T) command timeout(2048ms) or parameter error." :
                    "Select(T)指令超時(2048ms)或參數錯誤。", true);
                goto PreSelectCustomEXIT;
            }
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.Custom(VM.B01GroupMsgTextBoxCustom), CommandStatus.CUSTOM))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Custom command timeout(2048ms)." : "Custom指令超時(2048ms)", true);
                        break;
                    }
                }
            PreSelectCustomEXIT: this.IsRunning = false;
        }

        private void DoPreAccessCustomWork()
        {
            Int32 _index = 0;
            VM.B01GroupPreSetAccessPassword = Format.MakesUpZero(VM.B01GroupPreSetAccessPassword, 8);

            if (DoB01SendWork(this.ReaderService.CommandP(VM.B01GroupPreSetAccessPassword), CommandStatus.PASSWORD))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2048ms)." : "Access(P)指令超時(2048ms)", true);
                        goto PreAccessCustomEXIT;
                    }
                }
            _index = 0;
            if (DoB01SendWork(this.ReaderService.Custom(VM.B01GroupMsgTextBoxCustom), CommandStatus.CUSTOM))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Custom command timeout(2048ms)." : "Custom指令超時(2048ms)", true);
                        break;
                    }
                }
            PreAccessCustomEXIT: this.IsRunning = false;
        }

        private void DoCustomWork()
        {
            Int32 _index = 0;
            if (DoB01SendWork(this.ReaderService.Custom(VM.B01GroupMsgTextBoxCustom), CommandStatus.CUSTOM))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 512)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Custom command timeout(2048ms)." : "Custom指令超時(2048ms)", true);
                        break;
                    }
                }
            this.IsRunning = false;
        }

        /// <summary>
        /// Custom Button
        /// </summary>
        private void OnB01GroupMsgButtonCustomClick(object sender, RoutedEventArgs e)
        {
            if (!this.IsRunning)
            {
                this.IsRunning = true;
                MessageShow(String.Empty, false);

                if (VM.B01GroupPreSetSelectCheckBoxIsChecked)
                {
                    if (DoB01SelectInfoCheck())
                    {
                        if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                        {
                            if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.IsRunning = false;
                                this.B01GroupPreSetAccessPassword.Focus();
                                goto CustomEXIT;
                            }
                            this.SettingThread = new Thread(DoPreSelectAccessCustomWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.SettingThread = new Thread(DoPreSelectCustomWork)
                            {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                    else this.IsRunning = false;
                }
                else
                {
                    if (VM.B01GroupPreSetAccessheckBoxIsChecked)
                    {
                        if ((ValidationStates)ValidationState["B01GroupPreSetAccessPassword"] != ValidationStates.OK)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Access(P) command is pre-processed, the field is null" :
                                "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                            this.IsFocus = true;
                            this.IsRunning = false;
                            this.B01GroupPreSetAccessPassword.Focus();
                            goto CustomEXIT;
                        }
                        this.SettingThread = new Thread(DoPreAccessCustomWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                    else
                    {
                        this.SettingThread = new Thread(DoCustomWork)
                        {
                            IsBackground = true
                        };
                        this.SettingThread.Start();
                    }
                }
                CustomEXIT:;
            }
            

        }
        #endregion

        #region === #Group Message & Record ===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupMsgListBoxMenuItemDeleteAllClick(object sender, RoutedEventArgs e)
        {
            VM.B01GroupMsgListBox.Clear();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnB01GroupMsgListBoxMenuItemDeleteRangeClick(object sender, RoutedEventArgs e)
        {
            ListBox lb = new ListBox();
            object[] ob = new object[this.B01GroupMsgListBox.SelectedItems.Count];
            this.B01GroupMsgListBox.SelectedItems.CopyTo(ob, 0);

            foreach (object obj in ob)
                VM.B01GroupMsgListBox.Remove(obj as B01ListboxItem);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB01GroupMsgListBoxMenuItemCopyClick(object sender, RoutedEventArgs e)
        {
            String collectedText = String.Empty;

            foreach (B01ListboxItem lbi in B01GroupMsgListBox.SelectedItems)
            {
                collectedText += lbi.Handler + lbi.Content + "\r\n";
            }

            if (B01GroupMsgListBox.SelectedItems != null)
            {
                Clipboard.SetText(collectedText);
            }

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
        /// <param name="data"></param>
		private void B01DisplayInfoMsg(String str, String data)
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                if (VM.B01GroupMsgListBox.Count > 2000)
                    VM.B01GroupMsgListBox.Clear();

                var item = new B01ListboxItem();

                switch (str) {
                    case "TX":
                        this.IsDateTimeStamp = true;

                        item.HandlerColor = Brushes.SeaGreen;
                        item.ContentColor = Brushes.SeaGreen;
                        item.Handler = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", DateTime.Now.ToString("yy/MM/dd H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                        item.Content = Format.ShowCRLF(data);
                        break;
                    case "RX":
                        item.HandlerColor = Brushes.DarkRed;
                        item.ContentColor = Brushes.DarkRed;

                        if (this.IsDateTimeStamp)
                        {
                            item.Handler = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", DateTime.Now.ToString("yy/MM/dd H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                            item.Content = Format.ShowCRLF(data);
                        }
                        else
                        {
                            item.Handler = String.Empty;
                            if (IsReceiveDataWork)
                                item.Content = Format.ShowCRLF(data);
                            else
                                item.Content = String.Format(CultureInfo.CurrentCulture, "{0}  -- {1}", Format.ShowCRLF(data), DateTime.Now.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture));
                        }
                        this.IsDateTimeStamp = false;
                        break;
                    case "Error":
                        item.HandlerColor = Brushes.DarkRed;
                        item.ContentColor = Brushes.DarkRed;
                        item.Handler = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", DateTime.Now.ToString("yy/MM/dd H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                        item.Content = Format.ShowCRLF(data);
                        this.IsDateTimeStamp = false;
                        break;
                }
               
                VM.B01GroupMsgListBox.Add(item);
                this.B01GroupMsgListBox.ScrollIntoView(VM.B01GroupMsgListBox[VM.B01GroupMsgListBox.Count - 1]);
            }));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
        /// <param name="buffer"></param>
        /// <param name="oldV"></param>
        /// <param name="newV"></param>
		private void B01DisplayInfoMsg(String str, String buffer, String oldV, String newV)
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
            {
                if (VM.B01GroupMsgListBox.Count > 2000)
                    VM.B01GroupMsgListBox.Clear();

                var item = new B01ListboxItem();
                if (str == "TX")
                {
                    this.IsDateTimeStamp = true;
                    item.HandlerColor = Brushes.SeaGreen;
                    item.ContentColor = Brushes.SeaGreen;
                    //itm.Foreground = Brushes.SeaGreen;
                }
                else
                {
                    item.HandlerColor = Brushes.DarkRed;
                    item.ContentColor = Brushes.DarkRed;
                    //itm.Foreground = Brushes.DarkRed;
                }

                String s1 = String.Empty, s2 = String.Empty;
                if (this.IsDateTimeStamp)
                {
                    if (str == "RX") this.IsDateTimeStamp = false;
                    s1 = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", DateTime.Now.ToString("yy/MM/dd H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                    s2 = Format.ShowCRLF(buffer);
                }
                else
                    s2 = Format.ShowCRLF(buffer);
                s2 = s2.Replace(oldV, newV);
                item.Handler = s1;
                item.Content = s2;
                //itm.Content = s;



                VM.B01GroupMsgListBox.Add(item);
                this.B01GroupMsgListBox.ScrollIntoView(VM.B01GroupMsgListBox[VM.B01GroupMsgListBox.Count - 1]);
                /*if (this.B01GroupMsgListBox.Items.Count > 1000)
                    this.B01GroupMsgListBox.Items.Clear();

                this.B01GroupMsgListBox.Items.Add(itm);
                this.B01GroupMsgListBox.ScrollIntoView(this.B01GroupMsgListBox.Items[this.B01GroupMsgListBox.Items.Count - 1]);
                itm = null;*/
            }));
        }

        #endregion

        #endregion


        #region === #Pager02 ===

        private void InitializeB02()
        {
            ValidationState["B02GroupPreSetSelectBitAddress"] = ValidationStates.DEFAULF;
            ValidationState["B02GroupPreSetSelectBitLength"] = ValidationStates.DEFAULF;
            ValidationState["B02GroupPreSetSelectBitData"] = ValidationStates.DEFAULF;
            ValidationState["B02GroupPreSetAccessPassword"] = ValidationStates.DEFAULF;
            ValidationState["B02GroupPreSetReadAddress"] = ValidationStates.OK;
            ValidationState["B02GroupPreSetReadLength"] = ValidationStates.OK;
            
            this.B02Item02Process.Tick += new EventHandler(DoB02Item02ProcessWork);
            this.B02Item02Process.Interval = TimeSpan.FromMilliseconds(6);

            this.B02Item02ListBox.ItemsSource = B02Item02Commands = new ObservableCollection<B02Item02Command>();
            B02Item02Commands.Add(new B02Item02Command());

            //VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
            //VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="command">data</param>
        /// <param name="process">command type</param>
		private Boolean DoB02SendWork(Byte[] command, CommandStatus process)
        {
            if (!IsReceiveDataWork)
            {
                IsReceiveDataWork = true;
                switch (process)
                {
                    case CommandStatus.B02U:
                    case CommandStatus.B02UR:
                    case CommandStatus.B02USLOTQ:
                    case CommandStatus.B02URSLOTQ:
                    case CommandStatus.B02QR:
                    case CommandStatus.B02Q:
                    case CommandStatus.B02ITEM02U:
                    case CommandStatus.B02ITEM02UR:
                    case CommandStatus.B02ITEM02USLOTQ:
                    case CommandStatus.B02ITEM02URSLOTQ:
                    case CommandStatus.B02ITEM02QR:
                    case CommandStatus.B02ITEM02Q:
                        this.B02ListViewRunCount++;
                        break;
                }
                
                this.DoProcess = process;
                try
                {
                    this.B02Item02CommandStartTime = DateTime.Now;
                    B02DisplayInfoMsg("TX", Format.BytesToString(command), false, this.B02Item02CommandStartTime);

                    switch (_ConnectType)
                    {
                        default:
                        case ReaderService.ConnectType.DEFAULT:
                            break;
                        case ReaderService.ConnectType.COM:
                            this._ICOM.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.USB:
                            this._IUSB.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.NET:
                            this._INet.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.BLE:
                            this._IBLE.Send(command, ReaderModule.CommandType.Normal);
                            break;
                    }
                }
                catch (ArgumentNullException ex)
                {
                    MessageShow(ex.Message, false);
                    return false;
                }
                catch (SocketException sx)
                {
                    MessageShow(sx.Message, false);
                    return false;
                }
                return true;
            }
            return false;
        }

        /// <summary>
        /// Show List box message
        /// </summary>
        /// <param name="str">direct type: "TX""RX"</param>
        /// <param name="data">data</param>
		private void B02DisplayInfoMsg(String str, String data, Boolean b, DateTime dt)
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => 
            {
                ListBoxItem itm = new ListBoxItem();

                if (str == "TX")
                {
                    this.IsDateTimeStamp = true;
                    itm.Foreground = Brushes.SeaGreen;
                }
                else
                    itm.Foreground = Brushes.DarkRed;
                if (this.IsDateTimeStamp)
                {
                    if (str == "RX") this.IsDateTimeStamp = false;
                    if (data == null)
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", dt.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                    else
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - {2}", dt.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), str, Format.ShowCRLF(data));
                }
                else
                {
                    if (data == null)
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "[{0}] - ", str);
                    else if (IsReceiveDataWork)
                    {
                        if (b)
                            itm.Content = String.Format(CultureInfo.CurrentCulture, "{0}      - {1}", dt.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), Format.ShowCRLF(data));
                        else
                            itm.Content = Format.ShowCRLF(data);
                    }  
                    else
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0}  -- {1}", Format.ShowCRLF(data), dt.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture));
                }
                if (this.B02ListBox.Items.Count > 2000)
                    //this.B02ListBox.Items.RemoveAt(0);
                    this.B02ListBox.Items.Clear();

                //log message
                if (VM.B02GroupMsgLogCheckBoxIsChecked)
                    B02MessageLogger.Info(itm.Content.ToString());

                this.B02ListBox.Items.Add(itm);

                itm = null;
            }));
        }


        private List<String> B02ListViewList = new List<String>();
        /// <summary>
        /// Show list view statistics message
        /// </summary>
        /// <param name="str"></param>
		private void B02DisplayStatisticsMsg(String str)
        {

            Boolean bCompare = false, isCRC = false; //isEPC = false;
            
            String[] data = str.Split(',');
            Int32 number = data[0].Length;

            try
            {
                if (Format.CRC16(Format.HexStringToBytes(data[0])) == 0x1D0F) isCRC = true;
                else
                    isCRC = false;
            }
            catch (ArgumentException)
            {
                return;
            }


            if (!IsMenuRecordMode && !isCRC)
            {
                VM.B02GroupRecordTextBlockCount = B02ListViewList.Distinct().Count().ToString(CultureInfo.CurrentCulture);
                VM.B02GroupRecordTextBlockRunCount = this.B02ListViewRunCount.ToString(CultureInfo.CurrentCulture);
                VM.B02GroupRecordTextBlockTagCount = B02ListViewTagCount.ToString(CultureInfo.CurrentCulture);
                if (VM.B02ListViewItemsSource.Count > 0)
                {
                    for (Int32 j = 0; j < VM.B02ListViewItemsSource.Count; j++)
                    {
                        number = Convert.ToInt32(VM.B02ListViewItemsSource[j].B02Count, CultureInfo.CurrentCulture);
                        if (this.B02ListViewRunCount > 0)
                            VM.B02ListViewItemsSource[j].B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / this.B02ListViewRunCount));
                    }
                }
                return;
            }

            if (!IsMenuRecordMode && VM.B02GroupReadCtrlCheckBoxIsChecked)
                if (data[1].Length <= 1) return;

            if (number > 8)
            {
                var newBank = new B02ListViewItem()
                {
                    B02PC = data[0].Substring(0, 4),
                    B02EPC = data[0].Substring(4, data[0].Length - 8),
                    B02CRC16 = (isCRC) ? data[0].Substring(data[0].Length - 4, 4) : data[0].Substring(data[0].Length - 4, 4) + " x",
                    B02Read = (data.Length > 1) ? data[1] : String.Empty
                };

                if (VM.B02ListViewItemsSource.Count > 0)
                {
                    for (Int32 j = 0; j < VM.B02ListViewItemsSource.Count; j++)
                    {
                        number = Convert.ToInt32(VM.B02ListViewItemsSource[j].B02Count, CultureInfo.CurrentCulture);
                        if (VM.B02ListViewItemsSource[j].B02CRC16.Equals(newBank.B02CRC16, StringComparison.CurrentCulture) 
                            && VM.B02ListViewItemsSource[j].B02Read.Equals(newBank.B02Read, StringComparison.CurrentCulture))
                        {
                            number++;
                            VM.B02ListViewItemsSource[j].B02Count = number.ToString(CultureInfo.CurrentCulture);
                            if (this.B02ListViewRunCount == 0)
                                VM.B02ListViewItemsSource[j].B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / 1));
                            else
                                VM.B02ListViewItemsSource[j].B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / this.B02ListViewRunCount));
                            bCompare = true;
                            break;
                        }
                        else
                            bCompare = false;


                        /*if (B02ListViewRunCount > 0)
                            VM.B02ListViewItemsSource[j].B02Percentage = 
                                String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / this.B02ListViewRunCount));

                        if (bCompare) break;*/
                            
                    }
                }

                //new tag
                if (!bCompare)
                {
                    newBank.B02Count = "1";
                    number = Convert.ToInt32(newBank.B02Count, CultureInfo.CurrentCulture);
                    if (this.B02ListViewRunCount == 0)
                        newBank.B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / 1));
                    else
                        newBank.B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / this.B02ListViewRunCount));
                    VM.B02ListViewAddNewItem(newBank);
                    B02ListViewList.Add(data[0]);
                    VM.B02GroupRecordTextBlockCount = B02ListViewList.Distinct().Count().ToString(CultureInfo.CurrentCulture);
                }

                B02ListViewTagCount++;
                VM.B02GroupRecordTextBlockTagCount = B02ListViewTagCount.ToString(CultureInfo.CurrentCulture);
            }
            else
            {
                if (VM.B02ListViewItemsSource.Count > 0)
                {
                    for (Int32 j = 0; j < VM.B02ListViewItemsSource.Count; j++)
                    { 
                        number = Convert.ToInt32(VM.B02ListViewItemsSource[j].B02Count, CultureInfo.CurrentCulture);
                        if (this.B02ListViewRunCount > 0)
                            VM.B02ListViewItemsSource[j].B02Percentage = String.Format(CultureInfo.CurrentCulture, "{0}%", (Int32)(number * 100 / this.B02ListViewRunCount));
                    }
                }
            }
            VM.B02GroupRecordTextBlockRunCount = this.B02ListViewRunCount.ToString(CultureInfo.CurrentCulture);    
        }

        /// <summary>
        /// Check B02 select parameter information
        /// </summary>
        /// <returns></returns>
        private Boolean DoB02SelectInfoCheck()
        {
            if ((ValidationStates)ValidationState["B02GroupPreSetSelectBitAddress"] != ValidationStates.OK)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Bit address is null or over range: 0 ~ 0x3FFF." :
                    "位元位址為空或超出規範值: 0 ~ 0x3FFF", true);
                this.IsFocus = true;
                this.IsRunning = false;
                this.B02GroupPreSetSelectBitAddress.Focus();
                goto B02SELECTINFO_EXIT;
            }

            if ((ValidationStates)ValidationState["B02GroupPreSetSelectBitLength"] != ValidationStates.OK)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Bit length is null or over range: 0x01 ~ 0x60." :
                    "位元長度為空或超出規範值: 0x01 ~ 0x60", true);
                this.IsFocus = true;
                this.IsRunning = false;
                this.B02GroupPreSetSelectBitLength.Focus();
                goto B02SELECTINFO_EXIT;
            }


            if ((ValidationStates)ValidationState["B02GroupPreSetSelectBitData"] != ValidationStates.OK)
            {
                Int32 nLength = VM.B02GroupPreSetSelectBitData.Length;
                if (nLength == 0)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Data is null." :
                    "資料內容不得為空.", true);
                    this.IsFocus = true;
                    this.IsRunning = false;
                    this.B02GroupPreSetSelectBitData.Focus();
                    goto B02SELECTINFO_EXIT;
                }
                else
                {
                    Int32 nBitsLength = Convert.ToInt32(VM.B02GroupPreSetSelectBitLength, 16);
                    Int32 nMax = nLength * 4;
                    Int32 nMin = nLength * 4 - 3;
                    if ((nBitsLength < nMin) || (nBitsLength > nMax))
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Bit length and data field isn't match, data={0}, the length value range is: 0x{1} ~ 0x{2}",
                                VM.B02GroupPreSetSelectBitData,
                                nMin.ToString("X2", new CultureInfo("en-us")),
                                nMax.ToString("X2", new CultureInfo("en-us"))) :
                            String.Format(CultureInfo.CurrentCulture, "位元長度與資料不符合,資料={0},對應的長度值範圍應為: 0x{1} ~ 0x{2}",
                                VM.B02GroupPreSetSelectBitData,
                                nMin.ToString("X2", new CultureInfo("en-us")),
                                nMax.ToString("X2", new CultureInfo("en-us"))), true);
                        this.IsFocus = true;
                        this.IsRunning = false;
                        this.B02GroupPreSetSelectBitLength.Focus();
                        goto B02SELECTINFO_EXIT;
                    }
                }
            }    
            return true;

            B02SELECTINFO_EXIT:
            return false;
        }

        /// <summary>
        /// Do select command, 2000ms time out
        /// </summary>
        /// <param name="cs">command state</param>
        /// <returns></returns>
        private Boolean DoB02SelectWork(CommandStatus cs)
        {
            Int32 _index = 0;
            Boolean _f = false;

            if (DoB02SendWork(this.ReaderService.CommandT(VM.B02GroupPreSetSelectMemBank.Tag,
                VM.B02GroupPreSetSelectBitAddress, VM.B02GroupPreSetSelectBitLength, VM.B02GroupPreSetSelectBitData), cs))
            {
                _f = true;
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        _f = false;
                        IsReceiveDataWork = false;
                        break;
                    }
                }
            }

            return _f;
        }

        /// <summary>
        /// Do select, acess and UR command process
        /// </summary>
        private void DoB02PreSelectAccessURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                if (!DoB02SelectWork(CommandStatus.B02T))
                {
                    this.IsB02Repeat = false;
                    UIControlStatus(UITempControPackets, false);
                    UIControlStatus(UIControPackets, true);
                    VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? 
                        "Select(T) command timeout(2000ms) or parameter error.":
                        "Select(T)指令超時(2000ms)或參數錯誤。", true);
                    break;
                }
                else
                {
                    _index = 0;
                    VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);
                    
                    if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                        while (IsReceiveDataWork)
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 200)
                            {
                                IsReceiveDataWork = false;
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                                return;
                            }
                        }

                    switch (DoFakeProcess)
                    {
                        case CommandStatus.B02URSLOTQ:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                                VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                                CommandStatus.B02URSLOTQ);
                            break;
                        case CommandStatus.B02UR:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                                VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                                CommandStatus.B02UR);
                            break;
                        case CommandStatus.B02USLOTQ:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                            break;
                        case CommandStatus.B02U:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandU(), CommandStatus.B02U);
                            break;
                        default:
                            _callback = false;
                            break;
                    }

                    if (_callback)
                    {
                        _index = 0;
                        while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                        {
                            Thread.Sleep(10);
                            if (IsReceiveSubDataWork)
                            {
                                _index = 0;
                                IsReceiveSubDataWork = false;
                            }
                            _index++;
                            if (_index >= 300)
                            {
                                this.IsB02ThreadRunCtrl = false;
                                this.IsB02Repeat = false;
                                UIControlStatus(UITempControPackets, false);
                                UIControlStatus(UIControPackets, true);
                                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                                /*Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                                {
                                    this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                                }));*/
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                    "U指令超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                                IsReceiveDataWork = false;
                                break;
                            }
                        }
                    }
                }
            }
        }


        private void DoB02PreSelectURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                if (!DoB02SelectWork(CommandStatus.B02T))
                {
                    this.IsB02ThreadRunCtrl = false;
                    this.IsB02Repeat = false;
                    UIControlStatus(UITempControPackets, false);
                    UIControlStatus(UIControPackets, true);
                    VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                    /*Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                    {
                        this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                    }));*/
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2048ms) or parameter error." :
                        "Select(T)指令超時(2048ms)或參數錯誤。", true);
                    break;
                }
                else
                {
                    switch (DoFakeProcess)
                    {
                        case CommandStatus.B02URSLOTQ:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                                VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                                CommandStatus.B02URSLOTQ);
                            break;
                        case CommandStatus.B02UR:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                                VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                                CommandStatus.B02UR);
                            break;
                        case CommandStatus.B02USLOTQ:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                            break;
                        case CommandStatus.B02U:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandU(), CommandStatus.B02U);
                            break;
                        default:
                            _callback = false;
                            break;
                    }

                    if (_callback)
                    {
                        _index = 0;
                        while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                        {
                            Thread.Sleep(10);
                            if (IsReceiveSubDataWork)
                            {
                                _index = 0;
                                IsReceiveSubDataWork = false;
                            }
                            _index++;
                            if (_index >= 300)
                            {
                                this.IsB02ThreadRunCtrl = false;
                                this.IsB02Repeat = false;
                                UIControlStatus(UITempControPackets, false);
                                UIControlStatus(UIControPackets, true);
                                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                    "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                                IsReceiveDataWork = false;
                                break;
                            }
                        }
                    }
                }
            }
        }


        private void DoB02PreAccessURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                _index = 0;
                VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

                if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                    while (IsReceiveDataWork)
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            IsReceiveDataWork = false;
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                            break;
                        }
                    }

                switch (DoFakeProcess)
                {
                    case CommandStatus.B02URSLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02URSLOTQ);
                        break;
                    case CommandStatus.B02UR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02UR);
                        break;
                    case CommandStatus.B02USLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                        break;
                    case CommandStatus.B02U:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(), CommandStatus.B02U);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                    {
                        Thread.Sleep(10);
                        if (IsReceiveSubDataWork)
                        {
                            _index = 0;
                            IsReceiveSubDataWork = false;
                        }
                        _index++;
                        if (_index >= 300)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
        }


        private void DoB02URWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                switch (DoFakeProcess)
                {
                    case CommandStatus.B02URSLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02URSLOTQ);
                        break;
                    case CommandStatus.B02UR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02UR);
                        break;
                    case CommandStatus.B02USLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                        break;
                    case CommandStatus.B02U:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(), CommandStatus.B02U);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                    {
                        Thread.Sleep(10);
                        if (IsReceiveSubDataWork)
                        {
                            _index = 0;
                            IsReceiveSubDataWork = false;
                        }
                        _index++;
                        if (_index >= 300)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
        }


        private void DoB02SinglePreSelectAccessURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;
            if (!DoB02SelectWork(CommandStatus.B02T))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
					"Select(T) command timeout(200ms) or parameter error." :
                    "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto B02SinglePreSelectAccessUREXIT;
            }
            else
            {
                _index = 0;
                VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

                if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                    while (IsReceiveDataWork)
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            IsReceiveDataWork = false;
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                            goto B02SinglePreSelectAccessUREXIT;
                        }
                    }

                switch (DoFakeProcess)
                {
                    case CommandStatus.B02URSLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02URSLOTQ);
                        break;
                    case CommandStatus.B02UR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02UR);
                        break;
                    case CommandStatus.B02USLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                        break;
                    case CommandStatus.B02U:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandU(), CommandStatus.B02U);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                    {
                        Thread.Sleep(10);
                        if (IsReceiveSubDataWork)
                        {
                            _index = 0;
                            IsReceiveSubDataWork = false;
                        }
                        _index++;
                        if (_index >= 300)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                            /*Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() =>
                            {
                                this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                            }));*/
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
            B02SinglePreSelectAccessUREXIT: IsRunning = false;
        }


        private void DoB02SinglePreSelectURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            if (!DoB02SelectWork(CommandStatus.B02T))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Select(T) command timeout(2000ms) or parameter error." :
                    "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto B02SinglePreSelectUREXIT;
            }
            else
            {
                switch (DoFakeProcess)
                {
                    case CommandStatus.B02URSLOTQ:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02URSLOTQ);
                        break;
                    case CommandStatus.B02UR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                            VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02UR);
                        break;
                    case CommandStatus.B02USLOTQ:
                        _callback = DoB02SendWork(this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                        break;
                    case CommandStatus.B02U:
                        _callback = DoB02SendWork(this.ReaderService.CommandU(), CommandStatus.B02U);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                    {
                        Thread.Sleep(10);
                        if (IsReceiveSubDataWork)
                        {
                            _index = 0;
                            IsReceiveSubDataWork = false;
                        }
                        _index++;
                        if (_index >= 300)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                                "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
            B02SinglePreSelectUREXIT: this.IsRunning = false;
        }

        private void DoB02SinglePreAccessURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;
          
            VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

            if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
            while (IsReceiveDataWork)
            {
                Thread.Sleep(10);
                _index++;
                if (_index >= 200)
                {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms) and stop process." : "Access(P)指令超時(2000ms)，執行結束。", true);
                        goto B02SinglePreAccessUREXIT;
                }
            }

            switch (DoFakeProcess)
            {
                case CommandStatus.B02URSLOTQ:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                        VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02URSLOTQ);
                    break;
                case CommandStatus.B02UR:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                        VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02UR);
                    break;
                case CommandStatus.B02USLOTQ:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                    break;
                case CommandStatus.B02U:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandU(), CommandStatus.B02U);
                    break;
                default:
                    _callback = false;
                    break;
            }

            if (_callback)
            {
                _index = 0;
                while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                {
                    Thread.Sleep(10);
                    if (IsReceiveSubDataWork)
                    {
                        _index = 0;
                        IsReceiveSubDataWork = false;
                    }
                    _index++;
                    if (_index >= 300)
                    {
                        this.IsB02ThreadRunCtrl = false;
                        this.IsB02Repeat = false;
                        UIControlStatus(UITempControPackets, false);
                        UIControlStatus(UIControPackets, true);
                        VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                            "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                        IsReceiveDataWork = false;
                        break;
                    }
                }
            }
            B02SinglePreAccessUREXIT: this.IsRunning = false;
        }

        private void DoB02SingleURWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            switch (DoFakeProcess)
            {
                case CommandStatus.B02URSLOTQ:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandUR(VM.B02GroupUSlotQComboBox.Tag, VM.B02GroupPreSetReadMemBank.Tag,
                        VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02URSLOTQ);
                    break;
                case CommandStatus.B02UR:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandUR(null, VM.B02GroupPreSetReadMemBank.Tag,
                        VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), CommandStatus.B02UR);
                    break;
                case CommandStatus.B02USLOTQ:
                    _callback = DoB02SendWork(this.ReaderService.CommandU(VM.B02GroupUSlotQComboBox.Tag), CommandStatus.B02USLOTQ);
                    break;
                case CommandStatus.B02U:
                    _callback = DoB02SendWork(this.ReaderService.CommandU(), CommandStatus.B02U);
                    break;
                default:
                    _callback = false;
                    break;
            }

            if (_callback)
            {
                _index = 0;
                while (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
                {
                    Thread.Sleep(10);
                    if (IsReceiveSubDataWork)
                    {
                        _index = 0;
                        IsReceiveSubDataWork = false;
                    }
                    _index++;
                    if (_index >= 300)
                    {
                        this.IsB02ThreadRunCtrl = false;
                        this.IsB02Repeat = false;
                        this.IsReceiveDataWork = false;
                        UIControlStatus(UITempControPackets, false);
                        UIControlStatus(UIControPackets, true);
                        VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Multi(U) command timeout(the last reception command interval is more than 3 seconds) and break process." :
                            "Multi(U)超時(最後接收的U指令起，間隔超過3秒)，執行中斷。", true);
                        IsReceiveDataWork = false;
                        break;
                    }
                }
            }
            this.IsRunning = false;
        }

        private void DoB02PreSelectAccessQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                if (!DoB02SelectWork(CommandStatus.B02T))
                {
                    this.IsB02Repeat = false;
                    UIControlStatus(UITempControPackets, false);
                    UIControlStatus(UIControPackets, true);
                    VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." :
                        "Select(T)指令超時(2000ms)或參數錯誤。", true);
                    break;
                }
                else
                {
                    _index = 0;
                    VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

                    if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                        while (IsReceiveDataWork)
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 200)
                            {
                                IsReceiveDataWork = false;
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                                return;
                            }
                        }

                    switch (DoFakeProcess)
                    {
                        case CommandStatus.B02QR:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength), 
                                CommandStatus.B02QR);
                            break;
                        case CommandStatus.B02Q:
                            _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                            break;
                        default:
                            _callback = false;
                            break;
                    }

                    if (_callback)
                    {
                        _index = 0;
                        while (IsReceiveDataWork) 
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 200)
                            {
                                this.IsB02ThreadRunCtrl = false;
                                this.IsB02Repeat = false;
                                UIControlStatus(UITempControPackets, false);
                                UIControlStatus(UIControPackets, true);
                                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
									"EPC(Q) command timeout(2000ms)." :
                                    "EPC(Q)指令超時(2000ms)", true);
                                IsReceiveDataWork = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        private void DoB02PreSelectQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                if (!DoB02SelectWork(CommandStatus.B02T))
                {
                    this.IsB02Repeat = false;
                    UIControlStatus(UITempControPackets, false);
                    UIControlStatus(UIControPackets, true);
                    VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Select(T) command timeout(2000ms) or parameter error." :
                        "Select(T)指令超時(2000ms)或參數錯誤。", true);
                    break;
                }
                else
                {
                    switch (DoFakeProcess)
                    {
                        case CommandStatus.B02QR:
                            _callback = DoB02SendWork(
                                this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                                CommandStatus.B02QR);
                            break;
                        case CommandStatus.B02Q:
                            _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                            break;
                        default:
                            _callback = false;
                            break;
                    }

                    if (_callback)
                    {
                        _index = 0;
                        while (IsReceiveDataWork)  
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 200)
                            {
                                this.IsB02ThreadRunCtrl = false;
                                this.IsB02Repeat = false;
                                UIControlStatus(UITempControPackets, false);
                                UIControlStatus(UIControPackets, true);
                                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
									"EPC(Q) command timeout(2000ms)." :
                                    "EPC(Q)指令超時(2000ms)", true);
                                IsReceiveDataWork = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        private void DoB02PreAccessQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                _index = 0;
                VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

                if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                            IsReceiveDataWork = false;
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                            return;
                    }
                }

                switch (DoFakeProcess)
                {
                    case CommandStatus.B02QR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02QR);
                        break;
                    case CommandStatus.B02Q:
                        _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)  
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "EPC(Q) command timeout(2000ms)." :
                                "EPC(Q)指令超時(2000ms)", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
        }

        private void DoB02QRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            while (this.IsB02ThreadRunCtrl)
            {
                switch (DoFakeProcess)
                {
                    case CommandStatus.B02QR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02QR);
                        break;
                    case CommandStatus.B02Q:
                        _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)  
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            this.IsB02ThreadRunCtrl = false;
                            this.IsB02Repeat = false;
                            UIControlStatus(UITempControPackets, false);
                            UIControlStatus(UIControPackets, true);
                            VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "EPC(Q) command timeout(2000ms)." :
                                "EPC(Q)指令超時(2000ms)", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
        }

        private void DoB02SinglePreSelectAccessQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;
            if (!DoB02SelectWork(CommandStatus.B02T))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Select(T) command timeout(2048ms) or parameter error." :
                    "Select(T)指令超時(2048ms)或參數錯誤。", true);
                goto B02SinglePreSelectAccessQREXIT;
            }
            else
            {
                _index = 0;
                VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

                if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                    while (IsReceiveDataWork)
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            IsReceiveDataWork = false;
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                            goto B02SinglePreSelectAccessQREXIT;
                        }
                    }

                switch (DoFakeProcess)
                {
                    case CommandStatus.B02QR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02QR);
                        break;
                    case CommandStatus.B02Q:
                        _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)  
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "EPC(Q) command timeout(2000ms)." :
                                "EPC(Q)指令超時(2000ms)", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
            B02SinglePreSelectAccessQREXIT: this.IsRunning = false;
        }

        private void DoB02SinglePreSelectQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;
            if (!DoB02SelectWork(CommandStatus.B02T))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Select(T) command timeout(2000ms) or parameter error." :
                    "Select(T)指令超時(2000ms)或參數錯誤。", true);
                goto B02SinglePreSelectQREXIT;
            }
            else
            { 
                switch (DoFakeProcess)
                {
                    case CommandStatus.B02QR:
                        _callback = DoB02SendWork(
                            this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                            CommandStatus.B02QR);
                        break;
                    case CommandStatus.B02Q:
                        _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                        break;
                    default:
                        _callback = false;
                        break;
                }

                if (_callback)
                {
                    _index = 0;
                    while (IsReceiveDataWork)   
                    {
                        Thread.Sleep(10);
                        _index++;
                        if (_index >= 200)
                        {
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "EPC(Q) command timeout(2000ms)." :
                                "EPC(Q)指令超時(2000ms)", true);
                            IsReceiveDataWork = false;
                            break;
                        }
                    }
                }
            }
            B02SinglePreSelectQREXIT: this.IsRunning = false;
        }

        private void DoB02SinglePreAccessQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;
           
            VM.B02GroupPreSetAccessPassword = Format.MakesUpZero(VM.B02GroupPreSetAccessPassword, 8);

            if (DoB02SendWork(this.ReaderService.CommandP(VM.B02GroupPreSetAccessPassword), CommandStatus.B02P))
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        IsReceiveDataWork = false;
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Access(P) command timeout(2000ms)." : "Access(P)指令超時(2000ms)", true);
                        goto B02SinglePreAccessQREXIT;
                    }
                }

            switch (DoFakeProcess)
            {
                case CommandStatus.B02QR:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                        CommandStatus.B02QR);
                    break;
                case CommandStatus.B02Q:
                    _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                    break;
                default:
                    _callback = false;
                    break;
            }

            if (_callback)
            {
                _index = 0;
                while (IsReceiveDataWork)  
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "EPC(Q) command timeout(2000ms)." :
                            "EPC(Q)指令超時(2000ms)", true);
                        IsReceiveDataWork = false;
                        break;
                    }
                }
            }

            B02SinglePreAccessQREXIT: this.IsRunning = false;
        }

        private void DoB02SingleQRWork()
        {
            Int32 _index = 0;
            Boolean _callback = false;

            switch (DoFakeProcess)
            {
                case CommandStatus.B02QR:
                    _callback = DoB02SendWork(
                        this.ReaderService.CommandQR(VM.B02GroupPreSetReadMemBank.Tag, VM.B02GroupPreSetReadAddress, VM.B02GroupPreSetReadLength),
                        CommandStatus.B02QR);
                    break;
                case CommandStatus.B02Q:
                    _callback = DoB02SendWork(this.ReaderService.CommandQ(), CommandStatus.B02Q);
                    break;
                default:
                    _callback = false;
                    break;
            }

            if (_callback)
            {
                _index = 0;
                while (IsReceiveDataWork)   
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "EPC(Q) command timeout(2000ms)." :
                            "EPC(Q)指令超時(2000ms)", true);
                        IsReceiveDataWork = false;
                        break;
                    }
                }
            }
            this.IsRunning = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void B02ListBoxCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Add)
            {
                B02ListBox.ScrollIntoView(e.NewItems[0]);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02CheckBoxAccessPreCommandChecked(object sender, RoutedEventArgs e) { }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnB02GroupPreSetReadMemBankDownClosed(object sender, EventArgs e)
        {
			switch ((sender as ComboBox).SelectedIndex) {
				case 0: VM.B02GroupPreSetReadAddress = "0"; VM.B02GroupPreSetReadLength = "4"; break;
				case 1: VM.B02GroupPreSetReadAddress = "2"; VM.B02GroupPreSetReadLength = "6"; break;
				case 2: VM.B02GroupPreSetReadAddress = "0"; VM.B02GroupPreSetReadLength = "4"; break;
				case 3: VM.B02GroupPreSetReadAddress = "0"; VM.B02GroupPreSetReadLength = "1"; break;
				case 4: VM.B02GroupPreSetReadAddress = ""; VM.B02GroupPreSetReadLength = ""; break;
				case 5: VM.B02GroupPreSetReadAddress = "0"; VM.B02GroupPreSetReadLength = "2"; break;
				case 6: VM.B02GroupPreSetReadAddress = "2"; VM.B02GroupPreSetReadLength = "2"; break;
			}
		}

        /// <summary>
        /// Multi read button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02GroupUButtonClick(object sender, RoutedEventArgs e) {
			MessageShow(String.Empty, false);

			if (VM.B02GroupReadCtrlCheckBoxIsChecked) {

                if ((ValidationStates)ValidationState["B02GroupPreSetReadAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Address is null or over range: 0 ~ 0x3FFF" :
                        "讀取位址為空或超出規範值: 0 ~ 0x3FFF", true);
                    this.B02GroupPreSetReadAddress.Focus();
                    goto B02MultiUEXIT;
                }

                if ((ValidationStates)ValidationState["B02GroupPreSetReadAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Length is null or over range: 1 ~ 0x20" :
                        "讀取長度為空或超出規範值:1 ~ 0x20", true);
                    this.B02GroupPreSetReadLength.Focus();
                    goto B02MultiUEXIT;
                }
			}


			if (!IsB02Repeat) {	
				if (B02GroupPreSetURepeat.IsChecked.Value)
                {
                    if (this.B02GroupPreSetSelectCheckBox.IsChecked.Value)
                    {
                        if (DoB02SelectInfoCheck())
                        {
                            if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                            {
                                if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                {
                                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                        "Access(P) command is pre-processed, the field is null" :
                                        "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                    this.IsFocus = true;
                                    this.B02GroupPreSetAccessPassword.Focus();
                                    goto B02MultiUEXIT;
                                }

                                this.IsB02Repeat = true;
                                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                //this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                UITempControPackets.Clear();
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                                UIControlStatus(UITempControPackets, true);
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02UR;
                                }
                                else
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02U;
                                }

                                this.IsB02ThreadRunCtrl = true;
                                this.SettingThread = new Thread(DoB02PreSelectAccessURWork) {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                            else
                            {
                                this.IsB02Repeat = true;
                                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                //this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                UITempControPackets.Clear();
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                                UIControlStatus(UITempControPackets, true);
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02UR;
                                }
                                else
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02U;
                                }

                                this.IsB02ThreadRunCtrl = true;
                                this.SettingThread = new Thread(DoB02PreSelectURWork) {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                        }
                    }
                    else//PreSetSelect = false
                    {
                        if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                        {
                            if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.B02GroupPreSetAccessPassword.Focus();
                                goto B02MultiUEXIT;
                            }

                            this.IsB02Repeat = true;
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            //this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            UITempControPackets.Clear();
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                            UIControlStatus(UITempControPackets, true);

                            if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                            {
                                if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                    this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                else
                                    this.DoFakeProcess = CommandStatus.B02UR;
                            }
                            else
                            {
                                if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                    this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                else
                                    this.DoFakeProcess = CommandStatus.B02U;
                            }
                            this.IsB02ThreadRunCtrl = true;
                            this.SettingThread = new Thread(DoB02PreAccessURWork) {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.IsB02Repeat = true;
                            VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            //this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            UITempControPackets.Clear();
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                            UIControlStatus(UITempControPackets, true);

                            if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                            {
                                if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                    this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                else
                                    this.DoFakeProcess = CommandStatus.B02UR;
                            }
                            else
                            {
                                if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                    this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                else
                                    this.DoFakeProcess = CommandStatus.B02U;
                            }
                            this.IsB02ThreadRunCtrl = true;
                            this.SettingThread = new Thread(DoB02URWork) {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        } 
                    }

                    VM.B02GroupRecordTextBlockTagCount = "0";
                    VM.B02GroupRecordTextBlockTimeAvgCount = "0";
                    this.B02Item02CommandRunTimesCount = 0.0d;
                    //this.B02ListViewTagTimeCount = 0;
                    this.B02ListViewTagCount = 0;
                    
                    //this.B02TagsTime.Start();

                }
                else//PreSetRepeat = false
                {
                    if (!this.IsRunning)
                    {
                        this.IsRunning = true;

                        if (this.B02GroupPreSetSelectCheckBox.IsChecked.Value)
                        {
                            if (DoB02SelectInfoCheck())
                            {
                                if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                                {
                                    if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                    {
                                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                            "Access(P) command is pre-processed, the field is null" :
                                            "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                        this.IsFocus = true;
                                        this.IsRunning = false;
                                        this.B02GroupPreSetAccessPassword.Focus();
                                        goto B02MultiUEXIT;
                                    }

                                    if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                    {
                                        if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                            this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                        else
                                            this.DoFakeProcess = CommandStatus.B02UR;
                                    }
                                    else
                                    {
                                        if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                            this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                        else
                                            this.DoFakeProcess = CommandStatus.B02U;
                                    }
                                    this.SettingThread = new Thread(DoB02SinglePreSelectAccessURWork)
                                    {
                                        IsBackground = true
                                    };
                                    this.SettingThread.Start();
                                }
                                else
                                {
                                    if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                    {
                                        if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                            this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                        else
                                            this.DoFakeProcess = CommandStatus.B02UR;
                                    }
                                    else
                                    {
                                        if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                            this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                        else
                                            this.DoFakeProcess = CommandStatus.B02U;
                                    }
                                    this.SettingThread = new Thread(DoB02SinglePreSelectURWork)
                                    {
                                        IsBackground = true
                                    };
                                    this.SettingThread.Start();
                                }

                            }
                        }
                        else//PreSetSelect = false
                        {
                            if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                            {
                                if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                {
                                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                        "Access(P) command is pre-processed, the field is null" :
                                        "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                    this.IsFocus = true;
                                    this.IsRunning = false;
                                    this.B02GroupPreSetAccessPassword.Focus();
                                    goto B02MultiUEXIT;
                                }

                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02UR;
                                }
                                else
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02U;
                                }
                                this.SettingThread = new Thread(DoB02SinglePreAccessURWork)
                                {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                            else
                            {
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02URSLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02UR;
                                }
                                else
                                {
                                    if (VM.B02GroupUSlotQCheckBoxIsChecked)
                                        this.DoFakeProcess = CommandStatus.B02USLOTQ;
                                    else
                                        this.DoFakeProcess = CommandStatus.B02U;
                                }
                                this.SettingThread = new Thread(DoB02SingleURWork)
                                {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                        }
                    }                   
				}
			}
			else {
                //if (this.B02TagsTime.IsEnabled)
                //    this.B02TagsTime.Stop();
                this.IsB02Repeat = false;
				UIControlStatus(UITempControPackets, false);
				UIControlStatus(UIControPackets, true);
                VM.B02GroupUButton = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                //this.B02GroupUButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Multi(U)" : "讀取(U)";
                this.IsB02ThreadRunCtrl = false;
            }
		B02MultiUEXIT: ;
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02GroupQButtonClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            this.B02ListViewTagCount = 0;

            if (VM.B02GroupReadCtrlCheckBoxIsChecked)
            {
                if ((ValidationStates)ValidationState["B02GroupPreSetReadAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Address is null or over range: 0 ~ 0x3FFF" :
                        "讀取位址為空或超出規範值: 0 ~ 0x3FFF", true);
                    this.B02GroupPreSetReadAddress.Focus();
                    goto B02EPCEXIT;
                }

                if ((ValidationStates)ValidationState["B02GroupPreSetReadAddress"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Length is null or over range: 1 ~ 0x20" :
                        "讀取長度為空或超出規範值:1 ~ 0x20", true);
                    this.B02GroupPreSetReadLength.Focus();
                    goto B02EPCEXIT;
                }
            }

            if (!IsB02Repeat)
            {
                if (B02GroupPreSetQRepeat.IsChecked.Value)
                {
                    if (this.B02GroupPreSetSelectCheckBox.IsChecked.Value)
                    {
                        if (DoB02SelectInfoCheck())
                        {
                            if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                            {
                                if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                {
                                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                        "Access(P) command is pre-processed, the field is null" :
                                        "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                    this.IsFocus = true;
                                    this.B02GroupPreSetAccessPassword.Focus();
                                    goto B02EPCEXIT;
                                }

                                this.IsB02Repeat = true;
                                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                //this.B02GroupQButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                UITempControPackets.Clear();
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                                UIControlStatus(UITempControPackets, true);
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    this.DoFakeProcess = CommandStatus.B02QR;
                                }
                                else
                                {
                                    this.DoFakeProcess = CommandStatus.B02Q;
                                }

                                this.IsB02ThreadRunCtrl = true;
                                this.SettingThread = new Thread(DoB02PreSelectAccessQRWork) {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                            else
                            {
                                this.IsB02Repeat = true;
                                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                //this.B02GroupQButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                                UITempControPackets.Clear();
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_U, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                                UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                                UIControlStatus(UITempControPackets, true);
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked) {
                                   this.DoFakeProcess = CommandStatus.B02QR;
                                }
                                else {
                                    this.DoFakeProcess = CommandStatus.B02Q;
                                }

                                this.IsB02ThreadRunCtrl = true;
                                this.SettingThread = new Thread(DoB02PreSelectQRWork) {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                        }
                    }
                    else//PreSetSelect = false
                    {
                        if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                        {
                            if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                            {
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    "Access(P) command is pre-processed, the field is null" :
                                    "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                this.IsFocus = true;
                                this.B02GroupPreSetAccessPassword.Focus();
                                goto B02EPCEXIT;
                            }

                            this.IsB02Repeat = true;
                            VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            //this.B02GroupQButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            UITempControPackets.Clear();
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                            UIControlStatus(UITempControPackets, true);

                            if (VM.B02GroupReadCtrlCheckBoxIsChecked) {
                                this.DoFakeProcess = CommandStatus.B02QR;
                            }
                            else {
                                this.DoFakeProcess = CommandStatus.B02Q;
                            }
                            this.IsB02ThreadRunCtrl = true;
                            this.SettingThread = new Thread(DoB02PreAccessQRWork) {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                        else
                        {
                            this.IsB02Repeat = true;
                            VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            //this.B02GroupQButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "Stop" : "停止讀取";
                            UITempControPackets.Clear();
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_SELECT, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_ACCESS, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02PRESET_CONTINUE_Q, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02READCTRL, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_U, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_CLR, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02BTN_SAVE, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02SlotQ, false));
                            UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                            UIControlStatus(UITempControPackets, true);

                            if (VM.B02GroupReadCtrlCheckBoxIsChecked) {
                                this.DoFakeProcess = CommandStatus.B02QR;
                            }
                            else {
                                this.DoFakeProcess = CommandStatus.B02Q;
                            }
                            this.IsB02ThreadRunCtrl = true;
                            this.SettingThread = new Thread(DoB02QRWork) {
                                IsBackground = true
                            };
                            this.SettingThread.Start();
                        }
                    }
                }
                else//PreSetRepeat = false
                {
                    if (!this.IsRunning)
                    {
                        this.IsRunning = true;

                        if (this.B02GroupPreSetSelectCheckBox.IsChecked.Value)
                        {
                            if (DoB02SelectInfoCheck())
                            {
                                if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                                {
                                    if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                    {
                                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                            "Access(P) command is pre-processed, the field is null" :
                                            "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                        this.IsFocus = true;
                                        this.IsRunning = false;
                                        this.B02GroupPreSetAccessPassword.Focus();
                                        goto B02EPCEXIT;
                                    }

                                    if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                    {
                                        this.DoFakeProcess = CommandStatus.B02QR;
                                    }
                                    else
                                    {
                                        this.DoFakeProcess = CommandStatus.B02Q;
                                    }
                                    this.SettingThread = new Thread(DoB02SinglePreSelectAccessQRWork)
                                    {
                                        IsBackground = true
                                    };
                                    this.SettingThread.Start();
                                }
                                else
                                {
                                    if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                    {
                                        this.DoFakeProcess = CommandStatus.B02QR;
                                    }
                                    else
                                    {
                                        this.DoFakeProcess = CommandStatus.B02Q;
                                    }
                                    this.SettingThread = new Thread(DoB02SinglePreSelectQRWork)
                                    {
                                        IsBackground = true
                                    };
                                    this.SettingThread.Start();
                                }

                            }
                        }
                        else//PreSetSelect = false
                        {
                            if (B02GroupPreSetAccessCheckBox.IsChecked.Value)
                            {
                                if (String.IsNullOrEmpty(this.B02GroupPreSetAccessPassword.Text))
                                {
                                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                        "Access(P) command is pre-processed, the field is null" :
                                        "Access(P)指令已設為預處理指令，密碼欄位不得為空", true);
                                    this.IsFocus = true;
                                    this.IsRunning = false;
                                    this.B02GroupPreSetAccessPassword.Focus();
                                    goto B02EPCEXIT;
                                }

                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    this.DoFakeProcess = CommandStatus.B02QR;
                                }
                                else
                                {
                                    this.DoFakeProcess = CommandStatus.B02Q;
                                }
                                this.SettingThread = new Thread(DoB02SinglePreAccessQRWork)
                                {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                            else
                            {
                                if (VM.B02GroupReadCtrlCheckBoxIsChecked)
                                {
                                    this.DoFakeProcess = CommandStatus.B02QR;
                                }
                                else
                                {
                                    this.DoFakeProcess = CommandStatus.B02Q;
                                }
                                this.SettingThread = new Thread(DoB02SingleQRWork)
                                {
                                    IsBackground = true
                                };
                                this.SettingThread.Start();
                            }
                        }
                    }

                    
                }
            }
            else
            {
                this.IsB02Repeat = false;
                UIControlStatus(UITempControPackets, false);
                UIControlStatus(UIControPackets, true);
                VM.B02GroupQButton = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)" : "讀取(Q)";
                //this.B02GroupQButton.Content = (this.Culture.IetfLanguageTag == "en-US") ? "EPC(Q)": "讀取(Q)";
                this.IsB02ThreadRunCtrl = false;
            }
            B02EPCEXIT:;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnB02ButtonSaveClick(object sender, RoutedEventArgs e)
        {
            var _path = AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "log\\";
            var _file = "MultiOperatorSummary_" + DateTime.Now.ToString("yyyy-MM-dd", CultureInfo.CurrentCulture) + ".log";
            var _filePath = _path + _file;
            StreamWriter swStream;
			String str = String.Empty;

			if (File.Exists(_filePath))
				swStream = new StreamWriter(_filePath);
			else
            {
                try
                {
                    swStream = File.CreateText(_filePath);
                }
                catch (DirectoryNotFoundException)
                {
                    Directory.CreateDirectory(_path);
                    swStream = File.CreateText(_filePath);
                }
                
            }
				
            //title
            swStream.WriteLine("===============================================");
            swStream.WriteLine("欄位定義: PC,EPC,CRC16,Read Command,Count");
            swStream.WriteLine(String.Format(CultureInfo.CurrentCulture, "標籤張數: {0}", VM.B02ListViewItemsSource.Count));
            swStream.WriteLine(String.Format(CultureInfo.CurrentCulture, "執行次數: {0}", VM.B02GroupRecordTextBlockRunCount));
            swStream.WriteLine("===============================================");
            for (Int32 i = 0; i < VM.B02ListViewItemsSource.Count; i++) {
				str = VM.B02ListViewItemsSource[i].B02PC + ",\t" +
                      VM.B02ListViewItemsSource[i].B02EPC + ",\t" +
                      VM.B02ListViewItemsSource[i].B02CRC16 + ",\t" +
                      VM.B02ListViewItemsSource[i].B02Read + ",\t" +
                      VM.B02ListViewItemsSource[i].B02Count;
				swStream.WriteLine(str);
				str = String.Empty;
			}

            swStream.WriteLine("");
            swStream.Flush();
			swStream.Close();
			Process.Start("notepad.exe", _filePath);
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02ButtonClearClick(object sender, RoutedEventArgs e) {
			this.B02ListViewRunCount = 0;
            this.B02ListViewTagCount = 0;
            B02ListViewList.Clear();
            VM.B02GroupRecordTextBlockCount = String.Empty;
			VM.B02GroupRecordTextBlockRunCount = String.Empty;
            VM.B02GroupRecordTextBlockTagCount = String.Empty;
            VM.B02GroupRecordTextBlockTimeAvgCount = String.Empty;
            this.B02Item02CommandRunTimesCount = 0.0d;
            VM.B02ListViewItemsSource.Clear();
			//this.B02ListViewQU.Items.Clear();
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02ListBoxMenuItemCopyClick(object sender, RoutedEventArgs e)
        {
            String collectedText = String.Empty;
            Int32 idx = this.B02ListBox.SelectedIndex;
            Int32 count = B02ListBox.Items.Count;

            if (count == this.B02ListBox.SelectedItems.Count) idx = 0;
            for (Int32 i = 0; i < this.B02ListBox.SelectedItems.Count; i++ )
            {
                ListBoxItem vv = B02ListBox.Items[idx + i] as ListBoxItem;
                collectedText += vv.Content + "\r\n";
            }

            if (B01GroupMsgListBox.SelectedItems != null)
            {
                Clipboard.SetText(collectedText);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02ListBoxMenuItemDeleteRangeClick(object sender, RoutedEventArgs e) {
			object[] ob = new object[this.B02ListBox.SelectedItems.Count];
			this.B02ListBox.SelectedItems.CopyTo(ob, 0);

			foreach (object obj in ob)
				this.B02ListBox.Items.Remove(obj);
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02ListBoxMenuItemDeleteAllClick(object sender, RoutedEventArgs e) {
			this.B02ListBox.Items.Clear();
		}

        GridViewColumnHeader _B02lastHeaderClicked = null;
        ListSortDirection _B02lastDirection = ListSortDirection.Ascending;
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02ListViewQUHeaderClick(object sender, RoutedEventArgs e) {
            ListSortDirection direction;

            if (e.OriginalSource is GridViewColumnHeader headerClicked)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    if (headerClicked != _B02lastHeaderClicked)
                        direction = ListSortDirection.Ascending;
                    else
                    {
                        if (_B02lastDirection == ListSortDirection.Ascending)
                            direction = ListSortDirection.Descending;
                        else
                            direction = ListSortDirection.Ascending;
                    }

                    if ((Binding)headerClicked.Column.DisplayMemberBinding == null)
                        return;
                    String header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    Sort(header, direction, VM.B02ListViewItemsSource);

                    if (direction == ListSortDirection.Ascending)
                        headerClicked.Column.HeaderTemplate = Resources["HeaderTemplateArrowUp"] as DataTemplate;
                    else
                        headerClicked.Column.HeaderTemplate = Resources["HeaderTemplateArrowDown"] as DataTemplate;

                    if (_B02lastHeaderClicked != null && _B02lastHeaderClicked != headerClicked)
                        _B02lastHeaderClicked.Column.HeaderTemplate = null;

                    _B02lastHeaderClicked = headerClicked;
                    _B02lastDirection = direction;
                }
            }
        }


        /*private void DoB02TimeWork(object sender, EventArgs e)
        {

            //B02ListViewTagTimeCount++;
            //var avg = (Double)B02ListViewTagCount / B02ListViewTagTimeCount;
            //VM.B02GroupRecordTextBlockTimeAvgCount = avg.ToString("0.0");


            var avg1 = (Double)B02ListViewTagCount / B02Item02CommandRunTimesCount;
            VM.B02GroupRecordTextBlockTimeAvgCount = avg1.ToString("0.0");

        }*/


        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void B02TabControlItemSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (sender is TabControl tc)
            {
                B02TabCtrlIndex = tc.SelectedIndex;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnRadioButtonB02ItemChecked(object sender, RoutedEventArgs e)
        {
            var radioButton = sender as RadioButton;

            if (B02TabControlItem == null) return;

            switch (Convert.ToInt32(radioButton.Tag.ToString(), CultureInfo.CurrentCulture))
            {
                case 1:
                    B02TabControlItem.SelectedIndex = 0;
                    break;
                case 2:
                    B02TabControlItem.SelectedIndex = 1;
                    break;
            }
            MessageShow(String.Empty, false);
            
            DoProcess = CommandStatus.DEFAULT;

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02Item02SingleCommandSendButton(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            B02Item02Command _cmd = button.DataContext as B02Item02Command;

            if (String.IsNullOrEmpty(_cmd.Command))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Command is null." : "指令內容為空.", true);
            }
            else
            {
                MessageShow(String.Empty, false);
                var _thread = new Thread(new ThreadStart(() => {
                    DoB02SendWork(ReaderService.Custom(_cmd.Command), _cmd.CommandState);
                })) {
                    IsBackground = true
                };
                _thread.Start();
            }
        }

        private void DoB02Item02ProcessTimeUpWork (object sender, EventArgs e)
        {
            Int32 _index = 0;

            if (IsReceiveDataWork)   //SlotQ within U command, the received time will arrive to the max of 3s.
            {
                if (IsReceiveSubDataWork)
                {
                    _index = 0;
                    IsReceiveSubDataWork = false;
                }
                _index++;
                if (_index >= 300)
                {
                    IsReceiveDataWork = false;
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "command timeout(the last reception command interval is more than 3 seconds) and break process." :
                        "指令超時(最後接收的指令起，間隔超過3秒)，執行中斷。", true);

                    ((DispatcherTimer)sender).Stop();
                }
            }
            else
            {
                ((DispatcherTimer)sender).Stop();
            }
        }

        private void DoB02Item02ProcessWork(object sender, EventArgs e)
        {
            Boolean _callback = false;

            if (!IsB02Item02OnBtnClick)
            {
                this.B02Item02Process.Stop();
                UIControlStatus(UITempControPackets, false);
                return;
            }

            if (!this.IsReceiveDataWork)
            {
                if (B02Item02ProcessIdx == B02Item02ProcessCounts)
                {
                    this.IsB02Item02OnBtnClick = false;
                    this.B02Item02Process.Stop();
                    UIControlStatus(UITempControPackets, false);
                    return;
                }

                this.DoProcess = B02Item02ProcessItem[B02Item02ProcessIdx];
                _callback = DoB02SendWork(ReaderService.Custom(B02Item02CommandSets[B02Item02ProcessIdx]), this.DoProcess);
                if (_callback)
                {
                    B02Item02ProcessIdx++;
                    DispatcherTimer B02Item02ProcessTimeUp = new DispatcherTimer();
                    B02Item02ProcessTimeUp.Tick += new EventHandler(DoB02Item02ProcessTimeUpWork);
                    B02Item02ProcessTimeUp.Interval = TimeSpan.FromMilliseconds(10);
                    B02Item02ProcessTimeUp.Start();
                }
            }
        }

        private void OnB02Item02BtnOpenClick(object sender, RoutedEventArgs e)
        {
            var _openFileDialog = new OpenFileDialog {
                DefaultExt = ".xml",
                Filter = "Xml documents (.xml)|*.xml"
            };
            var _result = _openFileDialog.ShowDialog();
            if (_result == true)
            {
                B02Item02Commands.Clear();
                this.B02Item02ListBox.Items.Refresh();
                this.ProfileXml = new XmlFormat(_openFileDialog.FileName);


                var _section = this.ProfileXml.GetSectionNames();

                if (_section != null) {
                    for (int i = 0; i < _section.Length; i++) {
                        var _check = ProfileXml.GetValue(_section[i], "CHECK", "true");
                        var _type = ProfileXml.GetValue(_section[i], "TYPE", "false");
                        var _name = ProfileXml.GetValue(_section[i], "NAME", "");
                        var _commandstate = ProfileXml.GetValue(_section[i], "COMMANDSTATE", "0");
                        var _command = ProfileXml.GetValue(_section[i], "COMMAND", "");
                        var _idx = ProfileXml.GetValue(_section[i], "TABINDEX", "0");

                        B02Item02Commands.Add(new B02Item02Command(
                            (_check is "true") ? true : false,
                            (_type is "true") ? true : false,
                            _name,
                            (CommandStatus)Enum.ToObject(typeof(CommandStatus), Int32.Parse(_commandstate, CultureInfo.CurrentCulture)),
                            _command,
                            Int32.Parse(_idx, CultureInfo.CurrentCulture),
                            false));
                    }
                    B02Item02Commands.Add(new B02Item02Command());
                }
                else {
                    B02Item02Commands.Add(new B02Item02Command());
                }

                ProfileXmlName = System.IO.Path.GetFileName(_openFileDialog.FileName);


                
            }
            
        }

        private void OnB02Item02BtnSaveClick(object sender, RoutedEventArgs e)
        {
            if (B02Item02Commands.Count == 1)
            {
                MessageBox.Show("至少有一個指令");
                return;
            }

            var _saveFileDialog = new SaveFileDialog {
                FileName = ProfileXmlName,
                DefaultExt = ".xml",
                Filter = "Xml documents (.xml)|*.xml"
            };
            var _result = _saveFileDialog.ShowDialog();
            if (_result == true)
            {
                if (ProfileXmlName != null && ProfileXmlName.Equals(_saveFileDialog.FileName, StringComparison.CurrentCulture))
                {

                    if (ProfileXml == null)
                        ProfileXml = new XmlFormat(_saveFileDialog.FileName);

                    var sections = ProfileXml.GetSectionNames();

                    if (sections != null)
                    {
                        foreach (String _section in sections)
                            ProfileXml.RemoveSection(_section);
                    }

                    for (int i = 0; i < B02Item02Commands.Count - 1; i++)
                    {
                        var _section = i.ToString(CultureInfo.CurrentCulture);
                        ProfileXml.SetValue(_section, "CHECK", (B02Item02Commands[i].Check is true) ? "true" : "false");
                        ProfileXml.SetValue(_section, "TYPE", (B02Item02Commands[i].Type is true) ? "true" : "false");
                        ProfileXml.SetValue(_section, "NAME", B02Item02Commands[i].Name);
                        ProfileXml.SetValue(_section, "COMMANDSTATE", ((Int32)(B02Item02Commands[i].CommandState)).ToString(CultureInfo.CurrentCulture));
                        ProfileXml.SetValue(_section, "COMMAND", B02Item02Commands[i].Command);
                        ProfileXml.SetValue(_section, "TABINDEX", B02Item02Commands[i].TabIndex.ToString(CultureInfo.CurrentCulture));
                    }
                   
                }
                else
                {

                    this.ProfileXml = new XmlFormat(_saveFileDialog.FileName);
                    try
                    {
                        var sections = ProfileXml.GetSectionNames();

                        if (sections != null)
                        {
                            foreach (String _section in sections)
                                ProfileXml.RemoveSection(_section);
                        }

                        for (int i = 0; i < B02Item02Commands.Count - 1; i++)
                        {
                            var _section = i.ToString(CultureInfo.CurrentCulture);
                            ProfileXml.SetValue(_section, "CHECK", (B02Item02Commands[i].Check is true) ? "true" : "false");
                            ProfileXml.SetValue(_section, "TYPE", (B02Item02Commands[i].Type is true) ? "true" : "false");
                            ProfileXml.SetValue(_section, "NAME", B02Item02Commands[i].Name);
                            ProfileXml.SetValue(_section, "COMMANDSTATE", ((Int32)(B02Item02Commands[i].CommandState)).ToString(CultureInfo.CurrentCulture));
                            ProfileXml.SetValue(_section, "COMMAND", B02Item02Commands[i].Command);
                            ProfileXml.SetValue(_section, "TABINDEX", B02Item02Commands[i].TabIndex.ToString(CultureInfo.CurrentCulture));
                        }
                    }
                    catch (XPathException ex)
                    {
                        MessageShow(ex.Message, false);
                    }
                    finally
                    {
                        if (ProfileXml != null)
                            ProfileXml.Dispose();
                    }
                    
                }
            }

            /*String stFilePath = AppDomain.CurrentDomain.SetupInformation.ApplicationBase +
                                "\\log\\CommandSequence_" + DateTime.Now.ToString("yyyy-MM-dd-hh-mm-ss") + ".txt";
            StreamWriter swStream;

            if (File.Exists(stFilePath))
                swStream = new StreamWriter(stFilePath);
            else
                swStream = File.CreateText(stFilePath);
            //title
            for (Int32 i = 0; i < B02Item02Commands.Count - 1; i++)
            {
                swStream.WriteLine(String.Format(CultureInfo.CurrentCulture, "Name:{0}, \tCommand:{1}", B02Item02Commands[i].Name, B02Item02Commands[i].Command));
            }

            swStream.Flush();
            swStream.Close();
            Process.Start("notepad.exe", stFilePath);*/
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02Item02BtnClick(object sender, RoutedEventArgs e)
        {
            var _btn = sender as Button;

            if (!this.IsB02Item02OnBtnClick)
            {
                if (B02Item02Commands.Count == 1)
                    MessageBox.Show("至少有一個指令欄位");
                else
                {
                    B02Item02CommandSets.Clear();
                    B02Item02ProcessItem.Clear();
                    B02Item02ProcessCounts = 0;
                    B02Item02ProcessIdx = 0;


                    for (int i = 0; i < B02Item02Commands.Count -1; i++)
                    {
                        if (B02Item02Commands[i].Check)
                        {
                            B02Item02CommandSets.Add(B02Item02Commands[i].Command);
                            B02Item02ProcessItem.Add(B02Item02Commands[i].CommandState);
                            B02Item02ProcessCounts++;
                        }
                    }

                    UITempControPackets.Clear();
                    UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                    UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                    UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                    UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                    UITempControPackets.Add(new UIControl(GroupStatus.GB02TABCONTROLITEM, false));
                    UITempControPackets.Add(new UIControl(GroupStatus.GB02TABBTN, false));
                    UIControlStatus(UITempControPackets, true);

                    this.IsB02Item02OnBtnClick = true;
                    this.B02Item02Process.Start();
                }
            }
            else
            {
                //this.IsB02Item02OnBtnClick = false;
                //UIControlStatus(UITempControPackets, false);
            }
            
        }

        private ListBox DragSource = null;
        private void B02Item02ListBoxPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            ListBox parent = (ListBox)sender;
            DragSource = parent;
            object data = GetDataFromListBox(DragSource, e.GetPosition(parent));
            
            if (data != null)
            {
                DragDrop.DoDragDrop(parent, data, DragDropEffects.Move);
            }
        }

        private static object GetDataFromListBox(ListBox source, Point point)
        {
            if (source.InputHitTest(point) is UIElement element)
            {
                object data = DependencyProperty.UnsetValue;
                while (data == DependencyProperty.UnsetValue)
                {
                    data = source.ItemContainerGenerator.ItemFromContainer(element);

                    if (data == DependencyProperty.UnsetValue)
                    {
                        element = VisualTreeHelper.GetParent(element) as UIElement;
                    }

                    if (element == source)
                    {
                        return null;
                    }
                }

                if (data != DependencyProperty.UnsetValue)
                {
                    return data;
                }
            }

            return null;
        }

        private void EditCommandDialogReceiveValues(object sender, PassValuesEventArgs e)
        {
            this.B02Item02Commands[e.Index] = e.Command;
            this.B02Item02ListBox.Items.Refresh();
        }

        private void OnB02Item02ListBoxTemplateMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            
            if (e.ClickCount == 2)
            {
                this._EditCommandDialog = new EditCommandDialog(B02Item02Commands, B02Item02SelectIdx);
                this._EditCommandDialog.EditCommandPassValuesEventHandler += new EditCommandDialog.PassValuesEventHandler(EditCommandDialogReceiveValues);
                this._EditCommandDialog.Owner = this;
                this._EditCommandDialog.ShowInTaskbar = false;
                this._EditCommandDialog.ShowDialog();
                
                if (_EditCommandDialog.DialogResult.HasValue && _EditCommandDialog.DialogResult.Value)
                {
                    this.B02Item02Commands = _EditCommandDialog.GetSequences();
                }
                this.B02Item02ListBox.Items.Refresh();
                this._EditCommandDialog.Close();
            } 
        }

        private void OnB02Item02ListBoxMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {

            if (e.ClickCount == 2)
            {
                ListBox parent = (ListBox)sender;
                B02Item02SelectIdx = parent.SelectedIndex;
            }
        }

        private void OnB02Item02ListBoxMenuItemDeleteRangeClick(object sender, RoutedEventArgs e)
        {
            if (this.B02Item02ListBox.SelectedIndex != B02Item02Commands.Count - 1)
                B02Item02Commands.Remove(this.B02Item02ListBox.SelectedItem as B02Item02Command);
        }

        private void OnB02ButtonSaveMouseMove(object sender, MouseEventArgs e)
        {
            B02SavePop.IsOpen = true;
        }

        private void OnB02ButtonSaveMouseLeave(object sender, MouseEventArgs e)
        {
            B02SavePop.IsOpen = false;
        }

        /// <summary>
        /// Log display message to file if checked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB02CheckBoxGroupMsgLogChecked(object sender, RoutedEventArgs e)
        {
            if (B02MessageLogger == null)
            {
                B02MessageLogger = LogManager.GetLogger(typeof(MainWindow));
                XmlConfigurator.Configure(new FileInfo("./logConfig/log4MessageRaw.config"));

            }
        }
        #endregion


        #region === #Pager03 ===
        private void B03PollingButton(Boolean b)
        {
            VM.B03GroupTagWindowButtonRunIsEnabled = b;
            VM.B03GroupTagWindowButtonBattAlarmTempIsEnabled = b;
            VM.B03GroupTagWindowButtonBattVoltIsEnabled = b;

        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
        private void B03DisplayStatisticsMsg(String _str)
        {
            String[] _data = _str.Split(',');

            if (_str.Length >= 8)
            {
                if (Format.CRC16(Format.HexStringToBytes(_data[0])) != 0x1D0F) return;
                    
                if (_data[1].Length <= 1) return;

                var _tagRecord = new B03ListViewItem()
                {
                    B03ListViewTagWindowLightsTimes = String.Empty,
                    B03ListViewTagWindowLights = false,
                    B03ListViewTagWindowData = (_data.Length > 1) ? _data[1].Replace("R", String.Empty) : String.Empty,
                    B03ListViewTagWindowSelCheck = false
                };
                VM.B03ListViewAddNewItem(_tagRecord);
                    //this.B03ListViewTagWindow.Items.Add(_tagRecord);
                    //this.B03ListViewTagWindow.Items.Refresh();
            }

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="command"></param>
        /// <param name="process"></param>
		private Boolean DoB03SendReceiveWork(Byte[] command, CommandStatus process)
        {
            if (!IsReceiveDataWork)
            {
                IsReceiveDataWork = true;
                this.DoProcess = process;
                try
                {
                    B03DisplayInfoMsg("TX", Format.BytesToString(command), false);
                    switch (_ConnectType)
                    {
                        default:
                        case ReaderService.ConnectType.DEFAULT:
                            break;
                        case ReaderService.ConnectType.COM:
                            this._ICOM.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.USB:
                            this._IUSB.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.NET:
                            this._INet.Send(command, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.BLE:
                            this._IBLE.Send(command, ReaderModule.CommandType.Normal);
                            break;
                    }
                }
                catch (ArgumentNullException ane)
                {
                    MessageShow(ane.Message, false);
                    return false;
                }
                catch (InvalidOperationException ex)
                {
                    MessageShow(ex.Message, false);
                    return false;
                }
                catch (SocketException se)
                {
                    MessageShow(se.Message, false);
                    return false;
                }

                return true;
            }
            return false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="o"></param>
        /*private static void DoB03TagRunLightWork(object sender, EventArgs e, B03ListViewItem item)
        {
            item.Times--;
            item.B03ListViewTagWindowLightsTimes = item.Times.ToString(CultureInfo.CurrentCulture);

            if(item.Times == 0)
            {
                item.B03ListViewTagWindowLights = false;
                (sender as System.Timers.Timer).Enabled = false;
            }
        }*/

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        private void DoB03TagPollingRunLightWork(object sender)
        {
            var item = sender as B03ListViewItem;
            while (true)
            {
                Thread.Sleep(1000);

                item.Times--;
                item.B03ListViewTagWindowLightsTimes = item.Times.ToString(CultureInfo.CurrentCulture);
                
                if (item.Times == 0)
                {
                    item.B03ListViewTagWindowLights = false;
                    break;
                }
            }
            
        }

        /// <summary>
        /// 
        /// </summary>
        private void DoB03TagGetWork()
        {
            Int32 _index = 0;
            Boolean _f = false;

            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
				"Pre-Select EM Tag: TID E280B04" : 
				"Pre-Select EM標籤: TID E280B04", false);
            if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "0", "1A", "E280B04"), CommandStatus.B03SELECT))
            {
                _f = true;
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Select command timeout or parameter error." : 
					"Select指令超時或參數錯誤。", true);
                return;
            }

            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
			   "Send UR command" : 
			   "執行UR指令", false);
            DoB03SendReceiveWork(this.ReaderService.CommandUR(null, "2", "3", "3"), CommandStatus.B03GET);
        }
             
        /// <summary>
        /// 
        /// </summary>
        private void DoB03TagLEDFlashWork()
        {
            Int32 _index = 0;
            Boolean _f = false;

            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), CommandStatus.B03SELECT))
                    {
                        _f = true;
                        while (IsReceiveDataWork)
                        {
                            Thread.Sleep(4);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }
                    }

                    if (!_f)
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Select command timeout or parameter error.":
                            "Select指令超時或參數錯誤。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }

                    String _time = Format.MakesUpZero(VM.B03GroupTagWindowTime, 2);
                    EMFlashTime = Format.HexStringToInt(_time);

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.EMFlash(_time), CommandStatus.CUSTOMEM))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //1000ms timeout
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }

                        if (!_f)
                        {
                            VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                "EM flash command timeout." :
                                "EM LED flash指令超時。";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                String.Format(CultureInfo.CurrentCulture, "EM flash command timeout. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                String.Format(CultureInfo.CurrentCulture, "EM LED flash指令超時。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        }
                        else {
                            if (_time.Equals(EMTemp, StringComparison.CurrentCulture)) {
                                if (!VM.B03ListViewItemsSource[j].B03ListViewTagWindowLights)
                                {
                                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowLights = true;
                                    VM.B03ListViewItemsSource[j].Times = EMFlashTime;
                                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowLightsTimes = EMFlashTime.ToString(CultureInfo.CurrentCulture);

                                    var _thread = new Thread(new ParameterizedThreadStart(DoB03TagPollingRunLightWork)) {
                                        IsBackground = true
                                    };
                                    _thread.Start(VM.B03ListViewItemsSource[j]);
                                }
                                else
                                {
                                    //lock(VM.B03ListViewItemsSource[j].B03ListViewTagWindowLightsTimes)
                                    //{
                                        VM.B03ListViewItemsSource[j].Times = EMFlashTime;
                                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowLightsTimes = EMFlashTime.ToString(CultureInfo.CurrentCulture);
                                    //}  
                                }
                            } 
                            else
                            {
                                VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                    "The received telegrams is not same as transmission." :
                                    "接收的flash second與傳送的不同。";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "The received telegrams is not same as transmission. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                    String.Format(CultureInfo.CurrentCulture, "接收的flash second與傳送的不同 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                            }
                        }
                    }
                    else
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "EM LED flash command send fail." :
                            "EM LED flash傳送失敗。 [{0}]";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "EM LED flash command send fail. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "EM LED flash傳送失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                    }
                }
            }
            IsB03EMWork = false;
            B03PollingButton(true);
        }

        /// <summary>
        /// 
        /// </summary>
        private void DoB03TagBattAlarmTempWork()
        {
            Boolean _f = false;
            Int32 _index = 0;
            String _battAlarmTemp = String.Empty;

            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    VM.B03ListViewItemsSource[j].B03ListViewBattAlarmTemp = String.Empty;
                }   
            }

            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    //VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), CommandStatus.B03SELECT))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //400ms timeout
                        {
                            Thread.Sleep(4);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }
                    }

                    if (!_f)
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Select command timeout or parameter error.":
                            "Select指令超時或參數錯誤。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandW("3", "100", "1", "0000"), CommandStatus.B03WRITE))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //1000ms timeout
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }
                    }

                    if (!_f)
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Write command timeout.":
                            "Write指令寫入失敗。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Write command timeout . [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Write指令寫入失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }

                    if (!EMTemp.Equals("W<OK>", StringComparison.CurrentCulture))
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Write command no success." :
                            "Write指令寫入失敗。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Write command no success. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Write指令寫入失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), CommandStatus.B03SELECT))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //400ms timeout
                        {
                            Thread.Sleep(4);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }
                    }

                    if (!_f)
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Select command timeout or parameter error." :
                            "Select指令超時或參數錯誤。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }


                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandR("3", "100", "1"), CommandStatus.B03READ))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //1000ms timeout
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }

                        if (!_f)
                        {
                            VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                "EM Battery Alarm and Temperature command timeout.":
                                "EM Battery Alarm and Temperature指令超時。";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command timeout. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature指令超時。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                            continue;
                        }
                        else
                        {
                            if (EMTemp.Length == 4)
                            {
                                var _ba = Format.HexStringToBytes(EMTemp);
                                Int32 _degree = _ba[1];
                                if ((_ba[0] & 0x80) > 0)
                                    _battAlarmTemp = "Low / ";
                                else _battAlarmTemp = "OK / ";

                                if ((_ba[0] & 0x01) > 0)
                                    _degree -= 256;

                                _battAlarmTemp += (_degree * 0.25).ToString("##.00", CultureInfo.CurrentCulture);
                                VM.B03ListViewItemsSource[j].B03ListViewBattAlarmTemp = _battAlarmTemp;
                            }
                            else
                            {
                                VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                    "EM Battery Alarm and Temperature command receive fail.":
                                    "EM Battery Alarm and Temperature接收失敗。";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command receive fail. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                    String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature接收失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewBattAlarmTemp = "N/A";
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "EM Battery Alarm and Temperature command send fail.":
                            "EM Battery Alarm and Temperature傳送失敗。 [{0}]";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command send fail. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature傳送失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                    }
                        
                }
            }//for
            IsB03EMVoltTempWork = false;
            B03PollingButton(true);
        }

        /// <summary>
        /// 
        /// </summary>
        private void DoB03TagBattVoltWork()
        {
            Boolean _f = false;
            Int32 _index = 0;
            String _battAlarmTemp = String.Empty;

            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    VM.B03ListViewItemsSource[j].B03ListViewBattVolt = String.Empty;
                }
            }

            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    //VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    

                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), CommandStatus.B03SELECT))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //400ms timeout
                        {
                            Thread.Sleep(4);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }
                    }

                    if (!_f)
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Select command timeout or parameter error." :
                            "Select指令超時或參數錯誤。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                        continue;
                    }


                    _f = false; _index = 0;
                    if (DoB03SendReceiveWork(this.ReaderService.EMGetBattVolt(), CommandStatus.CUSTOMEM))
                    {
                        _f = true;
                        while (IsReceiveDataWork)   //1000ms timeout
                        {
                            Thread.Sleep(10);
                            _index++;
                            if (_index >= 100)
                            {
                                _f = false;
                                break;
                            }
                        }

                        if (!_f)
                        {
                            VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                "EM Battery Voltage command timeout." :
                                "EM Battery Voltage指令超時。";
                            MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage command timeout. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage指令超時。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                            continue;
                        }
                        else
                        {
                            if (EMTemp.Length == 4)
                            {
                                var _ba = Format.HexStringToInt(EMTemp);

                                if (_ba == 0)
                                    _battAlarmTemp = "0";
                                else
                                    _battAlarmTemp = (0x00100000 / _ba).ToString("0", CultureInfo.CurrentCulture);
                                VM.B03ListViewItemsSource[j].B03ListViewBattVolt = _battAlarmTemp;
                            }
                            else
                            {
                                VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                                    "EM Battery Voltage command receive fail." :
                                    "EM Battery Voltage接收失敗。";
                                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                    String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage command receive fail. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                                    String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage接收失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                                continue;
                            }
                        }    
                    }
                    else
                    {
                        VM.B03ListViewItemsSource[j].B03ListViewBattVolt = "N/A";
                        VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "EM Battery Voltage command send fail." :
                            "EM Battery Voltage傳送失敗。 [{0}]";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "EMBattery Voltage command send fail. [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage傳送失敗。 [{0}]", VM.B03ListViewItemsSource[j].B03ListViewTagWindowData), true);
                    }
                }
            }//for
            IsB03EMVoltTempWork = false;
            B03PollingButton(true);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        private void DoB03ListViewItemLEDFlashClickWork(object obj)
        {
            var item = obj as B03ListViewItem;
            Int32 _index = 0;
            Boolean _f = false;

            item.B03ListViewTagWindowStatus = String.Empty;

            if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", item.B03ListViewTagWindowData), CommandStatus.B03SELECT))
            {
                _f = true;
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "Select command timeout or parameter error." :
                            "Select指令超時或參數錯誤。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMWork = false;
                return;
            }

            String _time = Format.MakesUpZero(VM.B03GroupTagWindowTime, 2);
            EMFlashTime = Format.HexStringToInt(_time);

            _f = false; _index = 0;
            if (DoB03SendReceiveWork(this.ReaderService.EMFlash(_time), CommandStatus.CUSTOMEM))
            {
                _f = true;
                while (IsReceiveDataWork)   //1000ms timeout
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }

                if (!_f)
                {
                    item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                        "EM LED flash command timeout." :
                        "EM LED flash指令超時。";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        String.Format(CultureInfo.CurrentCulture, "EM LED flash command timeout. [{0}]", item.B03ListViewTagWindowData) :
                        String.Format(CultureInfo.CurrentCulture, "EM LED flash指令超時。 [{0}]", item.B03ListViewTagWindowData), true);
                }
                else
                {
                    if (_time.Equals(EMTemp, StringComparison.CurrentCulture))
                    {
                        if (!item.B03ListViewTagWindowLights)
                        {
                            item.B03ListViewTagWindowLights = true;
                            item.Times = EMFlashTime;
                            item.B03ListViewTagWindowLightsTimes = EMFlashTime.ToString(CultureInfo.CurrentCulture);

                            B03LightTimeEvent.Elapsed += (sender, e) => {
                                //DoB03TagRunLightWork(sender, e, item); 
                                item.Times--;
                                item.B03ListViewTagWindowLightsTimes = item.Times.ToString(CultureInfo.CurrentCulture);

                                if (item.Times == 0)
                                {
                                    item.B03ListViewTagWindowLights = false;
                                    (sender as System.Timers.Timer).Enabled = false;
                                }
                            };
                            B03LightTimeEvent.Interval = 1000;
                            B03LightTimeEvent.Enabled = true;
                            //_lightTimeEvent.Start();

                            
                        }
                        else
                        {
                            item.Times = EMFlashTime;
                            item.B03ListViewTagWindowLightsTimes = EMFlashTime.ToString(CultureInfo.CurrentCulture);
                        }
                    }
                    else
                    {
                        item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "The received telegrams is not same as transmission." :
                            "接收的flash second與傳送的不同。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "The received telegrams is not same as transmission. [{0}]", item.B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "接收的flash second與傳送的不同 [{0}]", item.B03ListViewTagWindowData), true);
                    }
                }
            }
            else
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "EM LED flash command send fail." :
                    "EM LED flash傳送失敗。 [{0}]";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "EM LED flash command send fail. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "EM LED flash傳送失敗。 [{0}]", item.B03ListViewTagWindowData), true);
            }


            IsB03EMWork = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        private void DoB03ListViewItemBattAlarmTempClickWork(object obj)
        {
            var item = obj as B03ListViewItem;
            Int32 _index = 0;
            Boolean _f = false;
            String _battAlarmTemp = String.Empty;

            item.B03ListViewTagWindowStatus = String.Empty;
            item.B03ListViewBattAlarmTemp = String.Empty;


            if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", item.B03ListViewTagWindowData), CommandStatus.B03SELECT))
            {
                _f = true;
                while (IsReceiveDataWork)   //400ms timeout
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "Select command timeout or parameter error." :
                    "Select指令超時或參數錯誤。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMVoltTempWork = false;
                return;
            }


            _f = false; _index = 0;
            if (DoB03SendReceiveWork(this.ReaderService.CommandW("3", "100", "1", "0000"), CommandStatus.B03WRITE))
            {
                _f = true;
                while (IsReceiveDataWork)   //1000ms timeout
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "Write command timeout." :
                    "Write指令超時。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Write command timeout. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Write指令超時。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMVoltTempWork = false;
                return;
            }

            if (!EMTemp.Equals("W<OK>", StringComparison.CurrentCulture))
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "Write command no success." :
                    "Write指令寫入失敗。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Write command no success. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Write指令寫入失敗。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMVoltTempWork = false;
                return;
            }

            _f = false; _index = 0;
            if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", item.B03ListViewTagWindowData), CommandStatus.B03SELECT))
            {
                _f = true;
                while (IsReceiveDataWork)   //400ms timeout
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "Select command timeout or parameter error." :
                    "Select指令超時或參數錯誤。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMVoltTempWork = false;
                return;
            }


            _f = false; _index = 0;
            if (DoB03SendReceiveWork(this.ReaderService.CommandR("3", "100", "1"), CommandStatus.B03READ))
            {
                _f = true;
                while (IsReceiveDataWork)   //1000ms timeout
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }

                if (!_f)
                {
                    item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                        "EM Battery Alarm and Temperature command timeout." :
                        "EM Battery Alarm and Temperature指令超時。";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command timeout. [{0}]", item.B03ListViewTagWindowData) :
                        String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature指令超時。 [{0}]", item.B03ListViewTagWindowData), true);
                    IsB03EMVoltTempWork = false;
                    return;
                }
                else
                {
                    if (EMTemp.Length == 4)
                    {
                        var _ba = Format.HexStringToBytes(EMTemp);
                        Int32 _degree = _ba[1];
                        if ((_ba[0] & 0x80) > 0)
                            _battAlarmTemp = "Low / ";
                        else _battAlarmTemp = "OK / ";

                        if ((_ba[0] & 0x01) > 0)
                            _degree -= 256;

                        _battAlarmTemp += (_degree * 0.25).ToString("##.00", CultureInfo.CurrentCulture);
                        item.B03ListViewBattAlarmTemp = _battAlarmTemp;
                    }
                    else
                    {
                        item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "EM Battery Alarm and Temperature command receive fail." :
                            "EM Battery Alarm and Temperature接收失敗。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command receive fail. [{0}]", item.B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature接收失敗。 [{0}]", item.B03ListViewTagWindowData), true);
                        IsB03EMVoltTempWork = false;
                        return;
                    }
                }
            }
            else
            {
                item.B03ListViewBattAlarmTemp = "N/A";
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "EM Battery Alarm and Temperature command send fail." :
                    "EM Battery Alarm and Temperature傳送失敗。 [{0}]";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature command send fail. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "EM Battery Alarm and Temperature傳送失敗。 [{0}]", item.B03ListViewTagWindowData), true);
            }

            IsB03EMVoltTempWork = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        private void DoB03ListViewItemBattVoltClickWork(object obj)
        {
            var item = obj as B03ListViewItem;
            Int32 _index = 0;
            Boolean _f = false;
            String _battAlarmTemp = String.Empty;

            item.B03ListViewTagWindowStatus = String.Empty;
            item.B03ListViewBattVolt = String.Empty;


            if (DoB03SendReceiveWork(this.ReaderService.CommandT("2", "30", "30", item.B03ListViewTagWindowData), CommandStatus.B03SELECT))
            {
                _f = true;
                while (IsReceiveDataWork)   //400ms timeout
                {
                    Thread.Sleep(4);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }
            }

            if (!_f)
            {
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "Select command timeout or parameter error." :
                    "Select指令超時或參數錯誤。";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "Select command timeout or parameter error. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "Select指令超時或參數錯誤。 [{0}]", item.B03ListViewTagWindowData), true);
                IsB03EMVoltTempWork = false;
                return;
            }



            _f = false; _index = 0;
            if (DoB03SendReceiveWork(this.ReaderService.EMGetBattVolt(), CommandStatus.CUSTOMEM))
            {
                _f = true;
                while (IsReceiveDataWork)   //1000ms timeout
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 100)
                    {
                        _f = false;
                        break;
                    }
                }

                if (!_f)
                {
                    item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                        "EM Battery Voltage command timeout." :
                        "EM Battery Voltage指令超時。";
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage command timeout. [{0}]", item.B03ListViewTagWindowData) :
                        String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage指令超時。 [{0}]", item.B03ListViewTagWindowData), true);
                    IsB03EMVoltTempWork = false;
                    return;
                }
                else
                {
                    if (EMTemp.Length == 4)
                    {
                        var _ba = Format.HexStringToInt(EMTemp);

                        if (_ba == 0)
                            _battAlarmTemp = "0";
                        else
                            _battAlarmTemp = (0x00100000 / _ba).ToString("0", CultureInfo.CurrentCulture);
                        item.B03ListViewBattVolt = _battAlarmTemp;
                    }
                    else
                    {
                        item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                            "EM Battery Voltage command receive fail." :
                            "EM Battery Voltage接收失敗。";
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage command receive fail. [{0}]", item.B03ListViewTagWindowData) :
                            String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage接收失敗。 [{0}]", item.B03ListViewTagWindowData), true);
                        IsB03EMVoltTempWork = false;
                        return;
                    }
                }
            }
            else
            {
                item.B03ListViewBattVolt = "N/A";
                item.B03ListViewTagWindowStatus = (this.Culture.IetfLanguageTag == "en-US") ?
                    "EM Battery Voltage command send fail." :
                    "EM Battery Voltage傳送失敗。 [{0}]";
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    String.Format(CultureInfo.CurrentCulture, "EMBattery Voltage command send fail. [{0}]", item.B03ListViewTagWindowData) :
                    String.Format(CultureInfo.CurrentCulture, "EM Battery Voltage傳送失敗。 [{0}]", item.B03ListViewTagWindowData), true);
            }

            IsB03EMVoltTempWork = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowButtonGetClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);
            VM.B03ListViewItemsSource.Clear();


            this.B03Thread = new Thread(DoB03TagGetWork) {
                IsBackground = true
            };
            this.B03Thread.Start();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03ListViewItemLEDFlashClick(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            var item = button.DataContext as B03ListViewItem;

            MessageShow(String.Empty, false);


            if (String.IsNullOrEmpty(VM.B03GroupTagWindowTime))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Time field is null" : "時間欄位不得為空", true);
                this.B03GroupTagWindowTime.Focus();
                return;
            }
            if (Convert.ToInt32(VM.B03GroupTagWindowTime, 16) > 0xFF || Convert.ToInt32(VM.B03GroupTagWindowTime, 16) == 0)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Time field value over range: 1 ~ 0xFF" : "時間欄位值超出規範值: 1 ~ 0xFF", true);
                this.B03GroupTagWindowTime.Focus();
                return;
            }


            if (IsB03EMWork)
                return;

            IsB03EMWork = true;

            var _thread = new Thread(new ParameterizedThreadStart(DoB03ListViewItemLEDFlashClickWork)) {
                IsBackground = true
            };
            _thread.Start(item);

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03ListViewItemBattAlarmTempClick(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            var item = button.DataContext as B03ListViewItem;

            MessageShow(String.Empty, false);

            if (IsB03EMVoltTempWork)
                return;

            IsB03EMVoltTempWork = true;
            var _thread = new Thread(new ParameterizedThreadStart(DoB03ListViewItemBattAlarmTempClickWork)) {
                IsBackground = true
            };
            _thread.Start(item);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03ListViewItemBattVoltClick(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            var item = button.DataContext as B03ListViewItem;

            MessageShow(String.Empty, false);

            if (IsB03EMVoltTempWork)
                return;

            IsB03EMVoltTempWork = true;
            var _thread = new Thread(new ParameterizedThreadStart(DoB03ListViewItemBattVoltClickWork)) {
                IsBackground = true
            };
            _thread.Start(item);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowButtonLEDFlashClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if (VM.B03ListViewItemsSource.Count == 0)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "No Tag item" : "沒有任何標籤項目", true);
                return;
            }
                

            Boolean _b = false;
            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    _b = true;
                    break;
                }
            }

            if (!_b) {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                    "Add one item to test." : "選擇一個測試標籤", true);
                return;
            }

            if (String.IsNullOrEmpty(VM.B03GroupTagWindowTime))
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Time field is null" : "時間欄位不得為空", true);
                this.B03GroupTagWindowTime.Focus();
                return;
            }

            if (Convert.ToInt32(VM.B03GroupTagWindowTime, 16) > 0xFF || Convert.ToInt32(VM.B03GroupTagWindowTime, 16) == 0)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Time field value over range: 1 ~ 0xFF" : "時間欄位值超出規範值: 1 ~ 0xFF", true);
                this.B03GroupTagWindowTime.Focus();
                return;
            }

            if (IsB03EMWork)
                return;

            B03PollingButton(false);
            IsB03EMWork = false;

            this.B03Thread = new Thread(DoB03TagLEDFlashWork) {
                IsBackground = true
            };
            this.B03Thread.Start();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowButtonBattAlarmTempClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if (VM.B03ListViewItemsSource.Count == 0)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "No Tag item" : "沒有任何標籤項目", true);
                return;
            }

            Boolean _b = false;
            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                //var item = this.B03ListViewTagWindow.Items.GetItemAt(j) as B03ListViewItem;
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    //this.B03ListViewTagWindow.Items.Refresh();
                    _b = true;
                    break;
                }
            }

            if (!_b)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Choice one item to run." : "未選擇測試標籤", true);
                return;
            }

            if (IsB03EMVoltTempWork)
                return;

            B03PollingButton(false);
            IsB03EMVoltTempWork = true;
            this.B03Thread = new Thread(DoB03TagBattAlarmTempWork) {
                IsBackground = true
            };
            this.B03Thread.Start();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowButtonBattVoltClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, false);

            if (VM.B03ListViewItemsSource.Count == 0)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "No Tag item" : "沒有任何標籤項目", true);
                return;
            }

            Boolean _b = false;
            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                //var item = this.B03ListViewTagWindow.Items.GetItemAt(j) as B03ListViewItem;
                if (VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck)
                {
                    VM.B03ListViewItemsSource[j].B03ListViewTagWindowStatus = String.Empty;
                    //this.B03ListViewTagWindow.Items.Refresh();
                    _b = true;
                    break;
                }
            }

            if (!_b)
            {
                MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                                "Choice one item to run." : "未選擇測試標籤", true);
                return;
            }

            if (IsB03EMVoltTempWork)
                return;

            B03PollingButton(false);
            IsB03EMVoltTempWork = true;
            this.B03Thread = new Thread(DoB03TagBattVoltWork) {
                IsBackground = true
            };
            this.B03Thread.Start();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowChecked(object sender, RoutedEventArgs e)
        {
            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck = true;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupTagWindowUnchecked(object sender, RoutedEventArgs e)
        {
            for (Int32 j = 0; j < VM.B03ListViewItemsSource.Count; j++)
            {
                VM.B03ListViewItemsSource[j].B03ListViewTagWindowSelCheck = false;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        /*private void B03ListViewTagWindowItemPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            
            var lvi = sender as ListViewItem;
            if (lvi != null && lvi.IsSelected)
            {
                var idx = B03ListViewTagWindow.SelectedIndex; 

                if (VM.B03GroupTagWindowTime == String.Empty)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? "Time field is null" : "時間欄位不得為空", true);
                    this.B03GroupTagWindowTime.Focus();
                    return;
                }
                if (Convert.ToInt32(VM.B03GroupTagWindowTime, 16) > 0xFF || Convert.ToInt32(VM.B03GroupTagWindowTime, 16) == 0)
                {
                    MessageShow(c "Time field value over range: 1 ~ 0xFF" : "時間欄位值超出規範值: 1 ~ 0xFF", true);
                    this.B03GroupTagWindowTime.Focus();
                    return;
                }

                var _thread = new Thread(new ParameterizedThreadStart(DoB03ListViewItemLEDFlashClickWork));
                _thread.IsBackground = true;
                _thread.Start(lvi.Content);
            }
        }*/

        /// <summary>
        /// 
        /// </summary>
        /// <param name="str">direct type: "TX""RX"</param>
        /// <param name="data">data</param>
		private void B03DisplayInfoMsg(String str, String data, Boolean b)
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                ListBoxItem itm = new ListBoxItem();
                if (str == "TX")
                {
                    this.IsDateTimeStamp = true;
                    itm.Foreground = Brushes.SeaGreen;
                }
                else
                    itm.Foreground = Brushes.DarkRed;
                if (this.IsDateTimeStamp)
                {
                    if (str == "RX") this.IsDateTimeStamp = false;
                    if (data == null)
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - ", DateTime.Now.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), str);
                    else
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0} [{1}] - {2}", DateTime.Now.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), str, Format.ShowCRLF(data));
                }
                else
                {
                    if (data == null)
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "[{0}] - ", str);
                    else if (IsReceiveDataWork)
                    {
                        if (b)
                            itm.Content = String.Format(CultureInfo.CurrentCulture, "{0}      - {1}", DateTime.Now.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture), Format.ShowCRLF(data));
                        else
                            itm.Content = Format.ShowCRLF(data);
                    }
                    else
                        itm.Content = String.Format(CultureInfo.CurrentCulture, "{0}  -- {1}", Format.ShowCRLF(data), DateTime.Now.ToString("H:mm:ss.fff", CultureInfo.CurrentCulture));
                }
                if (this.B03ListBox.Items.Count > 1000)
                    this.B03ListBox.Items.Clear();

                this.B03ListBox.Items.Add(itm);

                itm = null;
            }));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03ListBoxMenuItemDeleteAllClick(object sender, RoutedEventArgs e)
        {
            this.B03ListBox.Items.Clear();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void B03ListBoxCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Add)
            {
                B03ListBox.ScrollIntoView(e.NewItems[0]);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupLEDFlashMouseMove(object sender, MouseEventArgs e)
        {
            B03GroupB03GroupLEDFlashPop.IsOpen = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupLEDFlashMouseLeave(object sender, MouseEventArgs e)
        {
            B03GroupB03GroupLEDFlashPop.IsOpen = false;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupLowBattAlarmandTempMouseMove(object sender, MouseEventArgs e)
        {
            B03GroupLowBattAlarmandTempPop.IsOpen = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB03GroupLowBattAlarmandTempMouseLeave(object sender, MouseEventArgs e)
        {
            B03GroupLowBattAlarmandTempPop.IsOpen = false;
        }
        #endregion


        #region === #Pager04 ===
        private List<String> B04ListViewList = new List<String>();
        /// <summary>
        /// 
        /// </summary>
        private void InitializeB04()
        {
            VM.B04GroupPreSetReadMemBank = VM.MemBank[1];
            VM.B04GroupPreSetReadMemBankIndex = 1;
            ValidationState["B04GroupPreSetReadAddress"] = ValidationStates.OK;
            ValidationState["B04GroupPreSetReadLength"] = ValidationStates.OK;
            VM.B04Antenna1RunTimes = "10";
            VM.B04Antenna2RunTimes = "10";
            VM.B04Antenna3RunTimes = "10";
            VM.B04Antenna4RunTimes = "10";
            VM.B04Antenna5RunTimes = "10";
            VM.B04Antenna6RunTimes = "10";
            VM.B04Antenna7RunTimes = "10";
            VM.B04Antenna8RunTimes = "10";
            VM.B04AntennaLoopTimes = "10";
            VM.B04AntennaLoopDelayTime = "10";
            ValidationState["B04Antenna1RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna2RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna3RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna4RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna5RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna6RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna7RunTimes"] = ValidationStates.OK;
            ValidationState["B04Antenna8RunTimes"] = ValidationStates.OK;
            ValidationState["B04AntennaLoopTimes"] = ValidationStates.OK;
            ValidationState["B04AntennaLoopDelayTime"] = ValidationStates.OK;
            this.B04AntennaTestStart.Tick += new EventHandler(DoB04AntennaTestStartWork);
            this.B04Process.Tick += new EventHandler(DoB04ProcessWork);
            this.B04Process.Interval = TimeSpan.FromMilliseconds(1);
            this.B04ProcessDelay.Tick += new EventHandler(DoB04ProcessDelayWork);
            this.B04ProcessDelay.Interval = TimeSpan.FromMilliseconds(100);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
        private void B04DisplayStatisticsMsg(String str)
        {
            Boolean bCompare = false;
            Int32 number = 0;
            String[] data = str.Split(',');


            if (str.Length >= 8 && Format.CRC16(Format.HexStringToBytes(data[0])) == 0x1D0F)
            {
                var newItems = new B04ListViewItem()
                {
                    TagValue = str
                };

                VM.B04TagReadCount = (Convert.ToInt32(String.IsNullOrEmpty(VM.B04TagReadCount) ? "0" : VM.B04TagReadCount, CultureInfo.CurrentCulture) + 1).ToString(CultureInfo.CurrentCulture);
                if (VM.B04ListViewItemsSource.Count > 0)
                {
                    for (int j = 0; j < VM.B04ListViewItemsSource.Count; j++)
                    {
                        if (VM.B04ListViewItemsSource[j].TagValue == newItems.TagValue)
                        {
                            switch (this.DoProcess)
                            {
                                case CommandStatus.B04ANTENNA01:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A1Count) ?
                                        1 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A1Count, CultureInfo.CurrentCulture) + 1;
                                    VM.B04ListViewItemsSource[j].A1Count = number.ToString(CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A1RR = VM.B04ListViewItemsSource[j].A1Count;
                                    break;
                                case CommandStatus.B04ANTENNA02:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A2Count) ?
                                        1 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A2Count, CultureInfo.CurrentCulture) + 1;
                                    VM.B04ListViewItemsSource[j].A2Count = number.ToString(CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A2RR = VM.B04ListViewItemsSource[j].A2Count;
                                    break;
                                case CommandStatus.B04ANTENNA03:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A3Count) ?
                                        1 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A3Count, CultureInfo.CurrentCulture) + 1;
                                    VM.B04ListViewItemsSource[j].A3Count = number.ToString(CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A3RR = VM.B04ListViewItemsSource[j].A3Count;
                                    break;
                                case CommandStatus.B04ANTENNA04:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A4Count) ?
                                        1 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A4Count, CultureInfo.CurrentCulture) + 1;
                                    VM.B04ListViewItemsSource[j].A4Count = number.ToString(CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A4RR = VM.B04ListViewItemsSource[j].A4Count;
                                    break;
                            }
                            bCompare = true;
                        }
                        else
                        {
                            switch (this.DoProcess)
                            {
                                case CommandStatus.B04ANTENNA01:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A1Count) ?
                                        0 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A1Count, CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A1RR = number.ToString(CultureInfo.CurrentCulture);
                                    break;
                                case CommandStatus.B04ANTENNA02:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A2Count) ?
                                        0 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A2Count, CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A2RR = number.ToString(CultureInfo.CurrentCulture);
                                    break;
                                case CommandStatus.B04ANTENNA03:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A3Count)?
                                        0 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A3Count, CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A3RR = number.ToString(CultureInfo.CurrentCulture);
                                    break;
                                case CommandStatus.B04ANTENNA04:
                                    number = String.IsNullOrEmpty(VM.B04ListViewItemsSource[j].A4Count) ?
                                        0 : Convert.ToInt32(VM.B04ListViewItemsSource[j].A4Count, CultureInfo.CurrentCulture);
                                    VM.B04ListViewItemsSource[j].A4RR = number.ToString(CultureInfo.CurrentCulture);
                                    break;
                            }
                            bCompare = false;
                        }
                        if (bCompare) break;
                    }
                }
                else
                {
                    bCompare = false;
                }

                if (!bCompare)
                {
                    
                    switch (this.DoOldProcess)
                    {
                        case CommandStatus.B04ANTENNA01:
                            newItems.A1Count = "0"; newItems.A1RR = "0"; break;
                        case CommandStatus.B04ANTENNA02:
                            newItems.A2Count = "0"; newItems.A2RR = "0"; break;
                        case CommandStatus.B04ANTENNA03:
                            newItems.A3Count = "0"; newItems.A3RR = "0"; break;
                        case CommandStatus.B04ANTENNA04:
                            newItems.A4Count = "0"; newItems.A4RR = "0"; break;
                        default: break;
                    }
                    switch (this.DoProcess)
                    {
                        case CommandStatus.B04ANTENNA01:
                            newItems.A1Count = "1"; newItems.A1RR = "1"; break;
                        case CommandStatus.B04ANTENNA02:
                            newItems.A2Count = "1"; newItems.A2RR = "1"; break;
                        case CommandStatus.B04ANTENNA03:
                            newItems.A3Count = "1"; newItems.A3RR = "1"; break;
                        case CommandStatus.B04ANTENNA04:
                            newItems.A4Count = "1"; newItems.A4RR = "1"; break;
                    }
                    VM.B04ListViewAddNewItem(newItems);
                    B04ListViewList.Add(data[0]);
                }

                VM.B04TagCount = B04ListViewList.Distinct().Count().ToString(CultureInfo.CurrentCulture);
                //(VM.B04ListViewItemsSource.Count).ToString(CultureInfo.CurrentCulture);
            }
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="command"></param>
        /// <param name="process"></param>
        private Boolean DoB04AntennaChangeWork(CommandStatus idx)
        {
            Int32 _index = 0;
            Boolean _f = false;
            String _ant = String.Empty;
            Byte[] _data;

            if (!IsReceiveDataWork)
            {
                IsReceiveDataWork = true;
                this.DoProcess = CommandStatus.B04GPIOPINS;

                try
                {
                    switch (idx)
                    {
                        case CommandStatus.B04ANTENNA01:
                            B04RAWLOG("天線A1");
                            B04AntennaTempData = (Byte)0x0; _ant = "70"; break;
                        case CommandStatus.B04ANTENNA02:
                            B04RAWLOG("天線A2");
                            B04AntennaTempData = (Byte)0x1; _ant = "71"; break;
                        case CommandStatus.B04ANTENNA03:
                            B04RAWLOG("天線A3");
                            B04AntennaTempData = (Byte)0x3; _ant = "73"; break;
                        case CommandStatus.B04ANTENNA04:
                            B04RAWLOG("天線A4");
                            B04AntennaTempData = (Byte)0x2; _ant = "72"; break;
                    }

                    _data = this.ReaderService.SetGPIOPins(_ant);
                    B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(_data)));
                    switch (_ConnectType)
                    {
                        default:
                        case ReaderService.ConnectType.DEFAULT:
                            break;
                        case ReaderService.ConnectType.COM:
                            this._ICOM.Send(_data, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.USB:
                            this._IUSB.Send(_data, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.NET:
                            this._INet.Send(_data, ReaderModule.CommandType.Normal);
                            break;
                        case ReaderService.ConnectType.BLE:
                            this._IBLE.Send(_data, ReaderModule.CommandType.Normal);
                            break;
                    }
                }
                catch (InvalidOperationException ex)
                {
                    MessageShow(ex.Message, true);
                    return false;
                }
                catch (ArgumentNullException anx)
                {
                    MessageShow(anx.Message, true);
                    return false;
                }
                catch (SocketException se)
                {
                    MessageShow(se.Message, true);
                    return false;
                }


                _f = true;
                while (IsReceiveDataWork)
                {
                    Thread.Sleep(10);
                    _index++;
                    if (_index >= 200)
                    {
                        _f = false;
                        break;
                    }
                }
                if (IsB04AntennaSetPinWork && _f)
                    return true;
                else
                    return false;
            }
            return false;
        }

        
        /// <summary>
        /// 
        /// </summary>
        /// <param name="s"></param>
        private void B04RAWLOG(String s)
        {
            if (VM.B04AntennaRawLogCheckBoxIsChecked)
                RawLogger.Info(s);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="s"></param>
        private void FRAGMENTSUMMARYLOG(String s)
        {
            if (VM.B04AntennaFragmentSummaryLogCheckBoxIsChecked)
                FragmentSummaryLogger.Info(s);
        }

        /// <summary>
        ///  check the select antenna
        /// </summary>
        /// <returns></returns>
        private Byte TestItemCheck()
        {
            Byte b = 0;
            for (Byte i = 0; i < 8; i++)
            {
                if (((B04AntennaItemsTemp >> i) & 0x01) == 1)
                {
                    b = (Byte)(0x01 << i);
                    B04AntennaItemsTemp &= (Byte)(0xFF - b);
                    break;
                }
            }
            return b;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DoB04AntennaTestStartWork(object sender, EventArgs e)
        {
            String _selAntenna = String.Empty;
            String _selRunTimes = String.Empty;
            B04ProcessItem.Clear();
            B04AntennaTargetRunTimes.Clear();
            for (;;)
            {
                switch (TestItemCheck())
                {
                    case 0x0:
                        this.B04AntennaTestStart.Stop();
                        if (B04ProcessItem.Count > 0)
                        {
                            if (this.IsB04BtnAntennaRun)
                            {
                                this.IsB04BtnAntennaRun = false;
                                B04RAWLOG("================= Test Infomation =============");
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "天線形式: {0}", 4));
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "選擇天線: {0}", _selAntenna));
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "執行次數: {0}", _selRunTimes));
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "循環次數: {0}", VM.B04AntennaLoopTimes));
                                B04RAWLOG("");
                                B04RAWLOG("=================");
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "循環次數: {0}", B04AntennaLoopIndex + 1));
                                B04RAWLOG("=================");

                            }
                            else {
                                B04RAWLOG("");
                                B04RAWLOG("=================");
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "循環次數: {0}", B04AntennaLoopIndex + 1));
                                B04RAWLOG("=================");


                                FRAGMENTSUMMARYLOG("===============================================");
                                FRAGMENTSUMMARYLOG("欄位定義: Tag, Antenna1,..,Antenna4");
                                String _str;
                                for (Int32 i = 0; i < VM.B04ListViewItemsSource.Count; i++)
                                {
                                    _str = VM.B04ListViewItemsSource[i].TagValue + ",\t" +
                                          VM.B04ListViewItemsSource[i].A1RR + ",\t" +
                                          VM.B04ListViewItemsSource[i].A2RR + ",\t" +
                                          VM.B04ListViewItemsSource[i].A3RR + ",\t" +
                                          VM.B04ListViewItemsSource[i].A4RR;
                                    FRAGMENTSUMMARYLOG(_str);
                                    _str = String.Empty;
                                }

                                if (string.IsNullOrEmpty(VM.B04TagReadCount))
                                    B04AntennaTagIncreaseCount = 0;
                                else
                                    B04AntennaTagIncreaseCount = Int32.Parse(VM.B04TagReadCount, CultureInfo.CurrentCulture) - B04AntennaTagIncreaseCount;
                                FRAGMENTSUMMARYLOG(String.Format(CultureInfo.CurrentCulture, "標籤張數: {0},標籤讀取次數: {1}(+{2})", VM.B04TagCount, VM.B04TagReadCount, B04AntennaTagIncreaseCount));
                                if (string.IsNullOrEmpty(VM.B04TagReadCount))
                                    B04AntennaTagIncreaseCount = 0;
                                else
                                    B04AntennaTagIncreaseCount = Int32.Parse(VM.B04TagReadCount, CultureInfo.CurrentCulture);
                            }
                            
                            B04Process.Start();
                        }
                        return;
                    case 0x01:
                        B04ProcessItem.Add(CommandStatus.B04ANTENNA01);
                        B04AntennaTargetRunTimes.Add(Int32.Parse(VM.B04Antenna1RunTimes, CultureInfo.CurrentCulture));
                        this.B04AntennaRunIndex++;
                        _selAntenna += "A1";
                        _selRunTimes += VM.B04Antenna1RunTimes;
                        break;
                    case 0x02:
                        B04ProcessItem.Add(CommandStatus.B04ANTENNA02);
                        B04AntennaTargetRunTimes.Add(Int32.Parse(VM.B04Antenna2RunTimes, CultureInfo.CurrentCulture));
                        this.B04AntennaRunIndex++;
                        if (B04AntennaRunIndex > 1)
                        {
                            _selAntenna += ",A2";
                            _selRunTimes += "," + VM.B04Antenna2RunTimes;
                        }  
                        else
                        {
                            _selAntenna += "A2";
                            _selRunTimes += VM.B04Antenna2RunTimes;
                        } 
                        break;
                    case 0x04:
                        B04ProcessItem.Add(CommandStatus.B04ANTENNA03);
                        B04AntennaTargetRunTimes.Add(Int32.Parse(VM.B04Antenna3RunTimes, CultureInfo.CurrentCulture));
                        this.B04AntennaRunIndex++;
                        if (B04AntennaRunIndex > 1)
                        {
                            _selAntenna += ",A3";
                            _selRunTimes += "," + VM.B04Antenna3RunTimes;
                        }   
                        else
                        {
                            _selAntenna += "A3";
                            _selRunTimes += VM.B04Antenna3RunTimes;
                        }   
                        break;
                    case 0x08:
                        B04ProcessItem.Add(CommandStatus.B04ANTENNA04);
                        B04AntennaTargetRunTimes.Add(Int32.Parse(VM.B04Antenna4RunTimes, CultureInfo.CurrentCulture));
                        this.B04AntennaRunIndex++;
                        if (B04AntennaRunIndex > 1)
                        {
                            _selAntenna += ",A4";
                            _selRunTimes += "," + VM.B04Antenna4RunTimes;
                        }
                        else
                        {
                            _selAntenna += "A4";
                            _selRunTimes += VM.B04Antenna4RunTimes;
                        }   
                        break;
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void B04AntennaTestEndWork()
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {



                Thread.Sleep(500);
                B04RAWLOG("");
                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "標籤個數:{0}, 標籤讀取次數:{1}", VM.B04TagCount, VM.B04TagReadCount));
                B04RAWLOG("================= END =========================");

                FRAGMENTSUMMARYLOG("===============================================");
                FRAGMENTSUMMARYLOG("欄位定義: Tag, Antenna1,..,Antenna5(Antenna8)");
                String _str;
                for (Int32 i = 0; i < VM.B04ListViewItemsSource.Count; i++)
                {
                    _str = VM.B04ListViewItemsSource[i].TagValue + ",\t" +
                          VM.B04ListViewItemsSource[i].A1RR + ",\t" +
                          VM.B04ListViewItemsSource[i].A2RR + ",\t" +
                          VM.B04ListViewItemsSource[i].A3RR + ",\t" +
                          VM.B04ListViewItemsSource[i].A4RR;
                    FRAGMENTSUMMARYLOG(_str);
                    _str = String.Empty;
                }

                try
                {
                    if (string.IsNullOrEmpty(VM.B04TagReadCount))
                        B04AntennaTagIncreaseCount = 0;
                    else
                        B04AntennaTagIncreaseCount = Int32.Parse(VM.B04TagReadCount, CultureInfo.CurrentCulture) - B04AntennaTagIncreaseCount;
                }
                catch (FormatException)
                {
                    B04AntennaTagIncreaseCount = 0;
                }
                
                FRAGMENTSUMMARYLOG(String.Format(CultureInfo.CurrentCulture, "標籤張數: {0},標籤讀取次數: {1}(+{2})", VM.B04TagCount, VM.B04TagReadCount, B04AntennaTagIncreaseCount));
                
               

                this.IsOnB04BtnAntennaClick = false;
                this.B04AntennaRunButton.Content = Properties.Resources.B04AntennaRun;
                this.B04AntennaRunRepeatCheckBox.IsEnabled = true;
                this.B04AntennaRawLogCheckBox.IsEnabled = true;
                this.B04AntennaSingleSummaryLogCheckBox.IsEnabled = true;
                this.B04AntennaRunLogButton.IsEnabled = true;

                UIControlStatus(UITempControPackets, false);
                UIControlStatus(UIControPackets, true);

                
            }));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DoB04ProcessWork(object sender, EventArgs e)
        {
            if (!IsOnB04BtnAntennaClick)
            {
                this.IsB04ChangeAndRun = false;
                this.B04Process.Stop();
                B04AntennaTestEndWork();
                return;
            }

            //Antenna change
            if (!this.IsB04ChangeAndRun)
            {
                if (!DoB04AntennaChangeWork(B04ProcessItem[0]))
                {
                    MessageShow("Antenna change error", false);
                    this.B04Process.Stop();
                    B04AntennaTestEndWork();
                    return;
                }
                this.B04AntennaRunCount = 0;
                this.IsB04ChangeAndRun = true;
                this.IsB04RepeatRunEnd = false;
            }
            //Antenna run
            else {
                if (!this.IsReceiveDataWork) {     
                    if (B04AntennaRunCount < B04AntennaTargetRunTimes[0]) {
                        if (!IsOnB04BtnAntennaClick) {
                            IsB04RepeatRunEnd = true;
                        }
                        else
                        {
                            this.IsReceiveDataWork = true;
                            B04AntennaRunCount++;
                            this.DoProcess = B04ProcessItem[0];

                            switch (B04ProcessItem[0])
                            {
                                case CommandStatus.B04ANTENNA01:
                                    MessageShow(String.Format(CultureInfo.CurrentCulture, "Loop{0}, Antenna01...{1}", B04AntennaLoopIndex + 1, B04AntennaRunCount), false);
                                    break;
                                case CommandStatus.B04ANTENNA02:
                                    MessageShow(String.Format(CultureInfo.CurrentCulture, "Loop{0}, Antenna02...{1}", B04AntennaLoopIndex + 1, B04AntennaRunCount), false);
                                    break;
                                case CommandStatus.B04ANTENNA03:
                                    MessageShow(String.Format(CultureInfo.CurrentCulture, "Loop{0}, Antenna03...{1}", B04AntennaLoopIndex + 1, B04AntennaRunCount), false);
                                    break;
                                case CommandStatus.B04ANTENNA04:
                                    MessageShow(String.Format(CultureInfo.CurrentCulture, "Loop{0}, Antenna04...{1}", B04AntennaLoopIndex + 1, B04AntennaRunCount), false);
                                    break;
                            }

                            try
                            {
                                byte[] _data;
                                if (VM.B04GroupReadCtrlCheckBoxIsChecked)
                                {
                                    if (VM.B04GroupUSlotQCheckBoxIsChecked)
                                        _data = this.ReaderService.CommandUR(VM.B04GroupUSlotQComboBox.Tag, VM.B04GroupPreSetReadMemBank.Tag,
                                            VM.B04GroupPreSetReadAddress, VM.B04GroupPreSetReadLength);
                                    else
                                        _data = this.ReaderService.CommandUR(null, VM.B04GroupPreSetReadMemBank.Tag, VM.B04GroupPreSetReadAddress, VM.B04GroupPreSetReadLength);
                                }
                                else
                                {
                                    if (VM.B04GroupUSlotQCheckBoxIsChecked)
                                        _data = this.ReaderService.CommandU(VM.B04GroupUSlotQComboBox.Tag);
                                    else
                                        _data = this.ReaderService.CommandU();
                                }

                                B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(_data)));
                                switch (_ConnectType)
                                {
                                    default:
                                    case ReaderService.ConnectType.DEFAULT:
                                        break;
                                    case ReaderService.ConnectType.COM:
                                        this._ICOM.Send(_data, ReaderModule.CommandType.Normal);
                                        break;
                                    case ReaderService.ConnectType.USB:
                                        this._IUSB.Send(_data, ReaderModule.CommandType.Normal);
                                        break;
                                    case ReaderService.ConnectType.NET:
                                        this._INet.Send(_data, ReaderModule.CommandType.Normal);
                                        break;
                                    case ReaderService.ConnectType.BLE:
                                        this._IBLE.Send(_data, ReaderModule.CommandType.Normal);
                                        break;
                                }

                            }
                            catch (InvalidOperationException ex)
                            {
                                MessageShow(ex.Message, true);
                                this.IsReceiveDataWork = false;
                                IsB04RepeatRunEnd = true;
                            }
                            catch (ArgumentNullException ane)
                            {
                                MessageShow(ane.Message, true);
                                this.IsReceiveDataWork = false;
                                IsB04RepeatRunEnd = true;
                            }
                            catch (SocketException se)
                            {
                                MessageShow(se.Message, true);
                                this.IsReceiveDataWork = false;
                                IsB04RepeatRunEnd = true;
                            }
                        }
                    }
                    else
                    {
                        IsB04RepeatRunEnd = true;
                    }
                }

                if (this.IsB04RepeatRunEnd)
                {
                    this.IsB04ChangeAndRun = false;

                    this.DoOldProcess = this.B04ProcessItem[0]; //v3.2.7 add
                    this.B04ProcessItem.RemoveAt(0);
                    this.B04AntennaTargetRunTimes.RemoveAt(0);

                    if (--this.B04AntennaRunIndex == 0)
                    {
                        MessageShow(String.Empty, false);
                        
                        if (++this.B04AntennaLoopIndex == Int32.Parse(VM.B04AntennaLoopTimes, CultureInfo.CurrentCulture))
                        {
                            if (VM.B04AntennaRunRepeatCheckBoxIsChecked)
                            {
                                this.B04AntennaLoopIndex = 0;
                                this.B04Process.Stop();

                                B04RAWLOG("=========== DELAY ===================");
                                B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "Delay:{0}", VM.B04AntennaLoopDelayTime));
                                B04RAWLOG("=====================================");
                                    
                                this.B04AntennaDelayTimesIdx = 0;
                                this.B04ProcessDelay.Start();
                            }
                            else
                                this.IsOnB04BtnAntennaClick = false;
                                //B04AntennaTestEndWork();
                        }
                        else
                        {
                            this.B04Process.Stop();
                            this.B04AntennaItemsTemp = this.B04AntennaItems;
                            this.B04AntennaTestStart.Start();
                        } 
                    }
                }

            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DoB04ProcessDelayWork(object sender, EventArgs e)
        {
            this.B04AntennaDelayTimesIdx++;
            MessageShow(String.Format(CultureInfo.CurrentCulture, "Delay: {0:0.0}s", ((float)B04AntennaDelayTimesIdx/10)), false);

            if (this.IsOnB04BtnAntennaClick == false)
            {
                this.B04ProcessDelay.Stop();
                B04AntennaTestEndWork();
                return;
            }
            if (this.B04AntennaDelayTimesIdx == this.B04AntennaDelayTimes)
            {
                this.B04ProcessDelay.Stop();
                this.B04AntennaItemsTemp = this.B04AntennaItems;
                this.B04AntennaTestStart.Start();
            }
            
        }

        

        private void OnB04CheckBoxAntenna01Checked(object sender, RoutedEventArgs e) { B04AntennaItems |= 0x01; B04Antenna1RunTimes.IsEnabled = true; }
        private void OnB04CheckBoxAntenna02Checked(object sender, RoutedEventArgs e) { B04AntennaItems |= 0x02; B04Antenna2RunTimes.IsEnabled = true; }
        private void OnB04CheckBoxAntenna03Checked(object sender, RoutedEventArgs e) { B04AntennaItems |= 0x04; B04Antenna3RunTimes.IsEnabled = true; }
        private void OnB04CheckBoxAntenna04Checked(object sender, RoutedEventArgs e) { B04AntennaItems |= 0x08; B04Antenna4RunTimes.IsEnabled = true; }


        private void OnB04CheckBoxAntenna01UnChecked(object sender, RoutedEventArgs e) { B04AntennaItems &= 0xFE; B04Antenna1RunTimes.IsEnabled = false; }
        private void OnB04CheckBoxAntenna02UnChecked(object sender, RoutedEventArgs e) { B04AntennaItems &= 0xFD; B04Antenna2RunTimes.IsEnabled = false; }
        private void OnB04CheckBoxAntenna03UnChecked(object sender, RoutedEventArgs e) { B04AntennaItems &= 0xFB; B04Antenna3RunTimes.IsEnabled = false; }
        private void OnB04CheckBoxAntenna04UnChecked(object sender, RoutedEventArgs e) { B04AntennaItems &= 0xF7; B04Antenna4RunTimes.IsEnabled = false; }
       

        private void B04AntennaGroupRunTimesMouseMove(object sender, MouseEventArgs e)
        {
            B04AntennaGroupRunTimesPop.IsOpen = true;
        }

        private void B04AntennaGroupRunTimesMouseLeave(object sender, MouseEventArgs e)
        {
            B04AntennaGroupRunTimesPop.IsOpen = false;
        }

        private void B04AntennaLoopTimesMouseMove(object sender, MouseEventArgs e)
        {
            B04AntennaLoopTimesPop.IsOpen = true;
        } 

        private void B04AntennaLoopTimesMouseLeave(object sender, MouseEventArgs e)
        {
            B04AntennaLoopTimesPop.IsOpen = false;
        }
        private void B04AntennaLoopDelayTimeMouseMove(object sender, MouseEventArgs e)
        {
            B04AntennaLoopDelayTimePop.IsOpen = true;
        }
        private void B04AntennaLoopDelayTimeMouseLeave(object sender, MouseEventArgs e)
        {
            B04AntennaLoopDelayTimePop.IsOpen = false;
        }

        private void OnB04CheckBoxRawLogChecked(object sender, RoutedEventArgs e)
        {
            if (RawLogger == null)
            {
                RawLogger = LogManager.GetLogger(typeof(MainWindow));
                XmlConfigurator.Configure(new FileInfo("./logConfig/log4netRaw.config"));
                
            }
            //Logger = LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);
        }

        private void OnB04CheckBoxFragmentSummaryLogChecked(object sender, RoutedEventArgs e)
        {
            if (FragmentSummaryLogger == null)
            {
                FragmentSummaryLogger = LogManager.GetLogger(typeof(MainWindow));
                XmlConfigurator.Configure(new FileInfo("./logConfig/log4netFragmentSummary.config"));

            }
        }





        private void OnB04AntennaRunClick(object sender, RoutedEventArgs e)
        {
            MessageShow(String.Empty, true);
            if (!IsOnB04BtnAntennaClick)
            {
                //Check the run items
                if (this.B04AntennaItems == 0)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Please select an antenna item" :
                        "至少選擇一個天線項目", true);
                    return;
                }

                //Check the run times
                if (!B04CheckRunTimes())
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Error parameter in the RunTimes field" : 
						"執行次數欄位參數錯誤", true);
                    return;
                }

                //Check the loop times
                if ((ValidationStates)ValidationState["B04AntennaLoopTimes"] != ValidationStates.OK)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                        "Error parameter in the LoopTimes field" :
                        "循環次數欄位參數錯誤", true);
                    return;
                }

                //Check GPIO configuration to out mode
                Byte[] bs = null, tbs = this.ReaderService.SetGPIOConfiguration("77");
                B04RAWLOG("================= Set GPIO to OUT mode ========");
                switch (_ConnectType)
                {
                    default:
                    case ReaderService.ConnectType.DEFAULT:
                        break;
                    case ReaderService.ConnectType.COM:
                        this._ICOM.Send(tbs, ReaderModule.CommandType.Normal);
                        B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(tbs)));
                        bs = this._ICOM.Receive();
                        B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(Format.BytesToString(bs))));
                        break;
                    case ReaderService.ConnectType.USB:
                        this._IUSB.Send(tbs, ReaderModule.CommandType.Normal);
                        B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(tbs)));
                        bs = _IUSB.Receive();
                        B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(Format.BytesToString(bs))));
                        break;
                    case ReaderService.ConnectType.NET:
                        this._INet.Send(tbs, ReaderModule.CommandType.Normal);
                        B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(tbs)));
                        bs = _INet.Receive();
                        B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(Format.BytesToString(bs))));
                        break;
                    case ReaderService.ConnectType.BLE:
                        this._IBLE.Send(tbs, ReaderModule.CommandType.Normal);
                        B04RAWLOG("[TX]" + Format.ShowCRLF(Format.BytesToString(tbs)));
                        bs = _IBLE.Receive();
                        B04RAWLOG(String.Format(CultureInfo.CurrentCulture, "[RX]{0}", Format.ShowCRLF(Format.BytesToString(bs))));
                        break;
                }
                B04RAWLOG("");
                if (bs == null || bs.Length != 5)
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? 
						"Set GPIO configuration no response" : 
						"設定GPIO configuration沒回覆", true);
                    return;
                }

                var val = Int32.Parse(Format.ByteToString(bs[2]), CultureInfo.CurrentCulture);
                if (!val.Equals((Byte)0x7))
                {
                    MessageShow((this.Culture.IetfLanguageTag == "en-US") ? 
						"Set GPIO configuration error": 
						"設定GPIO configuration錯誤", true);
                    return;
                }

                //Check the read ctrl
                if (VM.B04GroupReadCtrlCheckBoxIsChecked)
                {
                    if ((ValidationStates)ValidationState["B04GroupPreSetReadAddress"] != ValidationStates.OK ||
                            (ValidationStates)ValidationState["B04GroupPreSetReadLength"] != ValidationStates.OK)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Read command parameter error." :
                            "Read指令參數錯誤", true);
                        return;
                    }
                    /*if (VM.B04GroupPreSetReadMemBank.Tag == "X")
                        return;
                    if (VM.B04GroupPreSetReadAddress == String.Empty)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Address is null" :
                            "讀取位址不得為空", true);
                        this.B04GroupPreSetReadAddress.Focus();
                        return;
                    }
                    if (Convert.ToInt32(VM.B04GroupPreSetReadAddress, 16) > 0x3FFF)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Address value over range: 0 ~ 0x3FFF" :
                            "讀取位址值超出規範值: 0 ~ 0x3FFF", true);
                        this.B04GroupPreSetReadAddress.Focus();
                        return;
                    }
                    if (VM.B04GroupPreSetReadLength == String.Empty)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Length is null" :
                            "讀取長度不得為空", true);
                        this.B04GroupPreSetReadLength.Focus();
                        return;
                    }
                    if (Convert.ToInt32(VM.B04GroupPreSetReadLength, 16) > 0x20 || Convert.ToInt32(VM.B04GroupPreSetReadLength, 16) < 1)
                    {
                        MessageShow((this.Culture.IetfLanguageTag == "en-US") ?
                            "Length value over range: 1 ~ 0x20" :
                            "讀取長度值超出規範值:1 ~ 0x20", true);
                        this.B04GroupPreSetReadLength.Focus();
                        return;
                    }*/
                }


                this.B04AntennaItemsTemp = this.B04AntennaItems;
                this.B04AntennaRunIndex = 0;
                this.DoOldProcess = CommandStatus.DEFAULT;
                this.IsOnB04BtnAntennaClick = true;
                this.IsB04BtnAntennaRun = true;
                this.B04AntennaLoopIndex = 0;
                this.B04AntennaTagIncreaseCount = 0;//add in v3.5.6
                this.B04AntennaDelayTimes = (Int32)(Double.Parse(VM.B04AntennaLoopDelayTime, CultureInfo.CurrentCulture) * 10);
                this.B04AntennaRunButton.Content = Properties.Resources.B04AntennaStop;

                VM.B04ListViewItemsSource.Clear();
                B04ListViewList.Clear();
                VM.B04TagCount = String.Empty;
                VM.B04TagReadCount = String.Empty;

                this.B04AntennaRunRepeatCheckBox.IsEnabled = false;
                this.B04AntennaRawLogCheckBox.IsEnabled = false;
                this.B04AntennaSingleSummaryLogCheckBox.IsEnabled = false;
                this.B04AntennaRunLogButton.IsEnabled = false;


                UITempControPackets.Clear();
                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_PAGES, false));
                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_CULTURE, false));
                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_LIGHT, false));
                UITempControPackets.Add(new UIControl(GroupStatus.BORDER_SET, false));
                UITempControPackets.Add(new UIControl(GroupStatus.GB04AntennaCtrl, false));
                UITempControPackets.Add(new UIControl(GroupStatus.GB04ReadCtrl, false));
                UITempControPackets.Add(new UIControl(GroupStatus.GB04SlotQ, false));
                UIControlStatus(UITempControPackets, true);

                this.B04AntennaTestStart.Start();

            }
            else
            {
                this.IsOnB04BtnAntennaClick = false;
                this.B04AntennaRunButton.Content = Properties.Resources.B04AntennaRun;
                this.B04AntennaRunRepeatCheckBox.IsEnabled = true;
                this.B04AntennaRawLogCheckBox.IsEnabled = true;
                this.B04AntennaSingleSummaryLogCheckBox.IsEnabled = true;
                this.B04AntennaRunLogButton.IsEnabled = true;

                UIControlStatus(UITempControPackets, false);
                UIControlStatus(UIControPackets, true);

                MessageShow("", false);
            }
        }

        private Boolean B04CheckRunTimes()
        {
            Boolean _b = true;
            for (int i = 1; i < 9; i++ )
            {
                if ((ValidationStates)ValidationState[String.Format(CultureInfo.CurrentCulture, "B04Antenna{0}RunTimes", i)] != ValidationStates.OK)
                {
                    _b = false;
                    break;
                }
            }
            return _b;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnB04AntennaRunLogClick(object sender, RoutedEventArgs e)
        {
            var _path = AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "log\\";
            var _file = "MultiAntennaSummary_" + DateTime.Now.ToString("yyyy-MM-dd", CultureInfo.CurrentCulture) + ".log";
            var _filePath = _path + _file;
            StreamWriter swStream;
            String str = String.Empty;

            if (File.Exists(_filePath))
                swStream = new StreamWriter(_filePath, true);
            else
            {
                try
                {
                    swStream = File.CreateText(_filePath);
                }
                catch (DirectoryNotFoundException)
                {
                    Directory.CreateDirectory(_path);
                    swStream = File.CreateText(_filePath);
                }
            }
            //title
            swStream.WriteLine("===============================================");
            swStream.WriteLine("欄位定義: Tag, Antenna1,..,Antenna5(Antenna8)");
            swStream.WriteLine(String.Format(CultureInfo.CurrentCulture, "標籤張數: {0}", VM.B04TagCount));
            swStream.WriteLine(String.Format(CultureInfo.CurrentCulture, "標籤讀取次數: {0}", VM.B04TagReadCount));
            swStream.WriteLine("===============================================");
            for (Int32 i = 0; i < VM.B04ListViewItemsSource.Count; i++)
            {
                str = VM.B04ListViewItemsSource[i].TagValue + ",\t" +
                      VM.B04ListViewItemsSource[i].A1RR + ",\t" +
                      VM.B04ListViewItemsSource[i].A2RR + ",\t" +
                      VM.B04ListViewItemsSource[i].A3RR + ",\t" +
                      VM.B04ListViewItemsSource[i].A4RR;
                swStream.WriteLine(str);
                str = String.Empty;
            }
            swStream.WriteLine("");
            swStream.Flush();
            swStream.Close();
            Process.Start("notepad.exe", _filePath);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnB04GroupPreSetReadMemBankDownClosed(object sender, EventArgs e)
        {
            var _idx = (sender as ComboBox).SelectedIndex;
            switch (_idx)
            {
                case 0: VM.B04GroupPreSetReadAddress = "0"; VM.B04GroupPreSetReadLength = "4"; break;
                case 1: VM.B04GroupPreSetReadAddress = "2"; VM.B04GroupPreSetReadLength = "6"; break;
                case 2: VM.B04GroupPreSetReadAddress = "0"; VM.B04GroupPreSetReadLength = "4"; break;
                case 3: VM.B04GroupPreSetReadAddress = "0"; VM.B04GroupPreSetReadLength = "1"; break;
                case 4: VM.B04GroupPreSetReadAddress = ""; VM.B04GroupPreSetReadLength = ""; break;
                case 5: VM.B04GroupPreSetReadAddress = "0"; VM.B04GroupPreSetReadLength = "2"; break;
                case 6: VM.B04GroupPreSetReadAddress = "2"; VM.B04GroupPreSetReadLength = "2"; break;
            }

            if (_idx == 4)
            {
                ValidationState["B04GroupPreSetReadAddress"] = ValidationStates.DEFAULF;
                ValidationState["B04GroupPreSetReadLength"] = ValidationStates.DEFAULF;                
            }
            else
            {
                ValidationState["B04GroupPreSetReadAddress"] = ValidationStates.OK;
                ValidationState["B04GroupPreSetReadLength"] = ValidationStates.OK;
            }
        }
        #endregion

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sortBy"></param>
        /// <param name="direction"></param>
        /// <param name="lv"></param>
        private void Sort(String sortBy, ListSortDirection direction, IEnumerable lv)
        {
            ICollectionView dataView = CollectionViewSource.GetDefaultView(lv);
            dataView.SortDescriptions.Clear();
            SortDescription sd = new SortDescription(sortBy, direction);
            dataView.SortDescriptions.Add(sd);
            dataView.Refresh();
        }

        #region IDisposable Support
        private bool disposedValue = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    if (_ICOM != null) _ICOM.Dispose();
                    if (_INet != null) _INet.Dispose();
                    if (_IUSB != null) _IUSB.Dispose();
                    if (_IBLE != null) _IBLE.Dispose();
                    if (_SerialPort != null) _SerialPort.Dispose();
                    if (_ConnectDialog != null) _ConnectDialog.Dispose();
                    if (_RegulationDialog != null) _RegulationDialog.Dispose();
                    if (ProfileXml != null) ProfileXml.Dispose();
                    if (B03LightTimeEvent != null) B03LightTimeEvent.Dispose();
                    
                }

                disposedValue = true;
            }
        }


        public void Dispose()
        {
            Dispose(true);

            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
