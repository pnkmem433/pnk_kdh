using System;
using System.ComponentModel;


namespace RFID.Utility.VM
{
    public class MemBank : INotifyPropertyChanged
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