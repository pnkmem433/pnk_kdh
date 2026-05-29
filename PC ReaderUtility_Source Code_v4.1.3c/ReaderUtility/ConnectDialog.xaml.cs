using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.IO.Ports;
using System.Collections.ObjectModel;
using System.Threading;
using System.Windows.Threading;
using RFID.Service;
using RFID.Service.IInterface.COM;
using RFID.Service.IInterface.NET;
using System.Net.Sockets;
using RFID.Service.IInterface.USB;
using System.Collections.Generic;
using RFID.Utility.VM;
using System.Linq;
using System.Net;
using System.Globalization;
using RFID.Service.IInterface.BLE;
using RFID.Service.IInterface.COM.IClass;
using RFID.Service.IInterface.BLE.IClass;
using System.Threading.Tasks;
using RFID.Service.IInterface.COM.Events;

namespace RFID.Utility
{
	/// <summary>
	/// Interaction logic for ConnectDialog.xaml
	/// </summary>
	public partial class ConnectDialog : Window, IDisposable
	{
		private ReaderService	                _ReaderService;
        private ICOM                            _ICOM;
        private INET                            _INet;
        private UdpClient                       _UDPSocket;
        private IUSB                            _IUSB;
        private IEnumerable<IUSB>               _IUSBs;
        private IBLE                            _IBLE;
        private ReaderService.ConnectType       _ConnectType = ReaderService.ConnectType.DEFAULT;
        private Thread			                SearchThread;
        private Thread                          NetSearchThread;
        private static ManualResetEvent         UPDReceiveDone = new ManualResetEvent(false);
        private ObservableCollection<String>    OCUPDReceives = new ObservableCollection<String>();
        private Int32                           IBaudRate = 38400;

        private readonly DispatcherTimer        BLEEnumerateProcess;


        private ConnectVM VM = new ConnectVM();

        public ConnectDialog()
        {
			InitializeComponent();
            this.DataContext = VM;
            VM.BaudRateSelectedItem = VM.BaudRate[4];

            //COM
            this.SearchThread = new Thread(DoSearchCOMWork) {
                IsBackground = true
            };
            this.SearchThread.Start();

            this._ReaderService = new ReaderService();

            if (this.BLEEnumerateProcess == null)
            {
                this.BLEEnumerateProcess = new DispatcherTimer();
                this.BLEEnumerateProcess.Tick += new EventHandler(DoEnumerateProcessWork);
                this.BLEEnumerateProcess.Interval = TimeSpan.FromMilliseconds(500);
            }
        }
        
        public ConnectDialog(ReaderModule.BaudRate br)
        {
            InitializeComponent();
            this.DataContext = VM;
            //COM
            switch (br)
            {
                case ReaderModule.BaudRate.B4800:   VM.BaudRateSelectedItem = VM.BaudRate[0]; break;
                case ReaderModule.BaudRate.B9600:   VM.BaudRateSelectedItem = VM.BaudRate[1]; break;
                case ReaderModule.BaudRate.B14400:  VM.BaudRateSelectedItem = VM.BaudRate[2]; break;
                case ReaderModule.BaudRate.B19200:  VM.BaudRateSelectedItem = VM.BaudRate[3]; break;
                case ReaderModule.BaudRate.B38400:  VM.BaudRateSelectedItem = VM.BaudRate[4]; break;
                case ReaderModule.BaudRate.B57600:  VM.BaudRateSelectedItem = VM.BaudRate[5]; break;
                case ReaderModule.BaudRate.B115200: VM.BaudRateSelectedItem = VM.BaudRate[6]; break;
                case ReaderModule.BaudRate.B230400: VM.BaudRateSelectedItem = VM.BaudRate[7]; break;
            }
            this.SearchThread = new Thread(DoSearchCOMWork) {
                IsBackground = true
            };
            this.SearchThread.Start();

            this._ReaderService = new ReaderService();

            if (this.BLEEnumerateProcess == null)
            {
                this.BLEEnumerateProcess = new DispatcherTimer();
                this.BLEEnumerateProcess.Tick += new EventHandler(DoEnumerateProcessWork);
                this.BLEEnumerateProcess.Interval = TimeSpan.FromMilliseconds(500);
            }
        }

