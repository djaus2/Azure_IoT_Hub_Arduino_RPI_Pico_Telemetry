using Microsoft.Azure.Devices; // Add this through Nuget
using System.Text;

internal class Program
{
    static ServiceClient serviceClient;
    static string connectionString = "[IoT Hub Connection String]";
    static string targetDevice = "[DeviceId]";
    static void Main(string[] args)
    {
        Console.WriteLine("Send Cloud-to-Device message\n");
        serviceClient = ServiceClient.CreateFromConnectionString(connectionString);

        //[Optional] ReceiveFeedbackAsync();

        Console.WriteLine("Press any key to send a C2D message.");
        Console.ReadLine();
        SendCloudToDeviceMessageAsync().Wait();
        Console.ReadLine();
    }


    private async static Task SendCloudToDeviceMessageAsync()
    {
        var commandMessage = new
         Message(Encoding.ASCII.GetBytes("Cloud to device message."));

        //[Optional] commandMessage.Ack = DeliveryAcknowledgement.Full;

        await serviceClient.SendAsync(targetDevice, commandMessage);
    }

    //[Optional]:
    private async static void ReceiveFeedbackAsync()
    {
        var feedbackReceiver = serviceClient.GetFeedbackReceiver();

        Console.WriteLine("\nReceiving c2d feedback from service");
        while (true)
        {
            var feedbackBatch = await feedbackReceiver.ReceiveAsync();
            if (feedbackBatch == null) continue;

            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("Received feedback: {0}",
              string.Join(", ", feedbackBatch.Records.Select(f => f.StatusCode)));
            Console.ResetColor();

            await feedbackReceiver.CompleteAsync(feedbackBatch);
        }
    }
}




