/*
 _____        _     _           _  ______       _ _     _       _      _     _____ 
|_   _|      (_)   | |         | | | ___ \     (_) |   | |     | |    | |   /  __ \
  | |_      ___ ___| |_ ___  __| | | |_/ /_   _ _| | __| |___  | |    | |   | /  \/
  | \ \ /\ / / / __| __/ _ \/ _` | | ___ \ | | | | |/ _` / __| | |    | |   | |    
  | |\ V  V /| \__ \ ||  __/ (_| | | |_/ / |_| | | | (_| \__ \ | |____| |___| \__/\
  \_/ \_/\_/ |_|___/\__\___|\__,_| \____/ \__,_|_|_|\__,_|___/ \_____/\_____/\____/
                                                                                   
*/

/*
  GM Class 2 VPW network arduino based sniffer. This sniffer is based on some of the code supplied by someone named Thaniel. The base of the code was forked from
  https://github.com/LegacyNsfw/ArduinoVpw/tree/main/Mega2560. Dale Follett of Twisted Builds LLC has changed the code to
  the bare bones needed to actually get data from a GM Class 2 VPW network. This sniffer is not coded to transmit anything, it is for receiving messages only.
*/

#include "vpw_interupts.h"

void setup() {
  Serial.begin(115200); // Change this for between nano/mega/etc
  writer_init(); // Trying to figure out if this is actually required.
  reader_init(); // Setup the interupts needed to receive VPW messages
}

void loop() {
    if(packet_waiting) // if there was a message and it was recieved propperly
  {
    //Serial.println("Packet is waiting."); // This was for debugging
    packet_waiting = 0;
    if(packet_size>15) //Only print the large messages.  Small messages are printed using Print_MSG() other places
    {
     
      int size = packet_size-reader_End_Cut;
      Serial.println(); //blank line to seperate the .bin reads
      for(int x=reader_Begin_Byte;x<size;x++) //loop to output the message
      {
        byte b;
        if(x<15)b=packet_data[x];//completed message for small messages
        else    b=reader_data[x];//large data moved over to reader data so it's not overwritten by next message.
       // if(x<0||x>=size)continue; //  the first 10 are the command.  Last 3 are the check sum.  To skip use if(x<10||x>=size-3)
        if(b<=0x0F) Serial.print("0"); //if a single character add a leading zero.
        Serial.print(b,HEX); //print the data to the screen
        Serial.print(" ");
        if((x+1-reader_Begin_Byte)%16==0)Serial.println();
      }
      Serial.println();
      Serial.println();
    }
    else
    {
      memcpy(VPW_data,(const void*)packet_data,15); //move data into the VPW data for use in other parts of the program
      VPW_len = packet_size;

      //ECU_Live = 1; //flag to note ECU is sending messages
      //VPW_Message =""; //clear message

     //Store message into VPW_Message string
      for(int x=0;x<VPW_len-1;x++) //(-1) to remove CRC
      {
        if(VPW_data[x]<=0x0F) VPW_Message += "0";
      VPW_Message += String(VPW_data[x],HEX);
      }
       VPW_Message.toUpperCase();

       //Adding this to print the messages hopefully
       //Serial.println("Got to the for statement."); // This was for debugging
      for(int x=0;x<VPW_len-!Print_CRC;x++) //(-1) to remove CRC
      {
        //Serial.println("inside for statement."); // This was for debugging
        if(VPW_data[x]<=0x0F) Serial.print("0"); //if a single character add a leading zero.
        Serial.print(VPW_data[x],HEX);
       Serial.print(" ");
      }
      Serial.println(" ");
       
    }
  }
}