        public ConnectDialog(ReaderModule.BaudRate br, IUSB usb)
        {
            InitializeComponent();
            this.DataContext = VM;
            TabControl.SelectedIndex = 1;
            //COM
            switch (br)
            {
                case ReaderModule.BaudRate.B4800: VM.BaudRateSelectedItem = VM.BaudRate[0]; break;
                case ReaderModule.BaudRate.B9600: VM.BaudRateSelectedItem = VM.BaudRate[1]; break;
                case ReaderModule.BaudRate.B14400: VM.BaudRateSelectedItem = VM.BaudRate[2]; break;
                case ReaderModule.BaudRate.B19200: VM.BaudRateSelectedItem = VM.BaudRate[3]; break;
                case ReaderModule.BaudRate.B38400: VM.BaudRateSelectedItem = VM.BaudRate[4]; break;
                case ReaderModule.BaudRate.B57600: VM.BaudRateSelectedItem = VM.BaudRate[5]; break;
                case ReaderModule.BaudRate.B115200: VM.BaudRateSelectedItem = VM.BaudRate[6]; break;
                case ReaderModule.BaudRate.B230400: VM.BaudRateSelectedItem = VM.BaudRate[7]; break;
            }

            if (usb != null)
                this._IUSB = usb;

            this._ReaderService = new ReaderService();
        }

        public ConnectDialog(ReaderModule.BaudRate br, INETInfo inet)
        {
            InitializeComponent();
            this.DataContext = VM;
            TabControl.SelectedIndex = 2;
            //COM
            switch (br)
            {
                case ReaderModule.BaudRate.B4800: VM.BaudRateSelectedItem = VM.BaudRate[0]; break;
                case ReaderModule.BaudRate.B9600: VM.BaudRateSelectedItem = VM.BaudRate[1]; break;
                case ReaderModule.BaudRate.B14400: VM.BaudRateSelectedItem = VM.BaudRate[2]; break;
                case ReaderModule.BaudRate.B19200: VM.BaudRateSelectedItem = VM.BaudRate[3]; break;
                case ReaderModule.BaudRate.B38400: VM.BaudRateSelectedItem = VM.BaudRate[4]; break;
                case ReaderModule.BaudRate.B57600: VM.BaudRateSelectedItem = VM.BaudRate[5]; break;
                case ReaderModule.BaudRate.B115200: VM.BaudRateSelectedItem = VM.BaudRate[6]; break;
                case ReaderModule.BaudRate.B230400: VM.BaudRateSelectedItem = VM.BaudRate[7]; break;
            }
            //NET
            if (inet != null) {
                TextBoxNetIP.Text = inet.IP;
                TextBoxNetPort.Text = inet.Port;
            }

            this._ReaderService = new ReaderService();
        }

        public ConnectDialog(ReaderModule.BaudRate br, IBLE ble)
        {
            InitializeComponent();
            this.DataContext = VM;
            TabControl.SelectedIndex = 3;
            //COM
            switch (br)
            {
                case ReaderModule.BaudRate.B4800: VM.BaudRateSelectedItem = VM.BaudRate[0]; break;
                case ReaderModule.BaudRate.B9600: VM.BaudRateSelectedItem = VM.BaudRate[1]; break;
                case ReaderModule.BaudRate.B14400: VM.BaudRateSelectedItem = VM.BaudRate[2]; break;
                case ReaderModule.BaudRate.B19200: VM.BaudRateSelectedItem = VM.BaudRate[3]; break;
                case ReaderModule.BaudRate.B38400: VM.BaudRateSelectedItem = VM.BaudRate[4]; break;
                case ReaderModule.BaudRate.B57600: VM.BaudRateSelectedItem = VM.BaudRate[5]; break;
                case ReaderModule.BaudRate.B115200: VM.BaudRateSelectedItem = VM.BaudRate[6]; break;
                case ReaderModule.BaudRate.B230400: VM.BaudRateSelectedItem = VM.BaudRate[7]; break;
            }

            if (ble != null)
                this._IBLE = ble;

            this._ReaderService = new ReaderService();

            if (this.BLEEnumerateProcess == null)
            {
                this.BLEEnumerateProcess = new DispatcherTimer();
                this.BLEEnumerateProcess.Tick += new EventHandler(DoEnumerateProcessWork);
                this.BLEEnumerateProcess.Interval = TimeSpan.FromMilliseconds(500);
            }
        }




        /// <summary>
        /// Return reader service
        /// </summary>
        /// <returns></returns>
		public ReaderService GetService() { return this._ReaderService; }


        /// <summary>
        /// Return interface type
        /// </summary>
        /// <returns></returns>
        public ReaderService.ConnectType GetIType()
        {
            return _ConnectType;
        }

        /// <summary>
        /// Get COM handler
        /// </summary>
        /// <returns></returns>
        public ICOM GetICOM()
        {
            return _ICOM;
        }

