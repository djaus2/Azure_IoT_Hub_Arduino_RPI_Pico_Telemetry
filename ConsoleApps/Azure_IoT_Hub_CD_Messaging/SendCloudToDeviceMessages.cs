// Ref: https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-csharp-csharp-c2d

using System;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.Devices;
using Microsoft.Rest;


namespace InvokeDirectMethod
{
    /// <summary>
    /// A sample to illustrate reading Device-to-Cloud messages from a service app.
    /// </summary>
    internal class Program
    {
        private static DeliveryAcknowledgement Ack = DeliveryAcknowledgement.Full;  // Can be None(Default),Full,PostiveOnly,NegativeOnly
        private static ServiceClient s_serviceClient;
        private static string DeviceGenerationId = "";

        // Connection string for your IoT Hub
        // az iot hub connection-string show --hub-name {your iot hub name} --policy-name service
        ////private readonly static string s_connectionString = "{Your service connection string here}";

        // For this sample either:
        // - pass this value as a command-prompt argument
        // - set the IOTHUB_CONN_STRING_CSHARP environment variable 
        // - create a launchSettings.json (see launchSettings.json.template) containing the variable
        private static string s_connectionString = Environment.GetEnvironmentVariable("IOTHUB_CONN_STRING_CSHARP");

        private static string s_DeviceName = Environment.GetEnvironmentVariable("DEVICE_NAME");



        private async static Task SendCloudToDeviceMessageAsync(string msg, bool IsJson)
        {
            var commandMessage = new
                Message(Encoding.ASCII.GetBytes(msg));
            commandMessage.ContentEncoding = "utf-8";
            if (IsJson)
            {
                commandMessage.ContentType = "application/json";
            }
            commandMessage.Ack = Ack; // Can be None(Default),Full,PostiveOnly,NegativeOnly
            //commandMessage.ExpiryTimeUtc = DateTime.UtcNow.AddSeconds(10)
            
            commandMessage.MessageId = Guid.NewGuid().ToString();
            await s_serviceClient.SendAsync(s_DeviceName, commandMessage);

            Console.WriteLine($"Message Sent: {msg}");
            if (Ack != DeliveryAcknowledgement.None)
            {
                Console.WriteLine("Wait for feedback");
                Console.ReadLine();
            }
            else
            {
                Console.WriteLine("Press any key to continue.");
                Console.ReadLine();
            }
        }

        private async static void ReceiveFeedbackAsync()
        {
            var feedbackReceiver = s_serviceClient.GetFeedbackReceiver();

            Console.WriteLine("\nReceiving c2d feedback from service");
            while (true)
            {
                var feedbackBatch = await feedbackReceiver.ReceiveAsync(CancellationToken.None);
                if (feedbackBatch == null) continue;

                Console.ForegroundColor = ConsoleColor.Yellow;

                Console.WriteLine("Received feedback - StatusCode: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.StatusCode)));
                Console.WriteLine("Received feedback - DeviceId: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.DeviceId)));
                Console.WriteLine("Received feedback - DeviceGenerationId: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.DeviceGenerationId)));
                Console.WriteLine("Received feedback - EnqueuedTimeUtc: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.EnqueuedTimeUtc)));
                Console.WriteLine("Received feedback - Description: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.Description)));
                Console.WriteLine("Received feedback - OriginalMessageId: {0}",
                    string.Join(", ", feedbackBatch.Records.Select(f => f.OriginalMessageId)));
                await feedbackReceiver.CompleteAsync(feedbackBatch, CancellationToken.None);

                Console.ResetColor();

                DeviceGenerationId = feedbackBatch.Records.First().DeviceGenerationId;

                Console.WriteLine("Press any key to continue.");
            }
        }

        public static async Task SendCDMessage(string msg, bool IsJosn)
        {
            Console.WriteLine("Send Cloud-to-Device message\n");

            s_serviceClient = ServiceClient.CreateFromConnectionString(s_connectionString);

            if (Ack != DeliveryAcknowledgement.None)  // Can be None(Default),Full,PostiveOnly,NegativeOnly
                ReceiveFeedbackAsync();
            await SendCloudToDeviceMessageAsync(msg, IsJosn);
        }

        public static async Task Main(string[] args)
        {
            Console.WriteLine("Send Cloud-to-Device messages\n");

            s_serviceClient = ServiceClient.CreateFromConnectionString(s_connectionString);

            if (Ack != DeliveryAcknowledgement.None)  // Can be None(Default),Full,PostiveOnly,NegativeOnly
                ReceiveFeedbackAsync();

            Console.WriteLine("Press any key to send a C2D message.");
            Console.ReadLine();

            SendCDMessage("This is a Cloud to device message.", false).Wait();
            Console.WriteLine("Press any key to send a second (Json) C2D message.");
            Console.ReadLine();

            SendCDMessage("{\"Msg\":\"Some Json.\"}", true).Wait();
            Console.WriteLine("Press any key to finish.");
            Console.ReadLine();
        }
    }
}

