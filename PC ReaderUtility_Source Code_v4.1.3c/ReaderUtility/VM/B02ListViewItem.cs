using System;
using System.ComponentModel;


namespace RFID.Utility.VM
{
    public class B02ListViewItem : INotifyPropertyChanged
    {
        private Int32 _length = 4;
        public Int32 Length() {
            return _length;
        }

        private String _B02PC = String.Empty;
        public String B02PC {
            get { return _B02PC; }
            set
            {
                if (_B02PC != value)
                {
                    _B02PC = value;
                    RaisePropertyChanged("B02PC");
                }
            }
        }

        private String _B02EPC = String.Empty;
        public String B02EPC {
            get { return _B02EPC; }
            set
            {
                if (_B02EPC != value)
                {
                    _B02EPC = value;
                    RaisePropertyChanged("B02EPC");
                }
            }
        }

        private String _B02CRC16 = String.Empty;
        public String B02CRC16
        {
            get { return _B02CRC16; }
            set
            {
                if (_B02CRC16 != value)
                {
                    _B02CRC16 = value;
                    RaisePropertyChanged("B02CRC16");
                }
            }
        }

        private String _B02Read = String.Empty;
        public String B02Read {
            get { return _B02Read; }
            set
            {
                if (_B02Read != value)
                {
                    _B02Read = value;
                    RaisePropertyChanged("B02Read");
                }
            }
        }

        private String _B02Count = String.Empty;
        public String B02Count {
            get { return _B02Count; }
            set
            {
                if (_B02Count != value)
                {
                    _B02Count = value;
                    RaisePropertyChanged("B02Count");
                }
            }
        }

        private String _B02Percentage = String.Empty;
        public String B02Percentage {
            get { return _B02Percentage; }
            set
            {
                if (_B02Percentage != value)
                {
                    _B02Percentage = value;
                    RaisePropertyChanged("B02Percentage");
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
