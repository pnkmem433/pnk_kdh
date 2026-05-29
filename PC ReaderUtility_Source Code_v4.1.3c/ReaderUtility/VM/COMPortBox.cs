using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RFID.Utility.VM
{
    public class COMPortBox : INotifyPropertyChanged
    {
        String _Content;
        public String Content
        {
            get { return _Content; }
            set
            {
                if (_Content != value)
                {
                    _Content = value;
                    RaisePropertyChanged("Content");
                }
            }
        }

        String _Name;
        public String Name
        {
            get { return _Name; }
            set
            {
                if (_Name != value)
                {
                    _Name = value;
                    RaisePropertyChanged("Name");
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
