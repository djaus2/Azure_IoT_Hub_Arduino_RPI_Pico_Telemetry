// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This application uses the Azure Event Hubs Client for .NET
// For samples see: https://github.com/Azure/azure-sdk-for-net/blob/main/sdk/eventhub/Azure.Messaging.EventHubs/samples/README.md
// For documentation see: https://docs.microsoft.com/azure/event-hubs/

using System;
using System.Threading.Tasks;
using Microsoft.Azure.Devices;


namespace InvokeDirectMethod
{
    /// <summary>
    /// A sample to illustrate reading Device-to-Cloud messages from a service app.
    /// </summary>
    internal class Program
    {

        private static ServiceClient s_serviceClient;

        // Connection string for your IoT Hub
        // az iot hub connection-string show --hub-name {your iot hub name} --policy-name service
        ////private readonly static string s_connectionString = "{Your service connection string here}";

        // For this sample either:
        // - pass this value as a command-prompt argument
        // - set the IOTHUB_CONN_STRING_CSHARP environment variable 
        // - create a launchSettings.json (see launchSettings.json.template) containing the variable
        private static string s_connectionString = Environment.GetEnvironmentVariable("IOTHUB_CONN_STRING_CSHARP");

        private static string s_DeviceName = Environment.GetEnvironmentVariable("DEVICE_NAME");



        // Invoke the direct method on the device, passing the payload
        private static async Task InvokeMethod(int period)
        {
            Console.WriteLine(period);
            var methodInvocation = new CloudToDeviceMethod("frequency") { ResponseTimeout = TimeSpan.FromSeconds(30) };
            methodInvocation.SetPayloadJson(period.ToString());

            // Invoke the direct method asynchronously and get the response from the simulated device.
            var response = await s_serviceClient.InvokeDeviceMethodAsync(s_DeviceName, methodInvocation);

            Console.WriteLine("Response status: {0}, payload:", response.Status);
            Console.WriteLine(response.GetPayloadAsJson());
        }

        public static async Task Main(string[] args)
        {
            // Create a ServiceClient to communicate with service-facing endpoint on your hub.
            s_serviceClient = ServiceClient.CreateFromConnectionString(s_connectionString);




            Console.WriteLine("IoT Hub Quickstarts #2 - Back-end application.\n");
            Console.WriteLine("Using Env Var IOTHUB_CONN_STRING_CSHARP = " + s_connectionString);
            Console.WriteLine("Using Env Var DEVICE_NAME (N.b.Same as DEVICE_ID) = " + s_DeviceName);
            Console.WriteLine("Press Enter to continue when the Simulated-Device-2 is sending messages.");
            Console.ReadLine();
            // Task.Delay(3000).Wait();

            InvokeMethod(0).GetAwaiter().GetResult();
            Console.WriteLine("Period is now 0 which means it is stopped");

            Console.WriteLine("1/4 Press Enter to change period to (10s)");
            Console.ReadLine();
            InvokeMethod(10).GetAwaiter().GetResult();


            Console.WriteLine("2/4 Press Enter to change period again(15s)");
            Console.ReadLine();
            InvokeMethod(15).GetAwaiter().GetResult();

            Console.WriteLine("3/4 Press Enter to change period again (5s)");
            Console.ReadLine();
            InvokeMethod(5).GetAwaiter().GetResult();

            Console.WriteLine("4/4 Press Enter to change period again (2s)");
            Console.ReadLine();
            InvokeMethod(2).GetAwaiter().GetResult();

            Console.WriteLine("Done: Press Enter signal device to close.");
            Console.ReadLine();
            InvokeMethod(0).GetAwaiter().GetResult();

            Console.WriteLine("Done: Press Enter to exit.");
            Console.ReadLine();
        }
    }
}

