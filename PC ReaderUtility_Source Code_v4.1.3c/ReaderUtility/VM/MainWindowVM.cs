using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace RFID.Utility.VM
{
    /// <summary>
    /// 
    /// </summary>
    /*public class CommandBase : ICommand
    {
        readonly Action<object> execute;
        readonly Predicate<object> canExecute;

        public CommandBase(Action<object> executeDelegate, Predicate<object> canExecuteDelegate)
        {
            execute = executeDelegate;
            canExecute = canExecuteDelegate;
        }

        bool ICommand.CanExecute(object parameter)
        {
            return CanExecute(object parameter);
            //return canExecute == null ? true : canExecute(parameter);
        }

        protected bool CanExecute(object parameter)
        {
            return canExecute == null ? true : canExecute(parameter);
        }

        event EventHandler ICommand.CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        void ICommand.Execute(object parameter)
        {
            execute(parameter);
        }
    }*/


    public class INETInfo
    {
        public String IP { get; set; } = String.Empty;
        public String Port { get; set; } = String.Empty;
    }


    /// <summary>
    /// 
    /// </summary>
    public class MainWindowVM : INotifyPropertyChanged
    {
        public ObservableCollection<MemBank> MemBank { get; private set; }
        public ObservableCollection<SlotQ> SlotQ { get; private set; }
        public ObservableCollection<B01ListboxItem> B01GroupMsgListBox { get; private set; }
        public ObservableCollection<GPIOConfig> GPIOConfig { get; private set; }

        private readonly object _b02stocksLock = new object();
        public ObservableCollection<B02ListViewItem> B02ListViewItemsSource { get; private set; }

        private readonly object _b03stocksLock = new object();
        public ObservableCollection<B03ListViewItem> B03ListViewItemsSource { get; private set; }

        private readonly object _b04stocksLock = new object();
        public ObservableCollection<B04ListViewItem> B04ListViewItemsSource { get; private set; }

        

        public MainWindowVM()
        {
            MemBank = new ObservableCollection<MemBank>
            {
                new MemBank { Content="00: RESERVED",  Tag="0" },
                new MemBank { Content="01: EPC",  Tag="1"},
                new MemBank { Content="02: TID",  Tag="2"},
                new MemBank { Content="03: USER",  Tag="3"},
                new MemBank { Content="--------",  Tag="X"},
                new MemBank { Content="00: RESERVED(KillPwd)",  Tag="0"},
                new MemBank { Content="00: RESERVED(AccessPwd)",  Tag="0"}
            };

            SlotQ = new ObservableCollection<SlotQ>
            {
                new SlotQ { Content="1",  Tag="1" },
                new SlotQ { Content="2",  Tag="2" },
                new SlotQ { Content="3",  Tag="3" },
                new SlotQ { Content="4",  Tag="4" },
                new SlotQ { Content="5",  Tag="5" },
                new SlotQ { Content="6",  Tag="6" },
                new SlotQ { Content="7",  Tag="7" },
                new SlotQ { Content="8",  Tag="8" },
                new SlotQ { Content="9",  Tag="9" },
                new SlotQ { Content="10",  Tag="A" }
            };

            GPIOConfig = new ObservableCollection<GPIOConfig>
            {
                new GPIOConfig { Content="0",  Tag="0" },
                new GPIOConfig { Content="1",  Tag="1" },
                new GPIOConfig { Content="2",  Tag="2" },
                new GPIOConfig { Content="3",  Tag="3" },
                new GPIOConfig { Content="4",  Tag="4" },
                new GPIOConfig { Content="5",  Tag="5" },
                new GPIOConfig { Content="6",  Tag="6" },
                new GPIOConfig { Content="7",  Tag="7" }
            };

            B01GroupMsgListBox = new ObservableCollection<B01ListboxItem>();

            B02ListViewItemsSource = new ObservableCollection<B02ListViewItem>();
            BindingOperations.EnableCollectionSynchronization(B02ListViewItemsSource, _b02stocksLock);

            B03ListViewItemsSource = new ObservableCollection<B03ListViewItem>();
            BindingOperations.EnableCollectionSynchronization(B03ListViewItemsSource, _b03stocksLock);

            B04ListViewItemsSource = new ObservableCollection<B04ListViewItem>();
            BindingOperations.EnableCollectionSynchronization(B04ListViewItemsSource, _b04stocksLock);

            //B03ListViewItemClick = new CommandBase(i => this.B03ListViewItemFlashRun(), null);
        }



        #region === B01 Group Pre-Set ===
        private Boolean _B01GroupPreSetSelectIsEnabled = true;
        private Boolean _B01GroupPreSetSelectCheckBoxIsEnabled = true;
        private MemBank _B01GroupPreSetSelectMemBank;
        private String _B01GroupPreSetSelectBitAddress = String.Empty;
        private String _B01GroupPreSetSelectBitLength = String.Empty;
        private String _B01GroupPreSetSelectBitData = String.Empty;
        private String _B01GroupPreSetAccessPassword = String.Empty;


        public Boolean B01GroupPreSetSelectIsEnabled
        {
            get { return _B01GroupPreSetSelectIsEnabled; }
            set
            {
                if (_B01GroupPreSetSelectIsEnabled != value)
                {
                    _B01GroupPreSetSelectIsEnabled = value;
                    OnPropertyChanged("B01GroupPreSetSelectIsEnabled");
                }
            }
        }

        public Boolean B01GroupPreSetSelectCheckBoxIsEnabled
        {
            get { return _B01GroupPreSetSelectCheckBoxIsEnabled; }
            set
            {
                if (_B01GroupPreSetSelectCheckBoxIsEnabled != value)
                {
                    _B01GroupPreSetSelectCheckBoxIsEnabled = value;
                    OnPropertyChanged("B01GroupPreSetSelectCheckBoxIsEnabled");
                }
            }
        }

        private Boolean _B01GroupPreSetSelectCheckBoxIsChecked = false;
        public Boolean B01GroupPreSetSelectCheckBoxIsChecked
        {
            get { return _B01GroupPreSetSelectCheckBoxIsChecked; }
            set
            {
                if (_B01GroupPreSetSelectCheckBoxIsChecked != value)
                {
                    _B01GroupPreSetSelectCheckBoxIsChecked = value;
                    OnPropertyChanged("B01GroupPreSetSelectCheckBoxIsChecked");
                }
            }
        }

        private Boolean _B01GroupPreSetAccessheckBoxIsChecked = false;
        public Boolean B01GroupPreSetAccessheckBoxIsChecked
        {
            get { return _B01GroupPreSetAccessheckBoxIsChecked; }
            set
            {
                if (_B01GroupPreSetAccessheckBoxIsChecked != value)
                {
                    _B01GroupPreSetAccessheckBoxIsChecked = value;
                    OnPropertyChanged("B01GroupPreSetAccessheckBoxIsChecked");
                }
            }
        }

        public MemBank B01GroupPreSetSelectMemBank
        {
            get { return _B01GroupPreSetSelectMemBank; }
            set
            {
                if (_B01GroupPreSetSelectMemBank != value)
                {
                    _B01GroupPreSetSelectMemBank = value;
                    OnPropertyChanged("B01GroupPreSetSelectMemBank");
                }
            }
        }

        public String B01GroupPreSetSelectBitAddress
        {
            get { return _B01GroupPreSetSelectBitAddress; }
            set
            {
                if (_B01GroupPreSetSelectBitAddress != value)
                {
                    _B01GroupPreSetSelectBitAddress = value;
                    OnPropertyChanged("B01GroupPreSetSelectBitAddress");
                }
            }
        }

        public String B01GroupPreSetSelectBitLength
        {
            get { return _B01GroupPreSetSelectBitLength; }
            set
            {
                if (_B01GroupPreSetSelectBitLength != value)
                {
                    _B01GroupPreSetSelectBitLength = value;
                    OnPropertyChanged("B01GroupPreSetSelectBitLength");
                }
            }
        } 

        public String B01GroupPreSetSelectBitData
        {
            get { return _B01GroupPreSetSelectBitData; }
            set
            {
                if (_B01GroupPreSetSelectBitData != value)
                {
                    _B01GroupPreSetSelectBitData = value;
                    OnPropertyChanged("B01GroupPreSetSelectBitData");
                }
            }
        }

        public String B01GroupPreSetAccessPassword
        {
            get { return _B01GroupPreSetAccessPassword; }
            set
            {
                if (_B01GroupPreSetAccessPassword != value)
                {
                    _B01GroupPreSetAccessPassword = value;
                    OnPropertyChanged("B01GroupPreSetAccessPassword");
                }
            }
        }
        #endregion

        #region === B01 Group EPC ===
        private String _B01GroupEPCTextBoxEPC = String.Empty;
        private String _B01GroupEPCTextBoxTID = String.Empty;
        private String _B01GroupEPCTextBoxTIDLength = String.Empty;
        private Boolean _B01GroupEPCIsEnabled = true;

        public String B01GroupEPCTextBoxEPC
        {
            get { return _B01GroupEPCTextBoxEPC; }
            set
            {
                if (_B01GroupEPCTextBoxEPC != value)
                {
                    _B01GroupEPCTextBoxEPC = value;
                    OnPropertyChanged("B01GroupEPCTextBoxEPC");
                }
            }
        }

        public String B01GroupEPCTextBoxTID
        {
            get { return _B01GroupEPCTextBoxTID; }
            set
            {
                if (_B01GroupEPCTextBoxTID != value)
                {
                    _B01GroupEPCTextBoxTID = value;
                    OnPropertyChanged("B01GroupEPCTextBoxTID");
                }
            }
        }
       
        public String B01GroupEPCTextBoxTIDLength
        {
            get { return _B01GroupEPCTextBoxTIDLength; }
            set
            {
                if (_B01GroupEPCTextBoxTIDLength != value)
                {
                    _B01GroupEPCTextBoxTIDLength = value;
                    OnPropertyChanged("B01GroupEPCTextBoxTIDLength");
                }
            }
        }

        public Boolean B01GroupEPCIsEnabled
        {
            get { return _B01GroupEPCIsEnabled; }
            set
            {
                if (_B01GroupEPCIsEnabled != value)
                {
                    _B01GroupEPCIsEnabled = value;
                    OnPropertyChanged("B01GroupEPCIsEnabled");
                }
            }
        }
        #endregion

        #region === B01 Group RW ===
        private MemBank _B01GroupRWComboBoxMemBank;
        private String _B01GroupRWTextBoxAddress = "2";
        private String _B01GroupRWTextBoxLength = "6";
        private String _B01GroupRWTextBoxWrite = String.Empty;
        private String _B01GroupRWTextBoxRead = String.Empty;
        private Boolean _B01GroupRWIsEnabled = true;


        public MemBank B01GroupRWComboBoxMemBank
        {
            get { return _B01GroupRWComboBoxMemBank; }
            set
            {
                if (_B01GroupRWComboBoxMemBank != value)
                {
                    _B01GroupRWComboBoxMemBank = value;
                    OnPropertyChanged("B01GroupRWComboBoxMemBank");
                }
            }
        }

        public String B01GroupRWTextBoxAddress
        {
            get { return _B01GroupRWTextBoxAddress; }
            set
            {
                if (_B01GroupRWTextBoxAddress != value)
                {
                    _B01GroupRWTextBoxAddress = value;
                    OnPropertyChanged("B01GroupRWTextBoxAddress");
                }
            }
        }
        
        public String B01GroupRWTextBoxLength
        {
            get { return _B01GroupRWTextBoxLength; }
            set
            {
                if (_B01GroupRWTextBoxLength != value)
                {
                    _B01GroupRWTextBoxLength = value;
                    OnPropertyChanged("B01GroupRWTextBoxLength");
                }
            }
        }

        public String B01GroupRWTextBoxWrite
        {
            get { return _B01GroupRWTextBoxWrite; }
            set
            {
                if (_B01GroupRWTextBoxWrite != value)
                {
                    _B01GroupRWTextBoxWrite = value;
                    OnPropertyChanged("B01GroupRWTextBoxWrite");
                }
            }
        }

        public String B01GroupRWTextBoxRead
        {
            get { return _B01GroupRWTextBoxRead; }
            set
            {
                if (_B01GroupRWTextBoxRead != value)
                {
                    _B01GroupRWTextBoxRead = value;
                    OnPropertyChanged("B01GroupRWTextBoxRead");
                }
            }
        }

        public Boolean B01GroupRWIsEnabled
        {
            get { return _B01GroupRWIsEnabled; }
            set
            {
                if (_B01GroupRWIsEnabled != value)
                {
                    _B01GroupRWIsEnabled = value;
                    OnPropertyChanged("B01GroupRWIsEnabled");
                }
            }
        }
        #endregion

        #region === B01 Group Lock === 
        private String _B01GroupLockTextBoxMask = String.Empty;
        private String _B01GroupLockTextBoxAction = String.Empty;
        private Boolean _B01GroupLockIsEnabled = true;

        public String B01GroupLockTextBoxMask
        {
            get { return _B01GroupLockTextBoxMask; }
            set
            {
                if (_B01GroupLockTextBoxMask != value)
                {
                    _B01GroupLockTextBoxMask = value;
                    OnPropertyChanged("B01GroupLockTextBoxMask");
                }
            }
        }
            
        public String B01GroupLockTextBoxAction
        {
            get { return _B01GroupLockTextBoxAction; }
            set
            {
                if (_B01GroupLockTextBoxAction != value)
                {
                    _B01GroupLockTextBoxAction = value;
                    OnPropertyChanged("B01GroupLockTextBoxAction");
                }
            }
        }
        
        public Boolean B01GroupLockIsEnabled
        {
            get { return _B01GroupLockIsEnabled; }
            set
            {
                if (_B01GroupLockIsEnabled != value)
                {
                    _B01GroupLockIsEnabled = value;
                    OnPropertyChanged("B01GroupLockIsEnabled");
                }
            }
        }
        #endregion

        #region === B01 Group Kill ===
        private Boolean _B01GroupKillIsEnabled = true;
        public Boolean B01GroupKillIsEnabled
        {
            get { return _B01GroupKillIsEnabled; }
            set
            {
                if (_B01GroupKillIsEnabled != value)
                {
                    _B01GroupKillIsEnabled = value;
                    OnPropertyChanged("B01GroupKillIsEnabled");
                }
            }
        }

        private String _B01TextBoxKillPassword = String.Empty;
        public String B01TextBoxKillPassword
        {
            get { return _B01TextBoxKillPassword; }
            set
            {
                if (_B01TextBoxKillPassword != value)
                {
                    _B01TextBoxKillPassword = value;
                    OnPropertyChanged("B01TextBoxKillPassword");
                }
            }
        }
        
        #endregion

        #region === B01 Group GPIO ===
        private Boolean _B01GroupGPIOIsEnabled = true;
        private GPIOConfig _B01GroupGPIOComboBoxConfigur;
        private Boolean _B01GroupGPIOCheckBoxConfigur10IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxConfigur11IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxConfigur14IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxPin10IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxPin11IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxPin14IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxStatus10IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxStatus11IsChecked = false;
        private Boolean _B01GroupGPIOCheckBoxStatus14IsChecked = false;
        private GPIOConfig _B01GroupGPIOComboBoxPins;


        public Boolean B01GroupGPIOIsEnabled
        {
            get { return _B01GroupGPIOIsEnabled; }
            set
            {
                if (_B01GroupGPIOIsEnabled != value)
                {
                    _B01GroupGPIOIsEnabled = value;
                    OnPropertyChanged("B01GroupGPIOIsEnabled");
                }
            }
        }

        public GPIOConfig B01GroupGPIOComboBoxConfigur
        {
            get { return _B01GroupGPIOComboBoxConfigur; }
            set
            {
                if (_B01GroupGPIOComboBoxConfigur != value)
                {
                    _B01GroupGPIOComboBoxConfigur = value;
                    OnPropertyChanged("B01GroupGPIOComboBoxConfigur");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxConfigur10IsChecked
        {
            get { return _B01GroupGPIOCheckBoxConfigur10IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxConfigur10IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxConfigur10IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxConfigur10IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxConfigur11IsChecked
        {
            get { return _B01GroupGPIOCheckBoxConfigur11IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxConfigur11IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxConfigur11IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxConfigur11IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxConfigur14IsChecked
        {
            get { return _B01GroupGPIOCheckBoxConfigur14IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxConfigur14IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxConfigur14IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxConfigur14IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxPin10IsChecked
        {
            get { return _B01GroupGPIOCheckBoxPin10IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxPin10IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxPin10IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxPin10IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxPin11IsChecked
        {
            get { return _B01GroupGPIOCheckBoxPin11IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxPin11IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxPin11IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxPin11IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxPin14IsChecked
        {
            get { return _B01GroupGPIOCheckBoxPin14IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxPin14IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxPin14IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxPin14IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxStatus10IsChecked
        {
            get { return _B01GroupGPIOCheckBoxStatus10IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxStatus10IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxStatus10IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxStatus10IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxStatus11IsChecked
        {
            get { return _B01GroupGPIOCheckBoxStatus11IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxStatus11IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxStatus11IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxStatus11IsChecked");
                }
            }
        }

        public Boolean B01GroupGPIOCheckBoxStatus14IsChecked
        {
            get { return _B01GroupGPIOCheckBoxStatus14IsChecked; }
            set
            {
                if (_B01GroupGPIOCheckBoxStatus14IsChecked != value)
                {
                    _B01GroupGPIOCheckBoxStatus14IsChecked = value;
                    OnPropertyChanged("B01GroupGPIOCheckBoxStatus14IsChecked");
                }
            }
        }

        public GPIOConfig B01GroupGPIOComboBoxPins
        {
            get { return _B01GroupGPIOComboBoxPins; }
            set
            {
                if (_B01GroupGPIOComboBoxPins != value)
                {
                    _B01GroupGPIOComboBoxPins = value;
                    OnPropertyChanged("B01GroupGPIOComboBoxPins");
                }
            }
        }
        #endregion

        #region === B01 Group Message ===
        private Boolean _B01GroupMsgIsEnabled = true;
        private Boolean _B01GroupMsgPopIsEnabled = true;
        private Double _B01GroupMsgListBoxHeight = 566;//594


        public Boolean B01GroupMsgIsEnabled
        {
            get { return _B01GroupMsgIsEnabled; }
            set
            {
                if (_B01GroupMsgIsEnabled != value)
                {
                    _B01GroupMsgIsEnabled = value;
                    OnPropertyChanged("B01GroupMsgIsEnabled");
                }
            }
        }

        public Boolean B01GroupMsgPopIsEnabled
        {
            get { return _B01GroupMsgPopIsEnabled; }
            set
            {
                if (_B01GroupMsgPopIsEnabled != value)
                {
                    _B01GroupMsgPopIsEnabled = value;
                    OnPropertyChanged("B01GroupMsgPopIsEnabled");
                }
            }
        }

        public Double B01GroupMsgListBoxHeight
        {
            get { return _B01GroupMsgListBoxHeight; }
            set
            {
                if (_B01GroupMsgListBoxHeight != value)
                {
                    _B01GroupMsgListBoxHeight = value;
                    OnPropertyChanged("B01GroupMsgListBoxHeight");
                }
            }
        }
        #endregion

        #region === B01 Group Custom ===
        private String _B01GroupMsgTextBoxCustom = String.Empty;
        public String B01GroupMsgTextBoxCustom
        {
            get { return _B01GroupMsgTextBoxCustom; }
            set
            {
                if (_B01GroupMsgTextBoxCustom != value)
                {
                    _B01GroupMsgTextBoxCustom = value;
                    OnPropertyChanged("B01GroupMsgTextBoxCustom");
                }
            }
        }
        #endregion

        #region === B02 ===

        #region === B02 Group Pre-Setting ===
        private Boolean _B02GroupPreSetAccessIsEnabled = true;
        private Boolean _B02GroupPreSetSelectIsEnabled = true;
        private MemBank _B02GroupPreSetSelectMemBank;
        private String _B02GroupPreSetSelectBitAddress = String.Empty;
        private String _B02GroupPreSetSelectBitLength = String.Empty;
        private String _B02GroupPreSetSelectBitData = String.Empty;
        private String _B02GroupPreSetAccessPassword = String.Empty;
   
        private Boolean _B02GroupReadCtrlCheckBoxIsChecked = false;
        private MemBank _B02GroupPreSetReadMemBank;
        private String _B02GroupPreSetReadAddress = "2";
        private String _B02GroupPreSetReadLength = "6";
        private Boolean _B02GroupReadCtrlIsEnabled = true;
        private Boolean _B02GroupPreSetRepeatUIsEnabled = true;
        private Boolean _B02GroupPreSetRepeatQIsEnabled = true;
        private Boolean _B02GroupUSlotQCheckBoxIsChecked = false;


        public Boolean B02GroupPreSetAccessIsEnabled
        {
            get { return _B02GroupPreSetAccessIsEnabled; }
            set
            {
                if (_B02GroupPreSetAccessIsEnabled != value)
                {
                    _B02GroupPreSetAccessIsEnabled = value;
                    OnPropertyChanged("B02GroupPreSetAccessIsEnabled");
                }
            }
        }

        public Boolean B02GroupPreSetSelectIsEnabled
        {
            get { return _B02GroupPreSetSelectIsEnabled; }
            set
            {
                if (_B02GroupPreSetSelectIsEnabled != value)
                {
                    _B02GroupPreSetSelectIsEnabled = value;
                    OnPropertyChanged("B02GroupPreSetSelectIsEnabled");
                }
            }
        }

        public MemBank B02GroupPreSetSelectMemBank
        {
            get { return _B02GroupPreSetSelectMemBank; }
            set
            {
                if (_B02GroupPreSetSelectMemBank != value)
                {
                    _B02GroupPreSetSelectMemBank = value;
                    OnPropertyChanged("B02GroupPreSetSelectMemBank");
                }
            }
        }

        public String B02GroupPreSetSelectBitAddress
        {
            get { return _B02GroupPreSetSelectBitAddress; }
            set
            {
                if (_B02GroupPreSetSelectBitAddress != value)
                {
                    _B02GroupPreSetSelectBitAddress = value;
                    OnPropertyChanged("B02GroupPreSetSelectBitAddress");
                }
            }
        }

        public String B02GroupPreSetSelectBitLength
        {
            get { return _B02GroupPreSetSelectBitLength; }
            set
            {
                if (_B02GroupPreSetSelectBitLength != value)
                {
                    _B02GroupPreSetSelectBitLength = value;
                    OnPropertyChanged("B02GroupPreSetSelectBitLength");
                }
            }
        }

        public String B02GroupPreSetSelectBitData
        {
            get { return _B02GroupPreSetSelectBitData; }
            set
            {
                if (_B02GroupPreSetSelectBitData != value)
                {
                    _B02GroupPreSetSelectBitData = value;
                    OnPropertyChanged("B02GroupPreSetSelectBitData");
                }
            }
        }

        public String B02GroupPreSetAccessPassword
        {
            get { return _B02GroupPreSetAccessPassword; }
            set
            {
                if (_B02GroupPreSetAccessPassword != value)
                {
                    _B02GroupPreSetAccessPassword = value;
                    OnPropertyChanged("B02GroupPreSetAccessPassword");
                }
            }
        }        
 
        public Boolean B02GroupReadCtrlCheckBoxIsChecked
        {
            get { return _B02GroupReadCtrlCheckBoxIsChecked; }
            set
            {
                if (_B02GroupReadCtrlCheckBoxIsChecked != value)
                {
                    _B02GroupReadCtrlCheckBoxIsChecked = value;
                    OnPropertyChanged("B02GroupReadCtrlCheckBoxIsChecked");
                }
            }
        }

        public MemBank B02GroupPreSetReadMemBank
        {
            get { return _B02GroupPreSetReadMemBank; }
            set
            {
                if (_B02GroupPreSetReadMemBank != value)
                {
                    _B02GroupPreSetReadMemBank = value;
                    OnPropertyChanged("B02GroupPreSetReadMemBank");
                }
            }
        }
 
        public String B02GroupPreSetReadAddress
        {
            get { return _B02GroupPreSetReadAddress; }
            set
            {
                if (_B02GroupPreSetReadAddress != value)
                {
                    _B02GroupPreSetReadAddress = value;
                    OnPropertyChanged("B02GroupPreSetReadAddress");
                }
            }
        }

        public String B02GroupPreSetReadLength
        {
            get { return _B02GroupPreSetReadLength; }
            set
            {
                if (_B02GroupPreSetReadLength != value)
                {
                    _B02GroupPreSetReadLength = value;
                    OnPropertyChanged("B02GroupPreSetReadLength");
                }
            }
        }

        public Boolean B02GroupReadCtrlIsEnabled
        {
            get { return _B02GroupReadCtrlIsEnabled; }
            set
            {
                if (_B02GroupReadCtrlIsEnabled != value)
                {
                    _B02GroupReadCtrlIsEnabled = value;
                    OnPropertyChanged("B02GroupReadCtrlIsEnabled");
                }
            }
        }

        public Boolean B02GroupPreSetRepeatUIsEnabled
        {
            get { return _B02GroupPreSetRepeatUIsEnabled; }
            set
            {
                if (_B02GroupPreSetRepeatUIsEnabled != value)
                {
                    _B02GroupPreSetRepeatUIsEnabled = value;
                    OnPropertyChanged("B02GroupPreSetRepeatUIsEnabled");
                }
            }
        }

        public Boolean B02GroupPreSetRepeatQIsEnabled
        {
            get { return _B02GroupPreSetRepeatQIsEnabled; }
            set
            {
                if (_B02GroupPreSetRepeatQIsEnabled != value)
                {
                    _B02GroupPreSetRepeatQIsEnabled = value;
                    OnPropertyChanged("B02GroupPreSetRepeatQIsEnabled");
                }
            }
        }

        public Boolean B02GroupUSlotQCheckBoxIsChecked
        {
            get { return _B02GroupUSlotQCheckBoxIsChecked; }
            set
            {
                if (_B02GroupUSlotQCheckBoxIsChecked != value)
                {
                    _B02GroupUSlotQCheckBoxIsChecked = value;
                    OnPropertyChanged("B02GroupUSlotQCheckBoxIsChecked");
                }
            }
        }
        #endregion

        #region === B02 Group U & Q ===
        private Boolean _B02GroupUSlotQIsEnabled = true;
        //private Boolean _B02GroupUSlotQCheckBoxIsEnabled = true;
        private SlotQ _B02GroupUSlotQComboBox;
        private Boolean _B02GroupUButtonIsEnabled = true;
        private Boolean _B02GroupQButtonIsEnabled = true;

        public Boolean B02GroupUSlotQIsEnabled
        {
            get { return _B02GroupUSlotQIsEnabled; }
            set
            {
                if (_B02GroupUSlotQIsEnabled != value)
                {
                    _B02GroupUSlotQIsEnabled = value;
                    OnPropertyChanged("B02GroupUSlotQIsEnabled");
                }
            }
        }
   
        /*public Boolean B02GroupUSlotQCheckBoxIsEnabled
        {
            get { return _B02GroupUSlotQCheckBoxIsEnabled; }
            set
            {
                if (_B02GroupUSlotQCheckBoxIsEnabled != value)
                {
                    _B02GroupUSlotQCheckBoxIsEnabled = value;
                    OnPropertyChanged("B02GroupUSlotQCheckBoxIsEnabled");
                }
            }
        }*/

        public SlotQ B02GroupUSlotQComboBox
        {
            get { return _B02GroupUSlotQComboBox; }
            set
            {
                if (_B02GroupUSlotQComboBox != value)
                {
                    _B02GroupUSlotQComboBox = value;
                    OnPropertyChanged("B02GroupUSlotQComboBox");
                }
            }
        }

        public Boolean B02GroupUButtonIsEnabled
        {
            get { return _B02GroupUButtonIsEnabled; }
            set
            {
                if (_B02GroupUButtonIsEnabled != value)
                {
                    _B02GroupUButtonIsEnabled = value;
                    OnPropertyChanged("B02GroupUButtonIsEnabled");
                }
            }
        }

        public Boolean B02GroupQButtonIsEnabled
        {
            get { return _B02GroupQButtonIsEnabled; }
            set
            {
                if (_B02GroupQButtonIsEnabled != value)
                {
                    _B02GroupQButtonIsEnabled = value;
                    OnPropertyChanged("B02GroupQButtonIsEnabled");
                }
            }

        }      
        #endregion

        #region === B02 Group Record ===
        private String _B02GroupRecordTextBlockCount = String.Empty;
        private String _B02GroupRecordTextBlockRunCount = String.Empty;
        private String _B02GroupRecordTextBlockTagCount = String.Empty;
        private String _B02GroupRecordTextBlockTimeAvgCount = String.Empty;
        private Boolean _B02ButtonClearIsEnabled = true;
        private Boolean _B02ButtonSaveIsEnabled = true;

        public String B02GroupRecordTextBlockCount
        {
            get { return _B02GroupRecordTextBlockCount; }
            set
            {
                if (_B02GroupRecordTextBlockCount != value)
                {
                    _B02GroupRecordTextBlockCount = value;
                    OnPropertyChanged("B02GroupRecordTextBlockCount");
                }
            }
        }
        
        public String B02GroupRecordTextBlockRunCount
        {
            get { return _B02GroupRecordTextBlockRunCount; }
            set
            {
                if (_B02GroupRecordTextBlockRunCount != value)
                {
                    _B02GroupRecordTextBlockRunCount = value;
                    OnPropertyChanged("B02GroupRecordTextBlockRunCount");
                }
            }
        }

        public String B02GroupRecordTextBlockTagCount
        {
            get { return _B02GroupRecordTextBlockTagCount; }
            set
            {
                if (_B02GroupRecordTextBlockTagCount != value)
                {
                    _B02GroupRecordTextBlockTagCount = value;
                    OnPropertyChanged("B02GroupRecordTextBlockTagCount");
                }
            }
        }

        public String B02GroupRecordTextBlockTimeAvgCount
        {
            get { return _B02GroupRecordTextBlockTimeAvgCount; }
            set
            {
                if (_B02GroupRecordTextBlockTimeAvgCount != value)
                {
                    _B02GroupRecordTextBlockTimeAvgCount = value;
                    OnPropertyChanged("B02GroupRecordTextBlockTimeAvgCount");
                }
            }
        }

        public Boolean B02ButtonClearIsEnabled
        {
            get { return _B02ButtonClearIsEnabled; }
            set
            {
                if (_B02ButtonClearIsEnabled != value)
                {
                    _B02ButtonClearIsEnabled = value;
                    OnPropertyChanged("B02ButtonClearIsEnabled");
                }
            }
        }
    
        public Boolean B02ButtonSaveIsEnabled
        {
            get { return _B02ButtonSaveIsEnabled; }
            set
            {
                if (_B02ButtonSaveIsEnabled != value)
                {
                    _B02ButtonSaveIsEnabled = value;
                    OnPropertyChanged("B02ButtonSaveIsEnabled");
                }
            }
        }

        public void B02ListViewAddNewItem(B02ListViewItem items)
        {
            B02ListViewItemsSource.Add(items);
        }
        #endregion



        private Boolean _B02TabControlItemIsEnabled = true;
        public Boolean B02TabControlItemIsEnabled
        {
            get { return _B02TabControlItemIsEnabled; }
            set
            {
                if (_B02TabControlItemIsEnabled != value)
                {
                    _B02TabControlItemIsEnabled = value;
                    OnPropertyChanged("B02TabControlItemIsEnabled");
                }
            }
        }


        /*private Int32 _B02TabControlItemSelectedIndex = 0; // Set the field to whichever tab you want to start on
        public Int32 B02TabControlItemSelectedIndex
        {
            get { return _B02TabControlItemSelectedIndex; }
            set
            {
                if (_B02TabControlItemSelectedIndex != value)
                {
                    _B02TabControlItemSelectedIndex = value;
                    OnPropertyChanged("B02TabControlItemSelectedIndex");
                }
            }
        }*/

        private Boolean _B02ScrollViewerIsTextChanged = true;
        public Boolean B02ScrollViewerIsTextChanged
        {
            get { return _B02ScrollViewerIsTextChanged; }
            set
            {
                if (_B02ScrollViewerIsTextChanged != value)
                {
                    _B02ScrollViewerIsTextChanged = value;
                    OnPropertyChanged("B02ScrollViewerIsTextChanged");
                }
            }
        }

        private Boolean _B02Item02BtnOpenIsEnabled = true;
        public Boolean B02Item02BtnOpenIsEnabled
        {
            get { return _B02Item02BtnOpenIsEnabled; }
            set
            {
                if (_B02Item02BtnOpenIsEnabled != value)
                {
                    _B02Item02BtnOpenIsEnabled = value;
                    OnPropertyChanged("B02Item02BtnOpenIsEnabled");
                }
            }
        }

        private Boolean _B02Item02BtnSaveIsEnabled = true;
        public Boolean B02Item02BtnSaveIsEnabled
        {
            get { return _B02Item02BtnSaveIsEnabled; }
            set
            {
                if (_B02Item02BtnSaveIsEnabled != value)
                {
                    _B02Item02BtnSaveIsEnabled = value;
                    OnPropertyChanged("B02Item02BtnSaveIsEnabled");
                }
            }
        }
        
        private Boolean _B02Item02BtnIsEnabled = true;
        public Boolean B02Item02BtnIsEnabled
        {
            get { return _B02Item02BtnIsEnabled; }
            set
            {
                if (_B02Item02BtnIsEnabled != value)
                {
                    _B02Item02BtnIsEnabled = value;
                    OnPropertyChanged("B02Item02BtnIsEnabled");
                }
            }
        }

        private String _B02GroupUButton = "Multi(U)";
        public String B02GroupUButton {
            get { return _B02GroupUButton; }
            set
            {
                if (_B02GroupUButton != value)
                {
                    _B02GroupUButton = value;
                    OnPropertyChanged("B02GroupUButton");
                }
            }
        }

        private String _B02GroupQButton = "EPC(Q)";
        public String B02GroupQButton
        {
            get { return _B02GroupQButton; }
            set
            {
                if (_B02GroupQButton != value)
                {
                    _B02GroupQButton = value;
                    OnPropertyChanged("B02GroupQButton");
                }
            }
        }


        private Boolean _B02GroupMsgLogCheckBoxIsChecked = false;
        public Boolean B02GroupMsgLogCheckBoxIsChecked
        {
            get { return _B02GroupMsgLogCheckBoxIsChecked; }
            set
            {
                if (_B02GroupMsgLogCheckBoxIsChecked != value)
                {
                    _B02GroupMsgLogCheckBoxIsChecked = value;
                    OnPropertyChanged("B02GroupMsgLogCheckBoxIsChecked");
                }
            }
        }

        #endregion

        #region === #B03 ===
        private Boolean _B03IsEnabled = true;
        private Boolean _B03GroupTagWindowButtonGetIsEnabled = true;
        private Boolean _B03GroupTagWindowButtonRunIsEnabled = true;
        private Boolean _B03GroupTagWindowButtonBattAlarmTempIsEnabled = true;
        private Boolean _B03GroupTagWindowButtonBattVoltIsEnabled = true;
        private String _B03GroupTagWindowTime = "05";
        private ObservableCollection<B03ListViewItem> _B03ListViewTagWindowList;
        private B03ListViewItem _B03ListViewTagWindowSelected;

        public Boolean B03IsEnabled
        {
            get { return _B03IsEnabled; }
            set
            {
                if (_B03IsEnabled != value)
                {
                    _B03IsEnabled = value;
                    OnPropertyChanged("B03IsEnabled");
                }
            }
        }

        public Boolean B03GroupTagWindowButtonGetIsEnabled
        {
            get { return _B03GroupTagWindowButtonGetIsEnabled; }
            set
            {
                if (_B03GroupTagWindowButtonGetIsEnabled != value)
                {
                    _B03GroupTagWindowButtonGetIsEnabled = value;
                    OnPropertyChanged("B03GroupTagWindowButtonGetIsEnabled");
                }
            }
        }
        
        public Boolean B03GroupTagWindowButtonRunIsEnabled
        {
            get { return _B03GroupTagWindowButtonRunIsEnabled; }
            set
            {
                if (_B03GroupTagWindowButtonRunIsEnabled != value)
                {
                    _B03GroupTagWindowButtonRunIsEnabled = value;
                    OnPropertyChanged("B03GroupTagWindowButtonRunIsEnabled");
                }
            }
        }

        public Boolean B03GroupTagWindowButtonBattAlarmTempIsEnabled
        {
            get { return _B03GroupTagWindowButtonBattAlarmTempIsEnabled; }
            set
            {
                if (_B03GroupTagWindowButtonBattAlarmTempIsEnabled != value)
                {
                    _B03GroupTagWindowButtonBattAlarmTempIsEnabled = value;
                    OnPropertyChanged("B03GroupTagWindowButtonBattAlarmTempIsEnabled");
                }
            }
        }

        public Boolean B03GroupTagWindowButtonBattVoltIsEnabled
        {
            get { return _B03GroupTagWindowButtonBattVoltIsEnabled; }
            set
            {
                if (_B03GroupTagWindowButtonBattVoltIsEnabled != value)
                {
                    _B03GroupTagWindowButtonBattVoltIsEnabled = value;
                    OnPropertyChanged("B03GroupTagWindowButtonBattVoltIsEnabled");
                }
            }
        }


        public String B03GroupTagWindowTime
        {
            get { return _B03GroupTagWindowTime; }
            set
            {
                if (_B03GroupTagWindowTime != value)
                {
                    _B03GroupTagWindowTime = value;
                    OnPropertyChanged("B03GroupTagWindowTime");
                }
            }
        }
     
        public ObservableCollection<B03ListViewItem> B03ListViewTagWindowList
        {
            get { return _B03ListViewTagWindowList; }
            private set
            {
                if (value != _B03ListViewTagWindowList)
                {
                    _B03ListViewTagWindowList = value;
                    OnPropertyChanged("B03ListViewTagWindowList");
                }
            }
        }
       
        public B03ListViewItem B03ListViewTagWindowSelected
        {
            get { return _B03ListViewTagWindowSelected; }
            set
            {
                if (value != _B03ListViewTagWindowSelected)
                {
                    _B03ListViewTagWindowSelected = value;
                    OnPropertyChanged("B03ListViewTagWindowSelected");
                }
            }
        }

        //public CommandBase B03ListViewItemClick { get; private set; }
       
        /*private void B03ListViewItemFlashRun()
        {
            B03ListViewItem s = B03ListViewTagWindowSelected;
            //Place your custom logic here based on YourProperty
            //ActionDescription = "Clicked!!";
            //Details = "Some Details";
        }*/

        public void B03ListViewAddNewItem(B03ListViewItem items)
        {
            B03ListViewItemsSource.Add(items);
        }
        #endregion



        #region === #B04 ===
        private Boolean _B04IsEnabled = true;
        private Boolean _B04AntennaRunRepeatCheckBoxIsChecked = false;
        private Boolean _B04AntennaRawLogCheckBoxIsChecked = false;
        private Boolean _B04AntennaFragmentSummaryLogCheckBoxIsChecked = false;

        private Boolean _B04GroupAntennaCtrlIsEnabled = true;
        private Boolean _B04GroupReadCtrlIsEnabled = true;
        private Boolean _B04GroupReadCtrlCheckBoxIsChecked = false;
        private Boolean _B04GroupUSlotQIsEnabled = true;
        private Boolean _B04GroupUSlotQCheckBoxIsChecked = false;

        private MemBank _B04GroupPreSetReadMemBank;
        private String _B04GroupPreSetReadAddress = "2";
        private String _B04GroupPreSetReadLength = "6";
        private SlotQ _B04GroupUSlotQComboBox;
        private String _B04TagCount = String.Empty;
        private String _B04TagReadCount = String.Empty;
        private Int32 _B04GroupPreSetReadMemBankIndex = 0;


        private String _B04Antenna1RunTimes = String.Empty;
        private String _B04Antenna2RunTimes = String.Empty;
        private String _B04Antenna3RunTimes = String.Empty;
        private String _B04Antenna4RunTimes = String.Empty;
        private String _B04Antenna5RunTimes = String.Empty;
        private String _B04Antenna6RunTimes = String.Empty;
        private String _B04Antenna7RunTimes = String.Empty;
        private String _B04Antenna8RunTimes = String.Empty;
        private String _B04AntennaLoopTimes = String.Empty;
        private String _B04AntennaLoopDelayTime = String.Empty;

        public String B04Antenna1RunTimes
        {
            get { return _B04Antenna1RunTimes; }
            set
            {
                if (_B04Antenna1RunTimes != value)
                {
                    _B04Antenna1RunTimes = value;
                    OnPropertyChanged("B04Antenna1RunTimes");
                }
            }
        }

        public String B04Antenna2RunTimes
        {
            get { return _B04Antenna2RunTimes; }
            set
            {
                if (_B04Antenna2RunTimes != value)
                {
                    _B04Antenna2RunTimes = value;
                    OnPropertyChanged("B04Antenna2RunTimes");
                }
            }
        }

        public String B04Antenna3RunTimes
        {
            get { return _B04Antenna3RunTimes; }
            set
            {
                if (_B04Antenna3RunTimes != value)
                {
                    _B04Antenna3RunTimes = value;
                    OnPropertyChanged("B04Antenna3RunTimes");
                }
            }
        }

        public String B04Antenna4RunTimes
        {
            get { return _B04Antenna4RunTimes; }
            set
            {
                if (_B04Antenna4RunTimes != value)
                {
                    _B04Antenna4RunTimes = value;
                    OnPropertyChanged("B04Antenna4RunTimes");
                }
            }
        }

        public String B04Antenna5RunTimes
        {
            get { return _B04Antenna5RunTimes; }
            set
            {
                if (_B04Antenna5RunTimes != value)
                {
                    _B04Antenna5RunTimes = value;
                    OnPropertyChanged("B04Antenna5RunTimes");
                }
            }
        }

        public String B04Antenna6RunTimes
        {
            get { return _B04Antenna6RunTimes; }
            set
            {
                if (_B04Antenna6RunTimes != value)
                {
                    _B04Antenna6RunTimes = value;
                    OnPropertyChanged("B04Antenna6RunTimes");
                }
            }
        }

        public String B04Antenna7RunTimes
        {
            get { return _B04Antenna7RunTimes; }
            set
            {
                if (_B04Antenna7RunTimes != value)
                {
                    _B04Antenna7RunTimes = value;
                    OnPropertyChanged("B04Antenna7RunTimes");
                }
            }
        }

        public String B04Antenna8RunTimes
        {
            get { return _B04Antenna8RunTimes; }
            set
            {
                if (_B04Antenna8RunTimes != value)
                {
                    _B04Antenna8RunTimes = value;
                    OnPropertyChanged("B04Antenna8RunTimes");
                }
            }
        }

        public String B04AntennaLoopTimes
        {
            get { return _B04AntennaLoopTimes; }
            set
            {
                if (_B04AntennaLoopTimes != value)
                {
                    _B04AntennaLoopTimes = value;
                    OnPropertyChanged("B04AntennaLoopTimes");
                }
            }
        }

        public String B04AntennaLoopDelayTime
        {
            get { return _B04AntennaLoopDelayTime; }
            set
            {
                if (_B04AntennaLoopDelayTime != value)
                {
                    _B04AntennaLoopDelayTime = value;
                    OnPropertyChanged("B04AntennaLoopDelayTime");
                }
            }
        }

        public Boolean B04IsEnabled
        {
            get { return _B04IsEnabled; }
            set
            {
                if (_B04IsEnabled != value)
                {
                    _B04IsEnabled = value;
                    OnPropertyChanged("B04IsEnabled");
                }
            }
        }

        public Boolean B04AntennaRunRepeatCheckBoxIsChecked
        {
            get { return _B04AntennaRunRepeatCheckBoxIsChecked; }
            set
            {
                if (_B04AntennaRunRepeatCheckBoxIsChecked != value)
                {
                    _B04AntennaRunRepeatCheckBoxIsChecked = value;
                    OnPropertyChanged("B04AntennaRunRepeatCheckBoxIsChecked");
                }
            }
        }

        public Boolean B04AntennaRawLogCheckBoxIsChecked
        {
            get { return _B04AntennaRawLogCheckBoxIsChecked; }
            set
            {
                if (_B04AntennaRawLogCheckBoxIsChecked != value)
                {
                    _B04AntennaRawLogCheckBoxIsChecked = value;
                    OnPropertyChanged("B04AntennaRawLogCheckBoxIsChecked");
                }
            }
        }

        public Boolean B04AntennaFragmentSummaryLogCheckBoxIsChecked
        {
            get { return _B04AntennaFragmentSummaryLogCheckBoxIsChecked; }
            set
            {
                if (_B04AntennaFragmentSummaryLogCheckBoxIsChecked != value)
                {
                    _B04AntennaFragmentSummaryLogCheckBoxIsChecked = value;
                    OnPropertyChanged("B04AntennaFragmentSummaryLogCheckBoxIsChecked");
                }
            }
        }


        public void B04ListViewAddNewItem(B04ListViewItem items)
        {
            B04ListViewItemsSource.Add(items);
        }

        public Boolean B04GroupAntennaCtrlIsEnabled
        {
            get { return _B04GroupAntennaCtrlIsEnabled; }
            set
            {
                if (_B04GroupAntennaCtrlIsEnabled != value)
                {
                    _B04GroupAntennaCtrlIsEnabled = value;
                    OnPropertyChanged("B04GroupAntennaCtrlIsEnabled");
                }
            }
        }
        
        public Boolean B04GroupReadCtrlIsEnabled
        {
            get { return _B04GroupReadCtrlIsEnabled; }
            set
            {
                if (_B04GroupReadCtrlIsEnabled != value)
                {
                    _B04GroupReadCtrlIsEnabled = value;
                    OnPropertyChanged("B04GroupReadCtrlIsEnabled");
                }
            }
        }

        public Boolean B04GroupReadCtrlCheckBoxIsChecked
        {
            get { return _B04GroupReadCtrlCheckBoxIsChecked; }
            set
            {
                if (_B04GroupReadCtrlCheckBoxIsChecked != value)
                {
                    _B04GroupReadCtrlCheckBoxIsChecked = value;
                    OnPropertyChanged("B04GroupReadCtrlCheckBoxIsChecked");
                }
            }
        }

        public MemBank B04GroupPreSetReadMemBank
        {
            get { return _B04GroupPreSetReadMemBank; }
            set
            {
                if (_B04GroupPreSetReadMemBank != value)
                {
                    _B04GroupPreSetReadMemBank = value;
                    OnPropertyChanged("B04GroupPreSetReadMemBank");
                }
            }
        }

        public Int32 B04GroupPreSetReadMemBankIndex
        {
            get { return _B04GroupPreSetReadMemBankIndex; }
            set
            {
                if (_B04GroupPreSetReadMemBankIndex != value)
                {
                    _B04GroupPreSetReadMemBankIndex = value;
                    OnPropertyChanged("B04GroupPreSetReadMemBankIndex");
                }
            }
        }

        public String B04GroupPreSetReadAddress
        {
            get { return _B04GroupPreSetReadAddress; }
            set
            {
                if (_B04GroupPreSetReadAddress != value)
                {
                    _B04GroupPreSetReadAddress = value;
                    OnPropertyChanged("B04GroupPreSetReadAddress");
                }
            }
        }

        public String B04GroupPreSetReadLength
        {
            get { return _B04GroupPreSetReadLength; }
            set
            {
                if (_B04GroupPreSetReadLength != value)
                {
                    _B04GroupPreSetReadLength = value;
                    OnPropertyChanged("B04GroupPreSetReadLength");
                }
            }
        }

        public Boolean B04GroupUSlotQIsEnabled
        {
            get { return _B04GroupUSlotQIsEnabled; }
            set
            {
                if (_B04GroupUSlotQIsEnabled != value)
                {
                    _B04GroupUSlotQIsEnabled = value;
                    OnPropertyChanged("B04GroupUSlotQIsEnabled");
                }
            }
        }

        public Boolean B04GroupUSlotQCheckBoxIsChecked
        {
            get { return _B04GroupUSlotQCheckBoxIsChecked; }
            set
            {
                if (_B04GroupUSlotQCheckBoxIsChecked != value)
                {
                    _B04GroupUSlotQCheckBoxIsChecked = value;
                    OnPropertyChanged("B04GroupUSlotQCheckBoxIsChecked");
                }
            }
        }

        public SlotQ B04GroupUSlotQComboBox
        {
            get { return _B04GroupUSlotQComboBox; }
            set
            {
                if (_B04GroupUSlotQComboBox != value)
                {
                    _B04GroupUSlotQComboBox = value;
                    OnPropertyChanged("B04GroupUSlotQComboBox");
                }
            }
        }

        public String B04TagCount
        {
            get { return _B04TagCount; }
            set
            {
                if (_B04TagCount != value)
                {
                    _B04TagCount = value;
                    OnPropertyChanged("B04TagCount");
                }
            }
        }

        public String B04TagReadCount
        {
            get { return _B04TagReadCount; }
            set
            {
                if (_B04TagReadCount != value)
                {
                    _B04TagReadCount = value;
                    OnPropertyChanged("B04TagReadCount");
                }
            }
        }
        
        #endregion



        #region === Border ===
        private Boolean _BorderSettingButtonIsEnabled = true;
        private String _BorderLabelMessage = String.Empty;
        private String _BorderFirmwareVersion = String.Empty;
        private String _BorderTBReaderID = String.Empty;
        private String _BorderTextBlockStatus = String.Empty;
        private Brush _BorderLabelMessageForegroud = Brushes.Black;
        private Boolean _BorderSelectedPageIsEnabled = true;
        private Boolean _BorderComboBoxCultureIsEnabled = true;
        private Boolean _BorderCheckBoxStatusIsEnabled = true;
        private Int32 _TabControlPageSelectedIndex = 0; // Set the field to whichever tab you want to start on

        public Boolean BorderSettingButtonIsEnabled
        {
            get { return _BorderSettingButtonIsEnabled; }
            set
            {
                if (_BorderSettingButtonIsEnabled != value)
                {
                    _BorderSettingButtonIsEnabled = value;
                    OnPropertyChanged("BorderSettingButtonIsEnabled");
                }
            }
        }
       
        public String BorderLabelMessage
        {
            get { return _BorderLabelMessage; }
            set
            {
                if (_BorderLabelMessage != value)
                {
                    _BorderLabelMessage = value;
                    OnPropertyChanged("BorderLabelMessage");
                }
            }
        }

        public String BorderFirmwareVersion
        {
            get { return _BorderFirmwareVersion; }
            set
            {
                if (_BorderFirmwareVersion != value)
                {
                    _BorderFirmwareVersion = value;
                    OnPropertyChanged("BorderFirmwareVersion");
                }
            }
        }

        public String BorderTBReaderID
        {
            get { return _BorderTBReaderID; }
            set
            {
                if (_BorderTBReaderID != value)
                {
                    _BorderTBReaderID = value;
                    OnPropertyChanged("BorderTBReaderID");
                }
            }
        }

        public String BorderTextBlockStatus
        {
            get { return _BorderTextBlockStatus; }
            set
            {
                if (_BorderTextBlockStatus != value)
                {
                    _BorderTextBlockStatus = value;
                    OnPropertyChanged("BorderTextBlockStatus");
                }
            }
        }

        public Brush BorderLabelMessageForegroud
        {
            get { return _BorderLabelMessageForegroud; }
            set
            {
                if (_BorderLabelMessageForegroud != value)
                {
                    _BorderLabelMessageForegroud = value;
                    OnPropertyChanged("BorderLabelMessageForegroud");
                }
            }
        }

        public Boolean BorderSelectedPageIsEnabled
        {
            get { return _BorderSelectedPageIsEnabled; }
            set
            {
                if (_BorderSelectedPageIsEnabled != value)
                {
                    _BorderSelectedPageIsEnabled = value;
                    OnPropertyChanged("BorderSelectedPageIsEnabled");
                }
            }
        }

        public Boolean BorderComboBoxCultureIsEnabled
        {
            get { return _BorderComboBoxCultureIsEnabled; }
            set
            {
                if (_BorderComboBoxCultureIsEnabled != value)
                {
                    _BorderComboBoxCultureIsEnabled = value;
                    OnPropertyChanged("BorderComboBoxCultureIsEnabled");
                }
            }
        }

        private String _BorderCheckBoxStatusTag = "False";
        public String BorderCheckBoxStatusTag
        {
            get { return _BorderCheckBoxStatusTag; }
            set
            {
                if (_BorderCheckBoxStatusTag != value)
                {
                    _BorderCheckBoxStatusTag = value;
                    OnPropertyChanged("BorderCheckBoxStatusTag");
                }
            }
        }

        public Boolean BorderCheckBoxStatusIsEnabled
        {
            get { return _BorderCheckBoxStatusIsEnabled; }
            set
            {
                if (_BorderCheckBoxStatusIsEnabled != value)
                {
                    _BorderCheckBoxStatusIsEnabled = value;
                    OnPropertyChanged("BorderCheckBoxStatusIsEnabled");
                }
            }
        }

        public Int32 TabControlPageSelectedIndex
        {
            get { return _TabControlPageSelectedIndex; }
            set
            {
                if (_TabControlPageSelectedIndex != value)
                {
                    _TabControlPageSelectedIndex = value;
                    OnPropertyChanged("TabControlPageSelectedIndex");
                }
            }
        }
        #endregion


        


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