        /// <summary>
        /// Get Net handler
        /// </summary>
        /// <returns></returns>
        public INET GetINET()
        {
            return _INet;
        }

        /// <summary>
        /// Get USB handler
        /// </summary>
        /// <returns></returns>
        public IUSB GetIUSB()
        {
            return _IUSB;
        }

        /// <summary>
        /// Get BLE handler
        /// </summary>
        /// <returns></returns>
        public IBLE GetIBLE()
        {
            return _IBLE;
        }


        /// <summary>
        /// Close the utility
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnConnectDialogCloseClick(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
            if (this._ICOM != null && this._ICOM.IsOpen())
                this._ICOM.Close();
            if (this._INet != null && this._INet.IsConnected())
            {
                this._INet.Close();

                if (this._UDPSocket != null)
                {
                    this._UDPSocket.Close();
                    this._UDPSocket = null;
                }
                OCUPDReceives.Clear();
            }

            if (this._IUSB != null && this._IUSB.IsOpen)
                this._IUSB.Close();

            if (this._IBLE != null)
            {
                this._IBLE.EnumerateStop();
                this._IBLE.Dispose();
            }
                

            this.Close();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnConnectBorderMouseLeftDown(object sender, MouseButtonEventArgs e)
        {
            this.DragMove();
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
		private void ShowOnTBMSG1(String page, String str)
        {
            switch (page)
            {
                case "COM":
                    VM.TBMSG1 = str;
                    VM.TBMSG1FontSize = 12.0;
                    break;
                case "Net":
                    break;
                case "USB":
                    VM.USBTBMSG1 = str;
                    VM.USBTBMSG1FontSize = 12.0;
                    
                    break;
                case "BLE":
                    VM.BLETBMSG1 = str;
                    VM.BLETBMSG1FontSize = 12.0;
                    break;
            }
        }

        private void ShowOnTBMSG1(String page, String str, double size)
        {
            switch (page)
            {
                case "COM":
                    VM.TBMSG1 = str;
                    VM.TBMSG1FontSize = size;
                    break;
                case "Net":
                    break;
                case "USB":
                    VM.USBTBMSG1 = str;
                    VM.USBTBMSG1FontSize = size;
                    break;
                case "BLE":
                    VM.BLETBMSG1 = str;
                    VM.BLETBMSG1FontSize = size;
                    break;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="str"></param>
		private void ShowOnTBMSG2(String page, String str)
        {
            switch (page)
            {
                case "COM":
                    VM.TBMSG2 = str;
                    break;
                case "Net":
                    break;
                case "USB":
                    VM.USBTBMSG2 = str;
                    break;
                case "BLE":
                    VM.BLETBMSG2 = str;
                    break;
            }
        }

        private void TabControlSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            //The sender is a type of TabControl...

            if (sender is TabControl tc)
            {
                var item = (TabItem)tc.SelectedItem;
                if (item != null)
                    switch (item.Header)
                    {

                        case "COM":
                            if (VM.ButtonCOMEnterIsEnabled)
                            {
                                VM.ButtonCOMConnectIsEnabled = false;
                            }
                            break;
                        case "USB":

                            if (this._IUSB != null)
                            {
                                if (this._IUSB.IsOpen)
                                    VM.ButtonUSBEnterIsEnabled = true;
                                else
                                    VM.ButtonUSBEnterIsEnabled = false;
                            }
                            else
                                VM.ButtonUSBEnterIsEnabled = false;
                            break;
                        case "NET":
                            break;
                        case "BLE":
                            OperatingSystem os = Environment.OSVersion;
                            if (os.Version.Major < 6)
                            {
                                ShowOnTBMSG1("BLE", "=== Windows Platform not support BluetoothBLE. ===", 20.0);
                                VM.ButtonBLEEnumerateIsEnabled = false;
                                VM.ButtonBLEConnectIsEnabled = false;
                                VM.ButtonBLEEnterIsEnabled = false;
                            }
                            else if (os.Version.Major == 6 && os.Version.Minor == 1)
                            {
                                ShowOnTBMSG1("BLE", "=== Windows Platform not support BluetoothBLE. ===", 20.0);
                                VM.ButtonBLEEnumerateIsEnabled = false;
                                VM.ButtonBLEConnectIsEnabled = false;
                                VM.ButtonBLEEnterIsEnabled = false;
                            }
                            else {
                                if (this._IBLE != null)
                                {
                                    if (this._IBLE.IsConnected)
                                        VM.ButtonBLEEnterIsEnabled = true;
                                    else
                                        VM.ButtonBLEEnterIsEnabled = false;
                                }
                                else
                                {
                                    VM.ButtonBLEConnectIsEnabled = false;
                                    VM.ButtonBLEEnterIsEnabled = false;
                                }
                            }
                            break;
                        default:
                            break;
                    }
            }
            e.Handled = true;
        }

        #region === COM ===
        /// <summary>
        /// Serach COM device
        /// Add baudrate parameter and modify process (2017/4/20)
        /// </summary>
        private void DoSearchCOMWork() {
            //ObservableCollection<String> oc = new ObservableCollection<String>();
            VM.COMPortBoxItemsSource.Clear();

            foreach (IReader rs in ICOM.GetReaders())
            {
                if (rs != null) {
                    //oc.Add(String.Format(CultureInfo.CurrentCulture, "{0} –{1}", rs.Name, rs.Description));
                    var newItems = new COMPortBox()
                    {
                        Content = String.Format(CultureInfo.CurrentCulture, "{0} –{1}", rs.Name, rs.Description),
                        Name = rs.Name
                    };
                    VM.COMPortBoxAddNewItem(newItems);
                }
                    
                
            }

            if (VM.COMPortBoxItemsSource.Count > 0)
            {
                VM.COMPortBoxSelectedItem = VM.COMPortBoxItemsSource[0];
            }
            //Dispatcher.Invoke(DispatcherPriority.Normal, new Action<ObservableCollection<String>>(CollectionData), oc);
		}


        private ICOM.CombineDataEventHandler _CombineDataHandler;
        private void DoConnectWork()
        {

            try
            {
                //String[] words = (ComboBoxCOMReader.SelectedItem as String).Split(' ');
                String name = VM.COMPortBoxSelectedItem.Name;
                IBaudRate = Int32.Parse(VM.BaudRateSelectedItem.Content, CultureInfo.CurrentCulture);


                this._ICOM = new ICOM();
                this._ICOM.Open(name, IBaudRate, (Parity)Enum.Parse(typeof(Parity), "None", true), 8, (StopBits)Enum.Parse(typeof(StopBits), "One", true));
                this._CombineDataHandler = new ICOM.CombineDataEventHandler(DoReceiveDataWork);
                this._ICOM.CombineDataReceiveEventHandler += this._CombineDataHandler;
                if (this._ICOM.IsOpen())
                {                  
                    this._ICOM.Send(this._ReaderService.CommandV(), ReaderModule.CommandType.Normal);
                    
                    Byte[] b = this._ICOM.Receive();
                    if (b != null)
                    {
                        string s = Format.RemoveCRLF(Format.BytesToString(b));
                        if (s.Contains("V"))
                        {
                            ShowOnTBMSG2("COM", String.Format(CultureInfo.CurrentCulture, "{0}已連接並驗證", name));
                            VM.ButtonCOMEnterIsEnabled = true;
                            VM.ButtonCOMConnectIsEnabled = false;
                        }
                        else
                        {
                            ShowOnTBMSG2("COM", "開啟" + name + "成功，驗證並非是Reader模組");
                            this._ICOM.Close();
                            this._ICOM = null;
                            VM.ButtonCOMConnectIsEnabled = true;
                        }
                    }
                    else
                    {
                        ShowOnTBMSG2("COM", "開啟" + name + "成功，驗證未回覆。");
                        this._ICOM.Close();
                        this._ICOM = null;
                        VM.ButtonCOMConnectIsEnabled = true;
                    }
                }
                else
                {
                    ShowOnTBMSG2("COM", "開啟" + name + "失敗.");
                    this._ICOM.Close();
                    this._ICOM = null;
                    VM.ButtonCOMConnectIsEnabled = true;
                }
                
            }
            catch (UnauthorizedAccessException uae)
            {
                ShowOnTBMSG1("COM", "拒絕對連接埠的存取");
                ShowOnTBMSG2("COM", "[" + uae.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
                this._ICOM.Close();
                this._ICOM = null;
            }
            catch (ArgumentOutOfRangeException outOfRange)
            {
                ShowOnTBMSG1("COM", "一個或多個參數屬性無效");
                ShowOnTBMSG2("COM", "[" + outOfRange.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
                this._ICOM.Close();
                this._ICOM = null;
            }
            catch (ArgumentException ae)
            {
                ShowOnTBMSG1("COM", "連接埠問題");
                ShowOnTBMSG2("COM", "[" + ae.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
                this._ICOM.Close();
                this._ICOM = null;
            }
            catch (System.IO.IOException ie)
            {
                ShowOnTBMSG1("COM", "連接埠問題");
                ShowOnTBMSG2("COM", "[" + ie.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
                this._ICOM.Close();
                this._ICOM = null;
            }
            catch (InvalidOperationException iex) {
                ShowOnTBMSG1("COM", "嘗試連接失敗");
                ShowOnTBMSG2("COM", "[" + iex.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
                this._ICOM.Close();
                this._ICOM = null;
                
            }
            catch (NullReferenceException nre)
            {
                ShowOnTBMSG1("COM", "請選擇一個序列埠");
                ShowOnTBMSG2("COM", "[" + nre.Message + "]");
                VM.ButtonCOMConnectIsEnabled = true;
            }
            finally
            {
                new Thread(DisableMsg).Start();
            }
            
		}


        private void DoReceiveDataWork(object sender, CombineDataReceiveArgumentEventArgs e)
        {
            //
        }


        private void DisableMsg()
        {
            Thread.Sleep(1500);
            ShowOnTBMSG1("COM", "");
            ShowOnTBMSG2("COM", "");
        }
       
		/*private void CollectionData(ObservableCollection<String> s) {
			this.ComboBoxCOMReader.ItemsSource = s;
			this.ComboBoxCOMReader.SelectedIndex = 0;
        }*/

        /// <summary>
        /// When combobox dropdown is opened which to search COM device
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void OnComboboxCOMReaderDropDownOpened(object sender, EventArgs e) {

            //ObservableCollection<string> oc = new ObservableCollection<string>();
            VM.COMPortBoxItemsSource.Clear();

            foreach (IReader rs in ICOM.GetReaders())
            {
                if (rs != null)
                {
                    //oc.Add(string.Format(CultureInfo.CurrentCulture, "{0} –{1}", rs.Name, rs.Description));
                    var newItems = new COMPortBox()
                    {
                        Content = String.Format(CultureInfo.CurrentCulture, "{0} –{1}", rs.Name, rs.Description),
                        Name = rs.Name
                    };
                    VM.COMPortBoxAddNewItem(newItems);
                }
                    
            }

            //if (oc.Count > 0)
            //    CollectionData(oc);
		}

		private void OnButtonCOMConnectClick(object sender, RoutedEventArgs e)
        {
            VM.ButtonCOMConnectIsEnabled = false;
            ShowOnTBMSG1("COM", "");
            ShowOnTBMSG2("COM", "");
            this.SearchThread = new Thread(DoConnectWork) {
                IsBackground = true
            };
            this.SearchThread.Start();
		}

		private void OnButtonCOMEnterClick(object sender, RoutedEventArgs e) {
            this._ConnectType = ReaderService.ConnectType.COM;
            this.DialogResult = true;
		}

        #endregion


        #region === USB ===

        /// <summary>
        /// Serach USB device
        /// </summary>
        /// <param name="parameterObject"></param>
        /*private void DoSearchUSBWork()
        {
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                ObservableCollection<String> _oc = new ObservableCollection<String>();

                this._IUSBs = IUSB.Enumerate();
                for (int i = 0; i < this._IUSBs.Count(); i++)
                {
                    String _productName = this._IUSBs.ElementAt(i).ProductName ?? "NULL";
                    _oc.Add($"{i + 1}: {_productName} (VID: 0x{this._IUSBs.ElementAt(i).VendorId}, PID: 0x{this._IUSBs.ElementAt(i).ProductID})");
                }

                if (_oc.Count > 0)
                {
                    this.ComboBoxUSBReader.ItemsSource = _oc;
                    this.ComboBoxUSBReader.SelectedIndex = 0;
                }
            }));
        }*/


        private void OnButtonUSBEnumerateClick(object sender, RoutedEventArgs e)
        {
            VM.ButtonUSBConnectIsEnabled = true;
            this.SearchThread = new Thread(DoUSBEnumerateWork) {
                IsBackground = true
            };
            this.SearchThread.Start();
        }

        private void DoUSBEnumerateWork()
        {

            ShowOnTBMSG1("USB", "Get HID device..");

            new Thread(USBDisableMsg1).Start();


            ObservableCollection<String> _oc = new ObservableCollection<String>();
            this._IUSBs = IUSB.Enumerate();

            for (int i = 0; i < this._IUSBs.Count(); i++)
            {
                String _productName = this._IUSBs.ElementAt(i).ProductName ?? "NULL";

                ShowOnTBMSG2("USB", $"{i + 1}: {_productName} ");


                _oc.Add($"{i + 1}: {_productName} (VID: 0x{this._IUSBs.ElementAt(i).VendorId}, PID: 0x{this._IUSBs.ElementAt(i).ProductID})");
            }

            if (_oc.Count > 0)
            {
                Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                    this.ComboBoxUSBReader.ItemsSource = _oc;
                    this.ComboBoxUSBReader.SelectedIndex = 0;
                }));
            }
            new Thread(USBDisableMsg2).Start();
        }

        private void USBDisableMsg1()
        {
            Thread.Sleep(2000);
            ShowOnTBMSG1("USB", "");
        }

        private void USBDisableMsg2()
        {
            Thread.Sleep(1500);
            //Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                ShowOnTBMSG2("USB", "");
            //}));
        }

        private void OnButtonUSBConnectClick(object sender, RoutedEventArgs e)
        {
            try
            {
                this._IUSB = _IUSBs.ElementAt(ComboBoxUSBReader.SelectedIndex);
                if (this._IUSB != null)
                {
                    this._IUSB.Open();
                    if (this._IUSB.IsOpen)
                    {
                        VM.ButtonUSBEnterIsEnabled = true;
                        VM.ButtonUSBConnectIsEnabled = false;
                        ShowOnTBMSG2("USB", _IUSB.ProductName + "成功開啟");
                    }
                    else {
                        ShowOnTBMSG2("USB", "USB裝置未成功開啟");
                        
                    }
                    new Thread(USBDisableMsg2).Start();
                }
            }
            catch (ArgumentNullException ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void OnButtonUSBEnterClick(object sender, RoutedEventArgs e)
        {
            this._ConnectType = ReaderService.ConnectType.USB;
            this.DialogResult = true;
        }
        #endregion


        #region === NET ===
        private void CheckBoxNetManageGroup(object sender, RoutedEventArgs e)
        {
            var cb = sender as CheckBox;
            switch (cb.Content.ToString())
            {
                case "Net Device List:":
                    CheckBoxNetAssign.IsChecked = false;
                    break;
                case "Net Assign:":
                    CheckBoxNetSearch.IsChecked = false;
                    break;
            }
        }

        private void DoSerachNetWork()
        {
            var b = System.Text.Encoding.UTF8.GetBytes("ReaderUtilityIOT");
            IPEndPoint iep = new IPEndPoint(IPAddress.Parse("255.255.255.255"), 8877);

            this._UDPSocket.Send(b, b.Length, iep);
            this._UDPSocket.BeginReceive(new AsyncCallback(OnUDPReceived), _UDPSocket);
            UPDReceiveDone.WaitOne(2000);
        }

        private void OnUDPReceived(IAsyncResult asyn)
        {
            try
            {
                UdpClient uc = (UdpClient)asyn.AsyncState;
                IPEndPoint iep = new IPEndPoint(IPAddress.Any, 0);

                byte[] data = uc.EndReceive(asyn, ref iep);
                if (data.Length > 0)
                {
                    String mac = Format.ByteToHexString(data[0]) + ':' + Format.ByteToHexString(data[1]) + ':' +
                                Format.ByteToHexString(data[2]) + ':' + Format.ByteToHexString(data[3]) + ':' +
                                Format.ByteToHexString(data[4]) + ':' + Format.ByteToHexString(data[5]);

                    Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                        for (int i = 0; i < ComboBoxNetReader.Items.Count; i++)
                        {
                            var val = ((ComboBoxNetReader.Items[i] as String).Split(';'))[0];
                            if (val.Equals(iep.Address.ToString(), StringComparison.CurrentCulture)) 
                                return;
                        }

                        OCUPDReceives.Add(iep.Address.ToString() + "; MAC Address =" + mac);

                        ComboBoxNetReader.ItemsSource = OCUPDReceives;
                        ComboBoxNetReader.SelectedIndex = 0;

                    }));

                }
                UPDReceiveDone.Set();
                UPDReceiveDone.Reset();
            }
            catch (ArgumentNullException e)
            {
                MessageBox.Show(e.Message);
            }
        }

        private void OnButtonNetSearchClick(object sender, RoutedEventArgs e)
        {
            OCUPDReceives.Clear();

            if (this._UDPSocket == null)
            {
                try
                {
                    this._UDPSocket = new UdpClient(8878);

                }
                catch (SocketException ee)
                {
                    MessageBox.Show(ee.Message);
                }

            }

            this.NetSearchThread = new Thread(DoSerachNetWork) {
                IsBackground = true
            };
            this.NetSearchThread.Start();
        }

        private void OnButtonNetConnectClick(object sender, RoutedEventArgs e)
        {
            if (CheckBoxNetAssign.IsChecked.Value == true)
            {
                try
                {
                    _INet = new INET(TextBoxNetIP.Text, TextBoxNetPort.Text);
                    if (this._INet.IsConnected())
                    {
                        this._ConnectType = ReaderService.ConnectType.NET;
                        this.DialogResult = true;
                    }
                }
                catch (SocketException ex)
                {
                    MessageBox.Show(ex.Message);
                }

            }
            else if (CheckBoxNetSearch.IsChecked.Value == true)
            {
                String ip = String.Empty;
                try
                {
                    ip = ((ComboBoxNetReader.SelectedValue as String).Split(';'))[0];
                    _INet = new INET(ip, "8800");
                    if (this._INet.IsConnected())
                    {
                        this._ConnectType = ReaderService.ConnectType.NET;

                        if (this._UDPSocket != null)
                        {
                            //this._UDPSocket.Dispose();
                            this._UDPSocket.Close();
                            this._UDPSocket = null;
                        }
                        this.DialogResult = true;
                    }
                }
                catch (SocketException ex)
                {
                    MessageBox.Show(ex.Message, ip);
                }
            }
        }
        #endregion


        #region === BLE ===
        private void OnButtonBLEEnumerateClick(object sender, RoutedEventArgs e)
        {


            //VM.ButtonBLEConnectIsEnabled = false;
            VM.ButtonBLEEnumerateIsEnabled = false;

            this.SearchThread = new Thread(DoBLEEnumerateWork)
            {
                IsBackground = true
            };
            this.SearchThread.Start();
            
        }

        private void DoBLEEnumerateWork()
        {
            _timeCount = 30;
            ShowOnTBMSG2("BLE", "");
            ShowOnTBMSG1("BLE", "");
            
            
            this.BLEEnumerateProcess.Start();
            

            VM.BLEDeviceUnpairedItemsSource.Clear();
            if (this._IBLE != null)
                this._IBLE.EnumerateStop();
            this._IBLE = new IBLE(DeviceSelector.BluetoothLeUnpairedOnly);
            this._IBLE.DeviceAdded += OnBLEDeviceAdded;
            this._IBLE.DeviceUpdated += OnBLEDeviceUpdated;
            this._IBLE.DeviceRemoved += OnBLEDeviceRemoved;
            this._IBLE.DeviceEnumerationCompleted += OnBLEDeviceEnumerationCompleted;
            this._IBLE.EnumerateStart();
        }


        private uint _toggleCount = 0;
        private uint _timeCount = 30;
        private void DoEnumerateProcessWork(object sender, EventArgs e) {

            if (_toggleCount%2 == 0) {
                _timeCount--;
            }
            switch (_toggleCount)
            {
                case 0:
                default:
                    _toggleCount = 1;
                    ShowOnTBMSG1("BLE", String.Format(CultureInfo.CurrentCulture, "Enumerate BLE device   ({0})", _timeCount.ToString("00", CultureInfo.CurrentCulture)));
                    break;
                case 1:
                    _toggleCount++;
                    ShowOnTBMSG1("BLE", String.Format(CultureInfo.CurrentCulture, "Enumerate BLE device.  ({0})", _timeCount.ToString("00", CultureInfo.CurrentCulture)));
                    break;
                case 2:
                    _toggleCount++;
                    ShowOnTBMSG1("BLE", String.Format(CultureInfo.CurrentCulture, "Enumerate BLE device.. ({0})", _timeCount.ToString("00", CultureInfo.CurrentCulture)));
                    break;
                case 3:
                    _toggleCount = 0;
                    ShowOnTBMSG1("BLE", String.Format(CultureInfo.CurrentCulture, "Enumerate BLE device...({0})", _timeCount.ToString("00", CultureInfo.CurrentCulture)));
                    break;
            }

        }

        private async void OnBLEDeviceAdded(object sender, Service.IInterface.BLE.Events.DeviceAddedEventArgs e)
        {
            await RunOnUiThread(() =>
            {
                if (!String.IsNullOrEmpty(e.Device.Name)) {
                    var bleDevice = new BLEListViewItem();
                    string str = e.Device.UUID.Substring(e.Device.UUID.Length - 17);
                    bleDevice.DeviceName = e.Device.Name;
                    bleDevice.DeviceUUID = e.Device.UUID;
                    bleDevice.ShowDeviceUUID = str.ToUpper(CultureInfo.CurrentCulture);
                    
                    VM.BLEListViewAddNewItem(bleDevice);
                    VM.ButtonBLEConnectIsEnabled = true;
                }
                
            }).ConfigureAwait(false);
        }

        private async void OnBLEDeviceUpdated(object sender, Service.IInterface.BLE.Events.DeviceUpdatedEventArgs e)
        {
            await RunOnUiThread(() =>
            {
                var sourceItem = VM.BLEDeviceUnpairedItemsSource.FirstOrDefault(a => a.DeviceUUID == e.Device.UUID);
                if (sourceItem != null)
                {
                    var destItem = new BLEListViewItem();
                    string str = sourceItem.DeviceUUID.Substring(e.Device.UUID.Length - 17);
                    destItem.DeviceName = sourceItem.DeviceName;
                    destItem.DeviceUUID = sourceItem.DeviceUUID;
                    destItem.ShowDeviceUUID = str.ToUpper(CultureInfo.CurrentCulture);

                    VM.BLEListViewUpdatedItem(sourceItem, destItem);
                }
                    
            }).ConfigureAwait(false);
        }

        private async void OnBLEDeviceRemoved(object sender, Service.IInterface.BLE.Events.DeviceRemovedEventArgs e)
        {
            await RunOnUiThread(() =>
            {
                var foundItem = VM.BLEDeviceUnpairedItemsSource.FirstOrDefault(a => a.DeviceUUID == e.Device.UUID);
                if (foundItem != null)
                    VM.BLEListViewRemoveItem(foundItem);
            }).ConfigureAwait(false);
        }

        private async void OnBLEDeviceEnumerationCompleted(object sender, object objj)
        {
            await RunOnUiThread(() =>
            {
                this.BLEEnumerateProcess.Stop();
                
                VM.ButtonBLEEnumerateIsEnabled = true;
                if (_timeCount > 10)
                    ShowOnTBMSG1("BLE", "Enumerate BLE device completed, Maybe don't insert the dongle.");
                else
                    ShowOnTBMSG1("BLE", "Enumerate BLE device completed.");

            }).ConfigureAwait(false);
        }

        private async Task RunOnUiThread(Action a)
        {
            await this.Dispatcher.InvokeAsync(() =>
            {
                a();
            });
        }

        private void BLEDisableMsg1()
        {
            Thread.Sleep(2000);
            ShowOnTBMSG1("BLE", "");
        }

        private void BLEDisableMsg2()
        {
            Thread.Sleep(1500);
            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(() => {
                ShowOnTBMSG2("BLE", "");
            }));
        }

        private async void OnButtonBLEConnectClick(object sender, RoutedEventArgs e)
        {
            var item = VM.BLEDeviceUnpairedItemsSelected;

            if (item != null)
            {
                this.BLEEnumerateProcess.Stop();
                ShowOnTBMSG1("BLE", "Connecting...");

                var result = await _IBLE.ConnectAsync(item.DeviceUUID).ConfigureAwait(false);
                if (result.IsConnected)
                {
                    VM.ButtonBLEEnterIsEnabled = true;
                    VM.ButtonBLEEnumerateIsEnabled = true;
                    VM.ButtonBLEConnectIsEnabled = false;
                    _IBLE.EnumerateStop();
                    ShowOnTBMSG1("BLE", "");
                    ShowOnTBMSG2("BLE", String.Format(CultureInfo.CurrentCulture, "{0} is connected.", result.Name));
                    
                }
                else {
                    VM.ButtonBLEEnumerateIsEnabled = true;
                    VM.ButtonBLEConnectIsEnabled = false;
                    _IBLE.EnumerateStop();
                    ShowOnTBMSG1("BLE", "");
                    ShowOnTBMSG2("BLE", String.Format(CultureInfo.CurrentCulture, "[Error]:{0}", result.ErrorMessage));  
                }
                
            }
            else
            {
                ShowOnTBMSG1("BLE", "");
                ShowOnTBMSG2("BLE", "Must select an unpaired/unconnected device");
                
            }
        }

        private void OnButtonBLEEnterClick(object sender, RoutedEventArgs e)
        {
            this._ConnectType = ReaderService.ConnectType.BLE;
            this.DialogResult = true;
        }



        #endregion


        #region IDisposable Support
        private bool disposedValue = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    this.DialogResult = false;
                    if (_UDPSocket != null) { _UDPSocket.Dispose(); }
                    if (this._ICOM != null) {
                        this._ICOM.CombineDataReceiveEventHandler -= this._CombineDataHandler;
                        this._ICOM.Dispose(); 
                    }
                    if (this._INet != null) { this._INet.Dispose(); OCUPDReceives.Clear(); }
                    if (this._IUSB != null) { this._IUSB.Dispose(); }
                    if (this._IBLE != null) { this._IBLE.Dispose(); }
                    //OnConnectDialogCloseClick(null, null);
                    this.Close();
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
