using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;

namespace RFID.Utility.VM
{
    public class B03ListViewItem : INotifyPropertyChanged
    {
        private Int32 _times = 0;
        public Int32 Times
        {
            get { return _times; }
            set
            {
                if (_times != value)
                {
                    _times = value;
                    RaisePropertyChanged("Times");
                }
            }
        }

        /*private Boolean _B03ListViewTagWindowLightsStatus = false;
        public Boolean B03ListViewTagWindowLightsStatus
        {
            get { return _B03ListViewTagWindowLightsStatus; }
            set { _B03ListViewTagWindowLightsStatus = value; }
        }*/

        private String _B03ListViewTagWindowLightsTimes = String.Empty;
        public String B03ListViewTagWindowLightsTimes
        {
            get { return _B03ListViewTagWindowLightsTimes; }
            set
            {
                if (_B03ListViewTagWindowLightsTimes != value)
                {
                    _B03ListViewTagWindowLightsTimes = value;
                    RaisePropertyChanged("B03ListViewTagWindowLightsTimes");
                }
            }
        }

        private Boolean _B03ListViewTagWindowLights = false;
        public Boolean B03ListViewTagWindowLights
        {
            get { return _B03ListViewTagWindowLights; }
            set
            {
                if (_B03ListViewTagWindowLights != value)
                {
                    _B03ListViewTagWindowLights = value;
                    RaisePropertyChanged("B03ListViewTagWindowLights");
                }
            }
        }

        private String _B03ListViewTagWindowData = String.Empty;
        public String B03ListViewTagWindowData
        {
            get { return _B03ListViewTagWindowData; }
            set
            {
                if (_B03ListViewTagWindowData != value)
                {
                    _B03ListViewTagWindowData = value;
                    RaisePropertyChanged("B03ListViewTagWindowData");
                }
            }
        }

        private Boolean _B03ListViewTagWindowSelCheck = false;
        public Boolean B03ListViewTagWindowSelCheck {
            get { return _B03ListViewTagWindowSelCheck; }
            set
            {
                if (_B03ListViewTagWindowSelCheck != value)
                {
                    _B03ListViewTagWindowSelCheck = value;
                    RaisePropertyChanged("B03ListViewTagWindowSelCheck");
                }
            }
        }

        private String _B03ListViewTagWindowStatus = String.Empty;
        public String B03ListViewTagWindowStatus
        {
            get { return _B03ListViewTagWindowStatus; }
            set
            {
                if (_B03ListViewTagWindowStatus != value)
                {
                    _B03ListViewTagWindowStatus = value;
                    RaisePropertyChanged("B03ListViewTagWindowStatus");
                }
            }
        }

        private String _B03ListViewBattAlarmTemp = String.Empty;
        public String B03ListViewBattAlarmTemp
        {
            get { return _B03ListViewBattAlarmTemp; }
            set
            {
                if (_B03ListViewBattAlarmTemp != value)
                {
                    _B03ListViewBattAlarmTemp = value;
                    RaisePropertyChanged("B03ListViewBattAlarmTemp");
                }
            }
        }

        private String _B03ListViewBattVolt = String.Empty;
        public String B03ListViewBattVolt
        {
            get { return _B03ListViewBattVolt; }
            set
            {
                if (_B03ListViewBattVolt != value)
                {
                    _B03ListViewBattVolt = value;
                    RaisePropertyChanged("B03ListViewBattVolt");
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void RaisePropertyChanged(String prop)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(prop));
        }
    }
}
