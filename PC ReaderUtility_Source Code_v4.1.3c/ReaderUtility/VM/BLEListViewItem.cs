using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RFID.Utility.VM
{
    public class BLEListViewItem : INotifyPropertyChanged
    {

        private String _DeviceName;
        public String DeviceName
        {
            get { return _DeviceName; }
            set
            {
                if (_DeviceName != value)
                {
                    _DeviceName = value;
                    RaisePropertyChanged("Content");
                }
            }
        }

        private String _DeviceUUID;
        public String DeviceUUID
        {
            get { return _DeviceUUID; }
            set
            {
                if (_DeviceUUID != value)
                {
                    _DeviceUUID = value;
                    RaisePropertyChanged("DeviceUUID");
                }
            }
        }

        private String _ShowDeviceUUID;
        public String ShowDeviceUUID
        {
            get { return _ShowDeviceUUID; }
            set
            {
                if (_ShowDeviceUUID != value)
                {
                    _ShowDeviceUUID = value;
                    RaisePropertyChanged("ShowDeviceUUID");
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
