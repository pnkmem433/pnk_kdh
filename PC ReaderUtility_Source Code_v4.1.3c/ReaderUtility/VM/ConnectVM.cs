using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace RFID.Utility.VM
{
    public class ConnectVM : INotifyPropertyChanged
    {
        private readonly object _blestocksLock = new object();
        public ObservableCollection<BLEListViewItem> BLEDeviceUnpairedItemsSource { get; private set; }

        private readonly object _comstocksLock = new object();
        public ObservableCollection<COMPortBox> COMPortBoxItemsSource { get; private set; }

        public ObservableCollection<BaudRate> BaudRate { get; private set; }

        public ConnectVM()
        {
            BLEDeviceUnpairedItemsSource = new ObservableCollection<BLEListViewItem>();
            BindingOperations.EnableCollectionSynchronization(BLEDeviceUnpairedItemsSource, _blestocksLock);

            COMPortBoxItemsSource = new ObservableCollection<COMPortBox>();
            BindingOperations.EnableCollectionSynchronization(COMPortBoxItemsSource, _comstocksLock);

            BaudRate = new ObservableCollection<BaudRate>
            {
                new BaudRate { Content="4800", Tag="0" },
                new BaudRate { Content="9600", Tag="1"},
                new BaudRate { Content="14400", Tag="2"},
                new BaudRate { Content="19200",  Tag="3"},
                new BaudRate { Content="38400",  Tag="4"},
                new BaudRate { Content="57600",  Tag="5"},
                new BaudRate { Content="115200",  Tag="6"},
                new BaudRate { Content="230400",  Tag="7"}
            };
        }


        public void BLEListViewAddNewItem(BLEListViewItem items)
        {
            BLEDeviceUnpairedItemsSource.Add(items);
        }

        public void BLEListViewUpdatedItem(BLEListViewItem Sourceitems, BLEListViewItem Destitems)
        {
            BLEDeviceUnpairedItemsSource.Insert(BLEDeviceUnpairedItemsSource.IndexOf(Sourceitems), Destitems);
            BLEDeviceUnpairedItemsSource.Remove(Sourceitems);
        }

        public void BLEListViewRemoveItem(BLEListViewItem items)
        {
            BLEDeviceUnpairedItemsSource.Remove(items);
        }

        private BLEListViewItem _BLEDeviceUnpairedItemsSelected;
        public BLEListViewItem BLEDeviceUnpairedItemsSelected
        {
            get { return _BLEDeviceUnpairedItemsSelected; }
            set
            {
                if (value != _BLEDeviceUnpairedItemsSelected)
                {
                    _BLEDeviceUnpairedItemsSelected = value;
                    OnPropertyChanged("BLEDeviceUnpairedItemsSelected");
                }
            }
        }



        public void COMPortBoxAddNewItem(COMPortBox items)
        {
            COMPortBoxItemsSource.Add(items);
        }


        private COMPortBox _COMPortBoxSelectedItem;
        public COMPortBox COMPortBoxSelectedItem
        {
            get { return _COMPortBoxSelectedItem; }
            set
            {
                if (_COMPortBoxSelectedItem != value)
                {
                    _COMPortBoxSelectedItem = value;
                    OnPropertyChanged("COMPortBoxSelectedItem");
                }
            }
        }


        private BaudRate _BaudRateSelectedItem;
        public BaudRate BaudRateSelectedItem
        {
            get { return _BaudRateSelectedItem; }
            set
            {
                if (_BaudRateSelectedItem != value)
                {
                    _BaudRateSelectedItem = value;
                    OnPropertyChanged("BaudRateSelectedItem");
                }
            }
        }


        #region COM page
        private double _TBMSG1FontSize = 12.0;
        public double TBMSG1FontSize
        {
            get { return _TBMSG1FontSize; }
            set
            {
                if (_TBMSG1FontSize != value)
                {
                    _TBMSG1FontSize = value;
                    OnPropertyChanged("TBMSG1FontSize");
                }
            }
        }


        private String _TBMSG1 = String.Empty;
        public String TBMSG1
        {
            get { return _TBMSG1; }
            set
            {
                if (_TBMSG1 != value)
                {
                    _TBMSG1 = value;
                    OnPropertyChanged("TBMSG1");
                }
            }
        }

        private String _TBMSG2 = String.Empty;
        public String TBMSG2
        {
            get { return _TBMSG2; }
            set
            {
                if (_TBMSG2 != value)
                {
                    _TBMSG2 = value;
                    OnPropertyChanged("TBMSG2");
                }
            }
        }

        private Boolean _ButtonCOMEnterIsEnabled = false;
        public Boolean ButtonCOMEnterIsEnabled
        {
            get { return _ButtonCOMEnterIsEnabled; }
            set
            {
                if (_ButtonCOMEnterIsEnabled != value)
                {
                    _ButtonCOMEnterIsEnabled = value;
                    OnPropertyChanged("ButtonCOMEnterIsEnabled");
                }
            }
        }

        private Boolean _ButtonCOMConnectIsEnabled = true;
        public Boolean ButtonCOMConnectIsEnabled
        {
            get { return _ButtonCOMConnectIsEnabled; }
            set
            {
                if (_ButtonCOMConnectIsEnabled != value)
                {
                    _ButtonCOMConnectIsEnabled = value;
                    OnPropertyChanged("ButtonCOMConnectIsEnabled");
                }
            }
        }
        #endregion


        #region USB Page
        private double _USBTBMSG1FontSize = 12.0;
        public double USBTBMSG1FontSize
        {
            get { return _USBTBMSG1FontSize; }
            set
            {
                if (_USBTBMSG1FontSize != value)
                {
                    _USBTBMSG1FontSize = value;
                    OnPropertyChanged("USBTBMSG1FontSize");
                }
            }
        }


        private String _USBTBMSG1 = String.Empty;
        public String USBTBMSG1
        {
            get { return _USBTBMSG1; }
            set
            {
                if (_USBTBMSG1 != value)
                {
                    _USBTBMSG1 = value;
                    OnPropertyChanged("USBTBMSG1");
                }
            }
        }

        private String _USBTBMSG2 = String.Empty;
        public String USBTBMSG2
        {
            get { return _USBTBMSG2; }
            set
            {
                if (_USBTBMSG2 != value)
                {
                    _USBTBMSG2 = value;
                    OnPropertyChanged("USBTBMSG2");
                }
            }
        }


        private Boolean _ButtonUSBEnumerateIsEnabled = true;
        public Boolean ButtonUSBEnumerateIsEnabled
        {
            get { return _ButtonUSBEnumerateIsEnabled; }
            set
            {
                if (_ButtonUSBEnumerateIsEnabled != value)
                {
                    _ButtonUSBEnumerateIsEnabled = value;
                    OnPropertyChanged("ButtonUSBEnumerateIsEnabled");
                }
            }
        }

        private Boolean _ButtonUSBConnectIsEnabled = false;
        public Boolean ButtonUSBConnectIsEnabled
        {
            get { return _ButtonUSBConnectIsEnabled; }
            set
            {
                if (_ButtonUSBConnectIsEnabled != value)
                {
                    _ButtonUSBConnectIsEnabled = value;
                    OnPropertyChanged("ButtonUSBConnectIsEnabled");
                }
            }
        }

        private Boolean _ButtonUSBEnterIsEnabled = false;
        public Boolean ButtonUSBEnterIsEnabled
        {
            get { return _ButtonUSBEnterIsEnabled; }
            set
            {
                if (_ButtonUSBEnterIsEnabled != value)
                {
                    _ButtonUSBEnterIsEnabled = value;
                    OnPropertyChanged("ButtonUSBEnterIsEnabled");
                }
            }
        }
        #endregion



        private double _BLETBMSG1FontSize = 12.0;
        public double BLETBMSG1FontSize
        {
            get { return _BLETBMSG1FontSize; }
            set
            {
                if (_BLETBMSG1FontSize != value)
                {
                    _BLETBMSG1FontSize = value;
                    OnPropertyChanged("BLETBMSG1FontSize");
                }
            }
        }


        private String _BLETBMSG1 = String.Empty;
        public String BLETBMSG1
        {
            get { return _BLETBMSG1; }
            set
            {
                if (_BLETBMSG1 != value)
                {
                    _BLETBMSG1 = value;
                    OnPropertyChanged("BLETBMSG1");
                }
            }
        }

        private String _BLETBMSG2 = String.Empty;
        public String BLETBMSG2
        {
            get { return _BLETBMSG2; }
            set
            {
                if (_BLETBMSG2 != value)
                {
                    _BLETBMSG2 = value;
                    OnPropertyChanged("BLETBMSG2");
                }
            }
        }

        private bool _ButtonBLEEnumerateIsEnabled = true;
        public bool ButtonBLEEnumerateIsEnabled
        {
            get { return _ButtonBLEEnumerateIsEnabled; }
            set
            {
                if (_ButtonBLEEnumerateIsEnabled != value)
                {
                    _ButtonBLEEnumerateIsEnabled = value;
                    OnPropertyChanged("ButtonBLEEnumerateIsEnabled");
                }
            }
        }

        private bool _ButtonBLEConnectIsEnabled = false;
        public bool ButtonBLEConnectIsEnabled
        {
            get { return _ButtonBLEConnectIsEnabled; }
            set
            {
                if (_ButtonBLEConnectIsEnabled != value)
                {
                    _ButtonBLEConnectIsEnabled = value;
                    OnPropertyChanged("ButtonBLEConnectIsEnabled");
                }
            }
        }


        private bool _ButtonBLEEnterIsEnabled = false;
        public bool ButtonBLEEnterIsEnabled
        {
            get { return _ButtonBLEEnterIsEnabled; }
            set
            {
                if (_ButtonBLEEnterIsEnabled != value)
                {
                    _ButtonBLEEnterIsEnabled = value;
                    OnPropertyChanged("ButtonBLEEnterIsEnabled");
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(String property)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(property));
        }
    }
}
