using System.Collections.Generic;


namespace RFID.Utility.IClass
{

	/*public class PowerItem {
		public string LocationName { get; set; }
		public string LocationValue { get; set; }
	}*/

	public static class DataRepository {
		/*public static List<PowerItem> GetPowerGroups(ReaderService.Module.Version v){
			var __list = new List<PowerItem>();
			int i;

			switch (v) {
				case ReaderService.Module.Version.FI_R300T_D1:
				case ReaderService.Module.Version.FI_R300T_D2:
                case ReaderService.Module.Version.FI_R300T_D3:
                    for (i = 27; i >= 0; i-- )
						__list.Add(new PowerItem() { LocationName = string.Format("{0} dBm", i-2), LocationValue = i.ToString("X2")});
					break;
				default:
				case ReaderService.Module.Version.FI_R300A_C1:
				case ReaderService.Module.Version.FI_R300A_C2:
                case ReaderService.Module.Version.FI_R300A_C3:
                    for (i = 20; i >= 0; i--)
						__list.Add(new PowerItem() { LocationName = string.Format("{0} dBm", i - 2), LocationValue = i.ToString("X2") });
					break;
				case ReaderService.Module.Version.FI_R300S:
					for (i = 27; i >= 0; i--)
						__list.Add(new PowerItem() { LocationName = string.Format("{0} dBm", i), LocationValue = i.ToString("X2") });
					break;
			}	
			return __list;
		}*/

		public static List<string> GetStepGroups() {
            var __list = new List<string>
            {
                "1",
                "2",
                "3",
                "4",
                "5",
                "6",
                "7",
                "8",
                "9",
                "10",
                "20",
                "50",
                "100"
            };
            return __list;
		}
	}
}
