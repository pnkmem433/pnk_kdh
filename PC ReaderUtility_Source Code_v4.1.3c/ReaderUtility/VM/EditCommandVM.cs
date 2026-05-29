using RFID.Utility.VM;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace RFID.Utility.VM
{
    public class EditCommandVM : INotifyPropertyChanged
    {
        public ObservableCollection<MemBank> MemBank { get; private set; }
        public ObservableCollection<SlotQ> SlotQ { get; private set; }

        public EditCommandVM() {

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

           
        }


        #region === Edit Command ===
        private Int32 _EditCommandTabControlItemSelectedIndex = 0;
        private String _EditCommandTypeIsChecked;

        /// <summary>
        /// Set the field to whichever tab you want to start on
        /// </summary>
        public Int32 EditCommandTabControlItemSelectedIndex
        {
            get { return _EditCommandTabControlItemSelectedIndex; }
            set
            {
                if (_EditCommandTabControlItemSelectedIndex != value)
                {
                    _EditCommandTabControlItemSelectedIndex = value;
                    OnPropertyChanged("EditCommandTabControlItemSelectedIndex");
                }
            }
        }
   
        /// <summary>
        /// Command type select: Standard, Customize
        /// </summary>
        public String EditCommandTypeIsChecked
        {
            get { return _EditCommandTypeIsChecked; }
            set
            {
                if (_EditCommandTypeIsChecked != value)
                {
                    _EditCommandTypeIsChecked = value;
                    OnPropertyChanged("EditCommandTypeIsChecked");
                }
            }
        }

        private Boolean _EditCommandUpdate;
        public Boolean EditCommandUpdate
        {
            get { return _EditCommandUpdate; }
            set
            {
                if (_EditCommandUpdate != value)
                {
                    _EditCommandUpdate = value;
                    OnPropertyChanged("EditCommandUpdate");
                }
            }
        }

        private Boolean _EditCommandFirst;
        public Boolean EditCommandFirst
        {
            get { return _EditCommandFirst; }
            set
            {
                if (_EditCommandFirst != value)
                {
                    _EditCommandFirst = value;
                    OnPropertyChanged("EditCommandFirst");
                }
            }
        }

        private Boolean _EditCommandFirstCustom;
        public Boolean EditCommandFirstCustom
        {
            get { return _EditCommandFirstCustom; }
            set
            {
                if (_EditCommandFirstCustom != value)
                {
                    _EditCommandFirstCustom = value;
                    OnPropertyChanged("EditCommandFirstCustom");
                }
            }
        }

        #region Tab_Read
        private MemBank _EditCommandTabReadMemBank;
        public MemBank EditCommandTabReadMemBank
        {
            get { return _EditCommandTabReadMemBank; }
            set
            {
                if (_EditCommandTabReadMemBank != value)
                {
                    _EditCommandTabReadMemBank = value;
                    OnPropertyChanged("EditCommandTabReadMemBank");
                }
            }
        }

        private Int32 _EditCommandTabReadMemBankIndex;
        public Int32 EditCommandTabReadMemBankIndex
        {
            get { return _EditCommandTabReadMemBankIndex; }
            set
            {
                if (_EditCommandTabReadMemBankIndex != value)
                {
                    _EditCommandTabReadMemBankIndex = value;
                    OnPropertyChanged("EditCommandTabReadMemBankIndex");
                }
            }
        }

        private String _EditCommandTabReadAddress = String.Empty;
        public String EditCommandTabReadAddress
        {
            get { return _EditCommandTabReadAddress; }
            set
            {
                if (_EditCommandTabReadAddress != value)
                {
                    _EditCommandTabReadAddress = value;
                    OnPropertyChanged("EditCommandTabReadAddress");
                }
            }
        }


        private String _EditCommandTabReadLength = String.Empty;
        public String EditCommandTabReadLength
        {
            get { return _EditCommandTabReadLength; }
            set
            {
                if (_EditCommandTabReadLength != value)
                {
                    _EditCommandTabReadLength = value;
                    OnPropertyChanged("EditCommandTabReadLength");
                }
            }
        }
        #endregion


        #region Tab_Write
        private MemBank _EditCommandTabWriteMemBank;
        public MemBank EditCommandTabWriteMemBank
        {
            get { return _EditCommandTabWriteMemBank; }
            set
            {
                if (_EditCommandTabWriteMemBank != value)
                {
                    _EditCommandTabWriteMemBank = value;
                    OnPropertyChanged("EditCommandTabWriteMemBank");
                }
            }
        }

        private Int32 _EditCommandTabWriteMemBankIndex;
        public Int32 EditCommandTabWriteMemBankIndex
        {
            get { return _EditCommandTabWriteMemBankIndex; }
            set
            {
                if (_EditCommandTabWriteMemBankIndex != value)
                {
                    _EditCommandTabWriteMemBankIndex = value;
                    OnPropertyChanged("EditCommandTabWriteMemBankIndex");
                }
            }
        }

        private String _EditCommandTabWriteAddress = String.Empty;
        public String EditCommandTabWriteAddress
        {
            get { return _EditCommandTabWriteAddress; }
            set
            {
                if (_EditCommandTabWriteAddress != value)
                {
                    _EditCommandTabWriteAddress = value;
                    OnPropertyChanged("EditCommandTabWriteAddress");
                }
            }
        }

        private String _EditCommandTabWriteAddressTemp = String.Empty;
        public String EditCommandTabWriteAddressTemp
        {
            get { return _EditCommandTabWriteAddressTemp; }
            set
            {
                if (_EditCommandTabWriteAddressTemp != value)
                {
                    _EditCommandTabWriteAddressTemp = value;
                    OnPropertyChanged("EditCommandTabWriteAddressTemp");
                }
            }
        }

        private String _EditCommandTabWriteLength = String.Empty;
        public String EditCommandTabWriteLength
        {
            get { return _EditCommandTabWriteLength; }
            set
            {
                if (_EditCommandTabWriteLength != value)
                {
                    _EditCommandTabWriteLength = value;
                    OnPropertyChanged("EditCommandTabWriteLength");
                }
            }
        }

        private String _EditCommandTabWriteLengthTemp = String.Empty;
        public String EditCommandTabWriteLengthTemp
        {
            get { return _EditCommandTabWriteLengthTemp; }
            set
            {
                if (_EditCommandTabWriteLengthTemp != value)
                {
                    _EditCommandTabWriteLengthTemp = value;
                    OnPropertyChanged("EditCommandTabWriteLengthTemp");
                }
            }
        }

        private String _EditCommandTabWriteData = String.Empty;
        public String EditCommandTabWriteData
        {
            get { return _EditCommandTabWriteData; }
            set
            {
                if (_EditCommandTabWriteData != value)
                {
                    _EditCommandTabWriteData = value;
                    OnPropertyChanged("EditCommandTabWriteData");
                }
            }
        }

        private String _EditCommandTabWriteDataTemp = String.Empty;
        public String EditCommandTabWriteDataTemp
        {
            get { return _EditCommandTabWriteDataTemp; }
            set
            {
                if (_EditCommandTabWriteDataTemp != value)
                {
                    _EditCommandTabWriteDataTemp = value;
                    OnPropertyChanged("EditCommandTabWriteDataTemp");
                }
            }
        }

        private String _EditCommandTabWriteDataIdx;
        public String EditCommandTabWriteDataIdx
        {
            get { return _EditCommandTabWriteDataIdx; }
            set
            {
                if (_EditCommandTabWriteDataIdx != value)
                {
                    _EditCommandTabWriteDataIdx = value;
                    OnPropertyChanged("EditCommandTabWriteDataIdx");
                }
            }
        }
        #endregion


        #region Tab_Access
        private String _EditCommandTabAccessPassword = String.Empty;
        public String EditCommandTabAccessPassword
        {
            get { return _EditCommandTabAccessPassword; }
            set
            {
                if (_EditCommandTabAccessPassword != value)
                {
                    _EditCommandTabAccessPassword = value;
                    OnPropertyChanged("EditCommandTabAccessPassword");
                }
            }
        }

        #endregion


        #region Tab_UQR

        private String _EditCommandTabUQRIsChecked;
        public String EditCommandTabUQRIsChecked
        {
            get { return _EditCommandTabUQRIsChecked; }
            set
            {
                if (_EditCommandTabUQRIsChecked != value)
                {
                    _EditCommandTabUQRIsChecked = value;
                    OnPropertyChanged("EditCommandTabUQRIsChecked");
                }
            }
        }
    
        private MemBank _EditCommandTabUQRMemBank;
        public MemBank EditCommandTabUQRMemBank
        {
            get { return _EditCommandTabUQRMemBank; }
            set
            {
                if (_EditCommandTabUQRMemBank != value)
                {
                    _EditCommandTabUQRMemBank = value;
                    OnPropertyChanged("EditCommandTabUQRMemBank");
                }
            }
        }

        private Int32 _EditCommandTabUQRMemBankIndex;
        public Int32 EditCommandTabUQRMemBankIndex
        {
            get { return _EditCommandTabUQRMemBankIndex; }
            set
            {
                if (_EditCommandTabUQRMemBankIndex != value)
                {
                    _EditCommandTabUQRMemBankIndex = value;
                    OnPropertyChanged("EditCommandTabUQRMemBankIndex");
                }
            }
        }

        private String _EditCommandTabUQRAddress = String.Empty;
        public String EditCommandTabUQRAddress
        {
            get { return _EditCommandTabUQRAddress; }
            set
            {
                if (_EditCommandTabUQRAddress != value)
                {
                    _EditCommandTabUQRAddress = value;
                    OnPropertyChanged("EditCommandTabUQRAddress");
                }
            }
        }

        private String _EditCommandTabUQRLength = String.Empty;
        public String EditCommandTabUQRLength
        {
            get { return _EditCommandTabUQRLength; }
            set
            {
                if (_EditCommandTabUQRLength != value)
                {
                    _EditCommandTabUQRLength = value;
                    OnPropertyChanged("EditCommandTabUQRLength");
                }
            }
        }

        private Boolean _EditCommandTabUQRReadIsCheck = false;
        public Boolean EditCommandTabUQRReadIsCheck
        {
            get { return _EditCommandTabUQRReadIsCheck; }
            set
            {
                if (_EditCommandTabUQRReadIsCheck != value)
                {
                    _EditCommandTabUQRReadIsCheck = value;
                    OnPropertyChanged("EditCommandTabUQRReadIsCheck");
                }
            }
        }
        
        private Boolean _EditCommandTabUQRSlotQIsCheck = false;
        public Boolean EditCommandTabUQRSlotQIsCheck
        {
            get { return _EditCommandTabUQRSlotQIsCheck; }
            set
            {
                if (_EditCommandTabUQRSlotQIsCheck != value)
                {
                    _EditCommandTabUQRSlotQIsCheck = value;
                    OnPropertyChanged("EditCommandTabUQRSlotQIsCheck");
                }
            }
        }

        private SlotQ _EditCommandTabUQRSlotQComboBox;
        public SlotQ EditCommandTabUQRSlotQComboBox
        {
            get { return _EditCommandTabUQRSlotQComboBox; }
            set
            {
                if (_EditCommandTabUQRSlotQComboBox != value)
                {
                    _EditCommandTabUQRSlotQComboBox = value;
                    OnPropertyChanged("EditCommandTabUQRSlotQComboBox");
                }
            }
        }

        private Visibility _EditCommandTabUQRSlotComboBoxVisibility = Visibility.Visible;
        public Visibility EditCommandTabUQRSlotComboBoxVisibility
        {
            get { return _EditCommandTabUQRSlotComboBoxVisibility; }
            set
            {
                if (_EditCommandTabUQRSlotComboBoxVisibility != value)
                {
                    _EditCommandTabUQRSlotComboBoxVisibility = value;
                    OnPropertyChanged("EditCommandTabUQRSlotComboBoxVisibility");
                }
            }
        }

        private Visibility _EditCommandTabUQRSlotTitlexVisibility = Visibility.Visible;
        public Visibility EditCommandTabUQRSlotTitlexVisibility
        {
            get { return _EditCommandTabUQRSlotTitlexVisibility; }
            set
            {
                if (_EditCommandTabUQRSlotTitlexVisibility != value)
                {
                    _EditCommandTabUQRSlotTitlexVisibility = value;
                    OnPropertyChanged("EditCommandTabUQRSlotTitlexVisibility");
                }
            }
        }
        #endregion

        #region Tab_Select
        private MemBank _EditCommandTabSelectMemBank;
        public MemBank EditCommandTabSelectMemBank
        {
            get { return _EditCommandTabSelectMemBank; }
            set
            {
                if (_EditCommandTabSelectMemBank != value)
                {
                    _EditCommandTabSelectMemBank = value;
                    OnPropertyChanged("EditCommandTabSelectMemBank");
                }
            }
        }

        private Int32 _EditCommandTabSelectMemBankIndex;
        public Int32 EditCommandTabSelectMemBankIndex
        {
            get { return _EditCommandTabSelectMemBankIndex; }
            set
            {
                if (_EditCommandTabSelectMemBankIndex != value)
                {
                    _EditCommandTabSelectMemBankIndex = value;
                    OnPropertyChanged("EditCommandTabSelectMemBankIndex");
                }
            }
        }

        private String _EditCommandTabSelectAddress = String.Empty;
        public String EditCommandTabSelectAddress
        {
            get { return _EditCommandTabSelectAddress; }
            set
            {
                if (_EditCommandTabSelectAddress != value)
                {
                    _EditCommandTabSelectAddress = value;
                    OnPropertyChanged("EditCommandTabSelectAddress");
                }
            }
        }

        private String _EditCommandTabSelectLength = String.Empty;
        public String EditCommandTabSelectLength
        {
            get { return _EditCommandTabSelectLength; }
            set
            {
                if (_EditCommandTabSelectLength != value)
                {
                    _EditCommandTabSelectLength = value;
                    OnPropertyChanged("EditCommandTabSelectLength");
                }
            }
        }

        private String _EditCommandTabSelectDataIdx;
        public String EditCommandTabSelectDataIdx
        {
            get { return _EditCommandTabSelectDataIdx; }
            set
            {
                if (_EditCommandTabSelectDataIdx != value)
                {
                    _EditCommandTabSelectDataIdx = value;
                    OnPropertyChanged("EditCommandTabSelectDataIdx");
                }
            }
        }

        private String _EditCommandTabSelectData = String.Empty;
        public String EditCommandTabSelectData
        {
            get { return _EditCommandTabSelectData; }
            set
            {
                if (_EditCommandTabSelectData != value)
                {
                    _EditCommandTabSelectData = value;
                    OnPropertyChanged("EditCommandTabSelectData");
                }
            }
        }

        #endregion

        #region Tab_Lock

        private String _EditCommandTabLockMask = String.Empty;
        public String EditCommandTabLockMask
        {
            get { return _EditCommandTabLockMask; }
            set
            {
                if (_EditCommandTabLockMask != value)
                {
                    _EditCommandTabLockMask = value;
                    OnPropertyChanged("EditCommandTabLockMask");
                }
            }
        }

        private String _EditCommandTabLockAction = String.Empty;
        public String EditCommandTabLockAction
        {
            get { return _EditCommandTabLockAction; }
            set
            {
                if (_EditCommandTabLockAction != value)
                {
                    _EditCommandTabLockAction = value;
                    OnPropertyChanged("EditCommandTabLockAction");
                }
            }
        }
        #endregion

        private String _DefineSequence = String.Empty;
        public String DefineSequence
        {
            get { return _DefineSequence; }
            set
            {
                if (_DefineSequence != value)
                {
                    _DefineSequence = value;
                    OnPropertyChanged("DefineSequence");
                }
            }
        }

        private Boolean _DefineSequenceIsEnabled = false;
        public Boolean DefineSequenceIsEnabled
        {
            get { return _DefineSequenceIsEnabled; }
            set
            {
                if (_DefineSequenceIsEnabled != value)
                {
                    _DefineSequenceIsEnabled = value;
                    OnPropertyChanged("DefineSequenceIsEnabled");
                }
            }
        }
        

        private String _DefineName = String.Empty;
        public String DefineName
        {
            get { return _DefineName; }
            set
            {
                if (_DefineName != value)
                {
                    _DefineName = value;
                    OnPropertyChanged("DefineName");
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
