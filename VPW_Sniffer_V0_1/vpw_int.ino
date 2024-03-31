void reader_init() {
  // For reader:
  // PC5 input, pull-up
  bit_RX = digitalPinToBitMask(input_pin);
  port_RX = digitalPinToPort(input_pin);
  if (port_RX == NOT_A_PIN)
 { Serial.println("BAD input PIN");
 return;
 }
 //Serial.println("Hi");
direction_RX = portModeRegister(port_RX);
 out_RX = portOutputRegister(port_RX);
  in_RX = portInputRegister(port_RX);
  *out_RX |= bit_RX;  
  // enable pin change interrupt PCINT#
  if(digitalPinToPCICR(input_pin)==0)
  {
    Serial.println("not interupt pin");
    return;
  }
  *digitalPinToPCICR(input_pin) |= (1<<digitalPinToPCICRbit(input_pin));
  *digitalPinToPCMSK(input_pin) |= (1<<digitalPinToPCMSKbit(input_pin));

  
  reader_pin_last = *in_RX & bit_RX; // read pin state


  // Timer1 setup CLK/64
  OCR1A = 0x61A7; // will campare value of OCR1A against timer 1 for timeout
  TCCR1B = (1<<CS11) | (1<<CS10); //CLK/64
  TCCR1A =0;
    TCCR1C =0;
  // Timer1 enable Output Compare 1 match interrupt
  TIMSK1 |= (1<<OCIE1A); // if OCR1A matches timer interupt will happen
  // notes:
  //	100,000us = 0x61A7 (CTC timer max)
  //	     64us = 15 (small bit period)
  //	    128us = 29 (large bit period)
  //	    200us = 46 (Start Of Frame period)
  //	    768us = 177 (Break period)
}

void writer_init() {
   bit_TX = digitalPinToBitMask(output_pin);
  port_TX = digitalPinToPort(output_pin);
  if (port_TX == NOT_A_PIN)
 { Serial.println("BAD output PIN");
 return;
 }
direction_TX = portModeRegister(port_TX);
 out_TX = portOutputRegister(port_TX);
  // outputPIN output
  *direction_TX |= bit_TX; // pin  to output

  // Timer3 setup CLK/8, CTC on OCR0A match
  TCCR2A = (1<<WGM21);//
TCCR2B = 0; //CTC CLear timer on compare match enabled 
  // Timer0 Output Compare Interrupt A enable
  TIMSK2 = (1<<OCIE2A);  
  ASSR=0;
}

ISR(PCINT0_vect) { //if pin has changed
  // record timer value
  uint16_t timer_val = TCNT1; //get timer value
  TCNT1 = 0; //clear timer
  uint8_t cur_pin_value = *in_RX & bit_RX; //get pin value
  if (cur_pin_value > 0)cur_pin_value = 1;

  cur_pin_value = !cur_pin_value;

  //verify that a transition has happened
  if (cur_pin_value == reader_pin_last || timer_val < 8)
  {
    return;	// glitched transition; ignore
  }

  reader_pin_last = cur_pin_value;

  // discriminate based on timer_val
  uint8_t bit_choice;
  if (timer_val < 0x2E) {	//if length of a bit 0x27= 160.6us Gchanged //was 27
    if (!reader_started)
      return;

    if (timer_val < 0x1C) {		// if short period 0x17=95.4us Gchanged //was 17
      // small period (64us)
      bit_choice = cur_pin_value ? 1 : 0; //if pin high 1 if low 0
    }
    else { //else it is large period
      // large period (128us)
      bit_choice = cur_pin_value ? 0 : 1; //if pin low 1 if pin high 0
    }

    // YAY! we can have new bit
    reader_byte = (reader_byte << 1); //shift temp byte over
    reader_byte |= bit_choice; //add new bit
    reader_byte_counter++; //count bits
    if (reader_byte_counter == 8) { // if full byte
      // DOUBLE YAY! we can has new byte
      //Serial.println("reader_byte");
      
        reader_data[reader_size] = reader_byte; //put byte in array
        reader_byte_counter = 0; //clear  bit counter
        reader_byte = 0;  //clear temp byte
        reader_size++;    //count bytes
        
    }
    return;
  } //end if 160 us

  else if (timer_val < 0x41) {	// if length of start of frame 0x41=264.8us Gchanged
    // Start Of Frame (200us)
    if (cur_pin_value == 1 && !reader_started) { // if pin high  and reader has not started
      reader_started = 1; //start reader
      reader_byte_counter = 0; // clear bit counter
      reader_byte = 0; //clear temp byte
      reader_size = 0; //clear byte counter
      // change timer overflow to 200us
      OCR1A = 0x31; // set compare match to look for EOF 0x31=200us Gchanged

    }
    return;

  }

  return;
}

ISR(TIMER1_COMPA_vect) { //reader compare match

  if (reader_started) { // if reader has started
    // indicates End Of Frame OR reader timeout
    if (OCR1A == 0x61A7) { //if time outg changed
      // was timeout
      reader_started = 0; //stop reading
    }
    else { //if EOF
      // was end of frame
      // change timer overflow to 100ms
      OCR1A = 0x61A7; //compare match to look for overflow 0x61A7=100ms g changed
      reader_started = 0; //stop reading
   
      unsigned char cksum = crc8buf((unsigned char *)reader_data, (reader_size - 1)); //calculate checksum
      if (cksum != reader_data[reader_size - 1]) { //if error
      }
      // copy all  bytes to the packet buffer
      uint8_t i;
      for (i = 0; i < 15; i++) {
        packet_data[i] = reader_data[i]; //copy bytes
      }
      packet_size = reader_size;
      packet_waiting = (reader_size > 0) ? 1 : 0; //if length greater than 0 packet is waiting
      //
      // stop the reader
    }//end else (is EOF)



  } //end reader started
  else {  }
}

// This checksum code originally from Bruce Lightner,
// Circuit Cellar Issue 183, October 2005
// ftp://ftp.circuitcellar.com/pub/Circuit_Cellar/2005/183/Lightner-183.zip
unsigned char crc8buf(const unsigned char *buf, uint8_t len) {
  unsigned char val;
  unsigned char i;
  unsigned char chksum;

  chksum = 0xff;  // start with all one's
  while (len--) {
    i = 8;
    val = *buf++;
    while (i--) {
      if (((val ^ chksum) & 0x80) != 0) {
        chksum ^= 0x0e;
        chksum = (chksum << 1) | 1;
      } 
      else {
        chksum = chksum << 1;
      }
      val = val << 1;
    }
  }

  return ~chksum;
}