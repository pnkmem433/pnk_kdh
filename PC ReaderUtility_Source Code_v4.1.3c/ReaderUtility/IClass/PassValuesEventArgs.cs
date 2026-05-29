using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RFID.Utility.IClass
{
    public class PassValuesEventArgs : EventArgs
    {
        private readonly Int32 index;
        private readonly B02Item02Command cmd;

        public PassValuesEventArgs(Int32 index, B02Item02Command cmd)
        {
            this.index = index;
            this.cmd = cmd;
        }
        public B02Item02Command Command { get { return this.cmd; } }
        public Int32 Index { get { return this.index; } }

    }
}
