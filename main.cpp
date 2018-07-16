#include "mbed.h"
#include "SDFileSystem.h"
#include "Camera_LS_Y201.h"


#define EnDebugMSG  true //true-> print debug message to PC USB terminal, false->not print
#include "filelib.h"

#define NEWLINE()   pc.printf("\r\n")
#define DEBMSG      pc.printf


#include "MQTTEthernet.h"
#include "MQTTClient.h"
#define ECHO_SERVER_PORT   7

#include "EthernetInterface.h"
#include "FTPClient.h"
#include <string.h>
#define FTP_SERVER_PORT     21
#define FILENAME    "/sd/IMG_%04d.jpg"
#define FILENAME_FTP    "IMG_%d.jpg"

Serial pc(USBTX,USBRX);


FTPClient FTP(PB_3, PB_2, PB_1, PB_0, "sd"); // WIZwiki-W7500

Camera_LS_Y201 cam1(D1,D0); //rx tx



Timer t;

typedef struct work {
    FILE *fp;
} work_t;

/*****************MQTT fun def START******************/
MQTTEthernet ipstack = MQTTEthernet();

MQTT::Client<MQTTEthernet, Countdown> client = MQTT::Client<MQTTEthernet, Countdown>(ipstack);
MQTT::Message message;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
}

/*****************MQTT fun def END******************/

work_t work;

int take_picture = 0;
char fname[64];
char fname_FTP[64];
/**
 * Callback function for readJpegFileContent.
 *
 * @param buf A pointer to a buffer.
 * @param siz A size of the buffer.
 */
void callback_func(int done, int total, uint8_t *buf, size_t siz)
{
    fwrite(buf, siz, 1, work.fp);

    static int n = 0;
    int tmp = done * 100 / total;
    if (n != tmp) {
        n = tmp;
        DEBMSG("Writing...: %3d%%", n);
        NEWLINE();
    }
}
/**
 * Capture.
 *
 * @param cam A pointer to a camera object.
 * @param filename The file name.
 *
 * @return Return 0 if it succeed.
 */
int capture(Camera_LS_Y201 *cam, char *filename)
{
    /*
     * Take a picture.
     */
    if (cam->takePicture() != 0) {
        return -1;
    }
    DEBMSG("Captured.");
    NEWLINE();

    /*
     * Open file.
     */
    work.fp = fopen(filename, "wb");
    if (work.fp == NULL) {
        return -2;
    }

    /*
     * Read the content.
     */
    DEBMSG("%s", filename);
    NEWLINE();
    if (cam->readJpegFileContent(callback_func) != 0) {
        fclose(work.fp);
        return -3;
    }
    fclose(work.fp);

// Stop taking pictures.

    cam->stopTakingPictures();

    return 0;
}


int main()
{
    pc.baud(9600);
    

    char* copyFileName;
    char *fileName ;

    //CAMERA MODULE CAPTURE IMAGE

    DEBMSG("Camera module");
    NEWLINE();
    DEBMSG("Resetting...");
    NEWLINE();
    //lede = true;
    if (cam1.reset() == 0) {
        DEBMSG("Reset OK.");
        NEWLINE();
    } else {
        DEBMSG("Reset fail.");
        NEWLINE();
        error("Reset fail.");
        // lede = false;
    }

    if (cam1.setImageSize() == 0) {
        DEBMSG("Set image OK.");
        NEWLINE();
    } else {
        DEBMSG("Set image fail.");
        NEWLINE();
        error("Set image fail.");
        // lede = false;
    }
    wait(1);

    /*****************************FTP CLIENT CONFIGURATION START HERE AND CONNECTING TO SERVER*******************/

    pc.printf("------------------------------FTP Client Example-------------------------------------------!\r\n");


    char ftpServer_control_ip_addr[] = "172.16.73.33"; // FTP Server location
    //char* userid = "FTP"; //FTP Server User ID
    //char* pass = "user"; //FTP Server Password
    EthernetInterface eth;
    uint8_t mac_addr[6] = {0x00, 0x08, 0xdc, 0x12, 0x34, 0x45};
    char IP_Addr[] = "172.16.73.37";
    char IP_Subnet[] = "255.255.255.0";
    char IP_Gateway[] = "172.16.73.254";
    eth.init(mac_addr, IP_Addr, IP_Subnet, IP_Gateway); //Use Static
    eth.connect();
    pc.printf("\nThe IP address of the client is %s\r\n",eth.getIPAddress());
    bool n  = FTP.open("172.16.73.33", 21,"user1","user1");

    /*****************************FTP CLIENT CONFIGURATION END HERE AND CONNECTION IS ALIVE *******************/

    /*****************************MQTT CONFIGURATION START FROM HERE *****************************************/

    printf("Wait a second...\r\n");
    char* topic = "openhab/parents/command";


    char* hostname = "172.16.73.1";
    int port = 1883;

    int rc = ipstack.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\n", rc);

    printf("Topic: %s\r\n",topic);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "parents";

    if ((rc = client.connect(data)) == 0)
        printf("rc from MQTT connect is %d\n", rc);


    /********************MQTT CONFIGURATION END FROM HERE********************/

    float count;
    int nameOfimage;
    int cnt = 0;
   
    while(1) {
        
    t.start();
        /******************CAPTURE THE IMAGAE FROM CAMERA*******************/


        snprintf(fname, sizeof(fname) - 1, FILENAME, cnt );
        int r = capture(&cam1, fname);
        if (r == 0) {
            DEBMSG("[%04d]:OK.", cnt);
            NEWLINE();
        } else {
            DEBMSG("[%04d]:NG. (code=%d)", cnt, r);
            NEWLINE();
            error("Failure.");
        }


        /******************CAPTURE END*******************/

        pc.printf("\nConnecting...FTPServer\r\nIP:%s, PORT:%d\r\n", ftpServer_control_ip_addr, FTP_SERVER_PORT);
        printf("%d\r\n",n);
        wait(2);
        count = t.read();
        nameOfimage = count *1000000;
        snprintf(fname_FTP, sizeof(fname_FTP), FILENAME_FTP, nameOfimage );
   
        t.stop();
        printf("The file name is : %s \n and our filename is :%s\n", fname, fname_FTP);
        FTP.putfile(fname,fname_FTP);
        pc.printf("\r\n");
        wait(2);
        
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
      
        printf("filename over MQTT is : %s\n", fname_FTP); 
        message.payload =(void *)fname_FTP;
        message.payloadlen = strlen(fname_FTP);
        
        rc = client.publish("cdi/laxmi", message);
        pc.printf("send via MQTT\n");
       
        cnt++;
      

    }


}