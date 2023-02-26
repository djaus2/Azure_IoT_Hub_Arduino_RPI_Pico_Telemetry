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
            
            var patchOff =
            @"{
                tags: {
                    location: {
                        region: 'AU',
                        plant: 'Melb'
                    }
                },
               properties: {
                     desired: {
                        IsRunning : false,
                        TelemetryFrequencyMilliseconds: 6000
                    }
                }
            }";
            var patchOn =
            @"{
               properties: {
                     desired: {
                        IsRunning : true,
                        TelemetryFrequencyMilliseconds: 3000
                    }
                }
            }";

            for (int i = 0; i < 2; i++)
            {
                if (i == 0)
                {
                    //Off
                    var twin = await registryManager.GetTwinAsync(s_DeviceName);
                    await registryManager.UpdateTwinAsync(twin.DeviceId, patchOff, twin.ETag);
                    Console.WriteLine("Telemetry Off");
                }
                else
                {
                    //On
                    var twin = await registryManager.GetTwinAsync(s_DeviceName);
                    await registryManager.UpdateTwinAsync(twin.DeviceId, patchOn, twin.ETag);
                    Console.WriteLine("Telemetry On");
                }
                Thread.Sleep(5000);
                var query = registryManager.CreateQuery(
                  "SELECT * FROM devices WHERE tags.location.plant = 'Melb'", 100);
                var twinsMelb = await query.GetNextAsTwinAsync();
                Console.WriteLine("Devices in Melb: {0}",
                  string.Join(", ", twinsMelb.Select(t => t.DeviceId)));

                {
                    query = registryManager.CreateQuery("SELECT * FROM devices WHERE tags.location.plant = 'Melb' AND properties.reported.IsRunning = false", 100);
                    var twinsInMelbourneRunningNotRunningTelemetry = await query.GetNextAsTwinAsync();
                    Console.WriteLine("Devices in Melb and not running Telemetry: {0}",
                      string.Join(", ", twinsInMelbourneRunningNotRunningTelemetry.Select(t => t.DeviceId)));
                }
                {
                    query = registryManager.CreateQuery("SELECT * FROM devices WHERE tags.location.plant = 'Melb' AND properties.reported.IsRunning = true", 100);
                    var twinsInMelbourneRunningRunningTelemetry = await query.GetNextAsTwinAsync();
                    Console.WriteLine("Devices in Melb and  running Telemetry: {0}",
                      string.Join(", ", twinsInMelbourneRunningRunningTelemetry.Select(t => t.DeviceId)));
                }
            }
        }

        public static async Task Main(string[] args)
        {
            registryManager = RegistryManager.CreateFromConnectionString(s_connectionString);
            await AddTagsAndQuery();
            Console.WriteLine("Press Enter to exit.");
            Console.ReadLine();
        }
    }
}

