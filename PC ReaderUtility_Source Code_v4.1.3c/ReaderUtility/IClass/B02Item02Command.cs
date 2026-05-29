using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static RFID.Utility.MainWindow;

namespace RFID.Utility.IClass
{
    public class B02Item02Command
    {
        public bool Check { get; set; }
        /// <summary>
        /// Command type select: Standard = false; Customize = true
        /// </summary>
        public bool Type { get; set; }
        /// <summary>
        /// Command type temp select: Standard = false; Customize = true
        /// </summary>
        public bool TypeTemp { get; set; }
        public String Name { get; set; }
        public String Command { get; set; }
        public String CommandTemp { get; set; }
        public int TabIndex { get; set; }
        public int TabIndexTemp { get; set; }
        public CommandStatus CommandState { get; set; }
        public String DefineSequenceStandardTemp { get; set; }
        public String DefineSequenceCustomizeTemp { get; set; }
        //public bool OKButton { get; set; }
        public bool ApplyButton { get; set; }

        public B02Item02Command()
        {
            Check = true;
            Type = false;
            TypeTemp = false;
            //OKButton = false;
            ApplyButton = false;
            Command = String.Empty;
            CommandTemp = String.Empty;
            TabIndexTemp = 0;
            DefineSequenceStandardTemp = String.Empty;
            DefineSequenceCustomizeTemp = String.Empty;
        }

        public B02Item02Command(bool check, bool type, String name, CommandStatus commandState, String command, int idx
            , bool apply)
        {
            Check = check;
            Type = type;
            Name = name;
            CommandState = commandState;
            Command = command;
            TypeTemp = Type;
            CommandTemp = Command;
            TabIndex = idx;
            TabIndexTemp = TabIndex;
            //OKButton = ok;
            ApplyButton = apply;
            DefineSequenceStandardTemp = String.Empty;
            DefineSequenceCustomizeTemp = String.Empty;
        }
    }

  
}
