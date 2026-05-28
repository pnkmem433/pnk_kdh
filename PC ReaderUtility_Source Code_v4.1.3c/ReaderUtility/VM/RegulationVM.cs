using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RFID.Utility.VM
{
    public class RegulationVM : INotifyPropertyChanged
    {
        public ObservableCollection<BaudRate> BaudRate { get; private set; }
        public ObservableCollection<String> ComboboxStepItemsSource { get; private set; }
        public ObservableCollection<String> ComboBoxAreaItemsSource { get; private set; }
        public ObservableCollection<String> ComboboxPowerItemsSource { get; private set; }
        public RegulationVM()
        {
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

            ComboboxStepItemsSource = new ObservableCollection<String>();
            ComboBoxAreaItemsSource = new ObservableCollection<String>();
            ComboboxPowerItemsSource = new ObservableCollection<String>();
        }

        private BaudRate _ComboBoxBaudRateSelectedBaudRate;
        public BaudRate ComboBoxBaudRateSelectedBaudRate
        {
            get { return _ComboBoxBaudRateSelectedBaudRate; }
            set
            {
                if (_ComboBoxBaudRateSelectedBaudRate != value)
                {
                    _ComboBoxBaudRateSelectedBaudRate = value;
                    OnPropertyChanged("ComboBoxBaudRateSelectedBaudRate");
                }
            }
        }

        private Int32 _ComboboxPowerSelectedIndex = 0;
        public Int32 ComboboxPowerSelectedIndex
        {
            get { return _ComboboxPowerSelectedIndex; }
            set
            {
                if (_ComboboxPowerSelectedIndex != value)
                {
                    _ComboboxPowerSelectedIndex = value;
                    OnPropertyChanged("ComboboxPowerSelectedIndex");
                }
            }
        }

        

        private Int32 _ComboboxStepSelectedIndex = 0;
        public Int32 ComboboxStepSelectedIndex
        {
            get { return _ComboboxStepSelectedIndex; }
            set
            {
                if (_ComboboxStepSelectedIndex != value)
                {
                    _ComboboxStepSelectedIndex = value;
                    OnPropertyChanged("ComboboxStepSelectedIndex");
                }
            }
        }

        private Int32 _ComboBoxAreaSelectedIndex = 0;
        public Int32 ComboBoxAreaSelectedIndex
        {
            get { return _ComboBoxAreaSelectedIndex; }
            set
            {
                if (_ComboBoxAreaSelectedIndex != value)
                {
                    _ComboBoxAreaSelectedIndex = value;
                    OnPropertyChanged("ComboBoxAreaSelectedIndex");
                }
            }
        }

        private String _LabelMessage = String.Empty;
        public String LabelMessage
        {
            get { return _LabelMessage; }
            set
            {
                if (_LabelMessage != value)
                {
                    _LabelMessage = value;
                    OnPropertyChanged("LabelMessage");
                }
            }
        }


        private Boolean _LoadConfigButtonIsEnabled = false;
        public Boolean LoadConfigButtonIsEnabled
        {
            get { return _LoadConfigButtonIsEnabled; }
            set
            {
                if (_LoadConfigButtonIsEnabled != value)
                {
                    _LoadConfigButtonIsEnabled = value;
                    OnPropertyChanged("LoadConfigButtonIsEnabled");
                }
            }
        }
        

        private Boolean _GroupModuleSetIsEnabled = true;
        public Boolean GroupModuleSetIsEnabled
        {
            get { return _GroupModuleSetIsEnabled; }
            set
            {
                if (_GroupModuleSetIsEnabled != value)
                {
                    _GroupModuleSetIsEnabled = value;
                    OnPropertyChanged("GroupModuleSetIsEnabled");
                }
            }
        }

        private Boolean _GroupModuleMeasureIsEnabled = true;
        public Boolean GroupModuleMeasureIsEnabled
        {
            get { return _GroupModuleMeasureIsEnabled; }
            set
            {
                if (_GroupModuleMeasureIsEnabled != value)
                {
                    _GroupModuleMeasureIsEnabled = value;
                    OnPropertyChanged("GroupModuleMeasureIsEnabled");
                }
            }
        }

        private Boolean _ButtonUpdateIsEnabled = true;
        public Boolean ButtonUpdateIsEnabled
        {
            get { return _ButtonUpdateIsEnabled; }
            set
            {
                if (_ButtonUpdateIsEnabled != value)
                {
                    _ButtonUpdateIsEnabled = value;
                    OnPropertyChanged("ButtonUpdateIsEnabled");
                }
            }
        }



        private Boolean _ButtonAdiustIsEnabled = true;
        public Boolean ButtonAdiustIsEnabled
        {
            get { return _ButtonAdiustIsEnabled; }
            set
            {
                if (_ButtonAdiustIsEnabled != value)
                {
                    _ButtonAdiustIsEnabled = value;
                    OnPropertyChanged("ButtonAdiustIsEnabled");
                }
            }
        }

        private Boolean _BasebandCarryModeIsChecked = true;
        public Boolean BasebandCarryModeIsChecked
        {
            get { return _BasebandCarryModeIsChecked; }
            set
            {
                if (_BasebandCarryModeIsChecked != value)
                {
                    _BasebandCarryModeIsChecked = value;
                    OnPropertyChanged("BasebandCarryModeIsChecked");
                }
            }
        }
        


        private Boolean _ButtonSetFrequencyPlusIsEnabled = true;
        public Boolean ButtonSetFrequencyPlusIsEnabled
        {
            get { return _ButtonSetFrequencyPlusIsEnabled; }
            set
            {
                if (_ButtonSetFrequencyPlusIsEnabled != value)
                {
                    _ButtonSetFrequencyPlusIsEnabled = value;
                    OnPropertyChanged("ButtonSetFrequencyPlusIsEnabled");
                }
            }
        }

        private Boolean _ButtonSetPowerIsEnabled = true;
        public Boolean ButtonSetPowerIsEnabled
        {
            get { return _ButtonSetPowerIsEnabled; }
            set
            {
                if (_ButtonSetPowerIsEnabled != value)
                {
                    _ButtonSetPowerIsEnabled = value;
                    OnPropertyChanged("ButtonSetPowerIsEnabled");
                }
            }
        }


        private Boolean _ComboBoxAreaIsEnabled = true;
        public Boolean ComboBoxAreaIsEnabled
        {
            get { return _ComboBoxAreaIsEnabled; }
            set
            {
                if (_ComboBoxAreaIsEnabled != value)
                {
                    _ComboBoxAreaIsEnabled = value;
                    OnPropertyChanged("ComboBoxAreaIsEnabled");
                }
            }
        }


        private Boolean _ButtonSetBuadRateIsEnabled = true;
        public Boolean ButtonSetBuadRateIsEnabled
        {
            get { return _ButtonSetBuadRateIsEnabled; }
            set
            {
                if (_ButtonSetBuadRateIsEnabled != value)
                {
                    _ButtonSetBuadRateIsEnabled = value;
                    OnPropertyChanged("ButtonSetBuadRateIsEnabled");
                }
            }
        }
        
        private Boolean _ButtonSetFrequencyResetIsEnabled = true;
        public Boolean ButtonSetFrequencyResetIsEnabled
        {
            get { return _ButtonSetFrequencyResetIsEnabled; }
            set
            {
                if (_ButtonSetFrequencyResetIsEnabled != value)
                {
                    _ButtonSetFrequencyResetIsEnabled = value;
                    OnPropertyChanged("ButtonSetFrequencyResetIsEnabled");
                }
            }
        }
        
        private Boolean _ButtonMeasureRunIsEnabled = true;
        public Boolean ButtonMeasureRunIsEnabled
        {
            get { return _ButtonMeasureRunIsEnabled; }
            set
            {
                if (_ButtonMeasureRunIsEnabled != value)
                {
                    _ButtonMeasureRunIsEnabled = value;
                    OnPropertyChanged("ButtonMeasureRunIsEnabled");
                }
            }
        }
        
        private Boolean _ButtonMeasureSetFrequencyIsEnabled = true;
        public Boolean ButtonMeasureSetFrequencyIsEnabled
        {
            get { return _ButtonMeasureSetFrequencyIsEnabled; }
            set
            {
                if (_ButtonMeasureSetFrequencyIsEnabled != value)
                {
                    _ButtonMeasureSetFrequencyIsEnabled = value;
                    OnPropertyChanged("ButtonMeasureSetFrequencyIsEnabled");
                }
            }
        }
        
        private Boolean _ButtonSetFrequencyMinusIsEnabled = true;
        public Boolean ButtonSetFrequencyMinusIsEnabled
        {
            get { return _ButtonSetFrequencyMinusIsEnabled; }
            set
            {
                if (_ButtonSetFrequencyMinusIsEnabled != value)
                {
                    _ButtonSetFrequencyMinusIsEnabled = value;
                    OnPropertyChanged("ButtonSetFrequencyMinusIsEnabled");
                }
            }
        }


        private Boolean _ButtonSetAreaIsEnabled = true;
        public Boolean ButtonSetAreaIsEnabled
        {
            get { return _ButtonSetAreaIsEnabled; }
            set
            {   
                if (_ButtonSetAreaIsEnabled != value)
                {
                    _ButtonSetAreaIsEnabled = value;
                    OnPropertyChanged("ButtonSetAreaIsEnabled");
                }
            }
        }


        private Boolean _IsButtonMeasureSetFrequencyFocused = false;
        public Boolean IsButtonMeasureSetFrequencyFocused
        {
            get { return _IsButtonMeasureSetFrequencyFocused; }
            set
            {
                if (_IsButtonMeasureSetFrequencyFocused != value)
                {
                    _IsButtonMeasureSetFrequencyFocused = value;
                    OnPropertyChanged("IsButtonMeasureSetFrequencyFocused");
                }
            }
        }

        
        private Boolean _ButtonSetFrequencyIsEnabled = true;
        public Boolean ButtonSetFrequencyIsEnabled
        {
            get { return _ButtonSetFrequencyIsEnabled; }
            set
            {
                if (_ButtonSetFrequencyIsEnabled != value)
                {
                    _ButtonSetFrequencyIsEnabled = value;
                    OnPropertyChanged("ButtonSetFrequencyIsEnabled");
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(String property)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(property));
        }
    }
}
