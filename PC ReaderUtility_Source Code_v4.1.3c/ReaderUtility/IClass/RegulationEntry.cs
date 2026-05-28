using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RFID.Utility.IClass
{
    public class RegulationEntry 
    {
        public String Area { get; set; }
       
        public String ShowName { get; set; }
        public float StartFreqMHz { get; set; }
        public float EndFreqMHz { get; set; }

        public float StartFreq2MHz { get; set; }
        public float EndFreq2MHz { get; set; }
        public float StepMHz { get; set; }
        public int Channel { get; set; }

        public RegulationEntry() { }
        public RegulationEntry(String area, String name, float startFreqMHz, float endFreqMHz, float startFreq2MHz, float endFreq2MHz, float stepMHz, int channel)
        {
            Area = area;
            ShowName = name;
            StartFreqMHz = startFreqMHz;
            EndFreqMHz = endFreqMHz;
            StartFreq2MHz = startFreq2MHz;
            EndFreq2MHz = endFreq2MHz;
            StepMHz = stepMHz;
            Channel = channel;
        }

        
    }
}
