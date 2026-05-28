using System;
using System.ComponentModel;
using System.Windows.Media;

namespace RFID.Utility.VM
{
    public class B01ListboxItem : INotifyPropertyChanged
    {
        private Brush _HandlerColor;
        public Brush HandlerColor
        {
            get { return _HandlerColor; }
            set
            {
                if (_HandlerColor != value)
                {
                    _HandlerColor = value;
                    RaisePropertyChanged("HandlerColor");
                }
            }
        }

        private String _Handler;
        public String Handler
        {
            get { return _Handler; }
            set
            {
                if (_Handler != value)
                {
                    _Handler = value;
                    RaisePropertyChanged("Handler");
                }
            }
        }

        private Brush _ContentColor;
        public Brush ContentColor
        {
            get { return _ContentColor; }
            set
            {
                if (_ContentColor != value)
                {
                    _ContentColor = value;
                    RaisePropertyChanged("ContentColor");
                }
            }
        }

        private String _Content;
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

        public event PropertyChangedEventHandler PropertyChanged;
        private void RaisePropertyChanged(String prop)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(prop));
        }
    }
}
