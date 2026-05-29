using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;

namespace RFID.Utility.VM
{
    public class GPIOConfig : INotifyPropertyChanged
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

        String _Tag;
        public String Tag
        {
            get { return _Tag; }
            set
            {
                if (_Tag != value)
                {
                    _Tag = value;
                    RaisePropertyChanged("Tag");
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
