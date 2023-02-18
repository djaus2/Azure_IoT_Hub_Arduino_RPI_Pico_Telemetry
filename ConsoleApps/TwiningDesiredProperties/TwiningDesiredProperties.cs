//Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-csharp-csharp-twin-getstarted

using System;
using System.Threading.Tasks;
using Microsoft.Azure.Devices;


namespace TwiningDesiredProperties
{
    /// <summary>
    /// A sample to illustrate reading Device-to-Cloud messages from a service app.
    /// </summary>
    internal class Program
    {
        static RegistryManager registryManager;


        // Connection string for your IoT Hub
        // az iot hub connection-string show --hub-name {your iot hub name} --policy-name service
        ////private readonly static string s_connectionString = "{Your service connection string here}";

        // For this sample either:
        // - pass this value as a command-prompt argument
        // - set the IOTHUB_CONN_STRING_CSHARP environment variable 
        // - create a launchSettings.json (see launchSettings.json.template) containing the variable
        private static string s_connectionString = Environment.GetEnvironmentVariable("IOTHUB_CONN_STRING_CSHARP");

        private static string s_DeviceName = Environment.GetEnvironmentVariable("DEVICE_NAME");

        public static async Task AddTagsAndQuery()
        {
            var twin = await registryManager.GetTwinAsync(s_DeviceName);
            var patch =
            @"{
                tags: {
                    location: {
                        region: 'AU',
                        plant: 'Melbourne137'
                    }
                },
               properties: {
                     desired: {
                        TelemetryFrequencyMilliseconds: 6000,
                    }
                }
            }";
            await registryManager.UpdateTwinAsync(twin.DeviceId, patch, twin.ETag);

            Thread.Sleep(5000);
            var query = registryManager.CreateQuery(
              "SELECT * FROM devices WHERE tags.location.plant = 'Melbourne137'", 100);
            var twinsMelbourne137 = await query.GetNextAsTwinAsync();
            Console.WriteLine("Devices in Redmond43: {0}",
              string.Join(", ", twinsMelbourne137.Select(t => t.DeviceId)));

            query = registryManager.CreateQuery("SELECT * FROM devices WHERE tags.location.plant = 'Melbourne137' AND properties.reported.IsRunning = false", 100);
            var twinsInRedmond43UsingCellular = await query.GetNextAsTwinAsync();
            Console.WriteLine("Devices in Redmond43 using cellular network: {0}",
              string.Join(", ", twinsInRedmond43UsingCellular.Select(t => t.DeviceId)));
        }

        public static async Task Main(string[] args)
        {
            registryManager = RegistryManager.CreateFromConnectionString(s_connectionString);
            AddTagsAndQuery().Wait();
            Console.WriteLine("Press Enter to exit.");
            Console.ReadLine();
        }
    }
}

