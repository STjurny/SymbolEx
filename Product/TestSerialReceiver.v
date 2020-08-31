/*

  Basic serial receiver (UART)
  Copyright (c) 2020 Stanislav Jurny (github.com/STjurny) license MIT

  Serial data format is 8 data bits, without parity, one stop bit (8N1) without hardware flow control.
  Please set parameters ClockFrequency and BaudRate to requirements of your design.
  BaudRate can be max 1/3 of ClockFrequency (but it is recommended greater ratio).

  Receiving serial line is connected to iRXD. Top level clock of your design is connected to iClock.
  Each time receiver receives one valid frame (byte) it makes it available in oData and set oReceived for one clock.
  If an error (missing stop bit) occurs in receiving data oError is set for one clock.

  On iCE40 it uses about 40 LUTs and can run on 175 MHz (by Lattice iCEcube2 / Lattice LSE).

*/

`default_nettype none

module InnerSub();
  initial
    $display("%m|3|InnerSub message");
endmodule


module Inner();
  initial
    $display("%m|1|Inner message");
  
  InnerSub innerSub1();
  InnerSub innerSub2();
endmodule
  
      
module SerialReceiver
  #(
    parameter ClockFrequency = 16000000,  // Top level clock frequency (set for frequency used by your desing)
    parameter BaudRate       = 115200,     // Set to required baud rate (9600, 115200, ...) can be max 1/3 of ClockFrequency
    parameter Parent = "BasicProcessor",
    parameter name2 = "SerialReceiver"
  )
  (
    input  wire       iClock,             // To level Clock used frequency specified in ClockFrequency parameter
    input  wire       iRXD,               // Serial data input with baud rate specified in BaudRate parameter
    output reg  [7:0] oData,              // Received data byte
    output reg        oReceived = 0,      // Signalizes (for one clock) received valid data in oData
    output reg        oError    = 0       // Signalizes (for one clock) error in receiving data (missing stop bit)
  );


  localparam
    TicksPerBit = ClockFrequency / BaudRate,
    TicksPerBitAndHalf = (ClockFrequency + ClockFrequency / 2) / BaudRate;

  localparam
    RawDelayToFirstBit = TicksPerBitAndHalf - 1 - 1, // - zero_tick - idle_tick
    RawDelayToNextBit  = TicksPerBit - 1 - 1;        // - zero_tick - read_data_tick

  localparam
    TimerMSB = $clog2(RawDelayToFirstBit + 1) - 1,   // width of timer
    DelayToFirstBit = RawDelayToFirstBit[TimerMSB:0],
    DelayToNextBit = RawDelayToNextBit[TimerMSB:0];

  localparam
    path = {Parent, "/", name2};

  localparam
    _ = 16 'b 11111111_11111111,
    test2 = 64'o10;
    
  initial
    begin
      $display("test: %h", _ );
      $display("test2: %d", test2);
      //$stop();


      $display("%m");
      $display("%m|1|#Test %L");

      $display("%s|10|Send bit:%0d", path, 7);
      $display("%m|5|-");
      $display(" %m | 5 |- Serial Receiver Summary");
      $display("%m|x|%l TicksPerBit = %d", TicksPerBit);
      $display("%m|5|TicksPerBitAndHalf = %d", TicksPerBitAndHalf);
      $display("%m|5|DelayToFirstBit = %d", DelayToFirstBit);
      $display("%m|label|5|DelayToNextBit = %0d", DelayToNextBit);
      $display("%m|5| TimerMSB = %0d -", TimerMSB);
      
      //$dumpfile("logs/SerialReceiver.vcd");
      //$dumpvars();
    end


  localparam // $States:10
 /**/sReset/* test */=/* */3'b0/* */ //
    ,
    sIdle          = 1,
    sSkipStartBit  = 2,
    sReadDataBit   = 3,
    sWaitToDataBit = 4,
    sWaitToStopBit = 5,
    sCheckStopBit  = 6,
    sErrorRecovery = 7;

    
    Inner inner1();
    Inner inner2();

  reg [2:0] State = sReset;


  reg Rxd, RxdSyncPipe;
  always @(posedge iClock)  // synchronizes Rxd input to avoid metastability issues (it delay Rxd for two clocks)
    if (State == sReset)
      {Rxd, RxdSyncPipe} <= 2'b11;
    else
      {Rxd, RxdSyncPipe} <= {RxdSyncPipe, iRXD};


  reg TimerIsZero;
  reg [TimerMSB:0] Timer;
  always @(posedge iClock)  // compare Timer to zero one clock earlier for optimize speed
    TimerIsZero <= Timer == 1;


  reg [2:0] BitIndex = 0;
  always @(posedge iClock)
    case (State)
      sReset: // 0
        State <= sIdle;

      sIdle: // 1
        begin
          oReceived <= 0;

          if (~Rxd)
            State <= sSkipStartBit;

          Timer <= DelayToFirstBit;
        end

      sSkipStartBit: // 2
        if (!TimerIsZero)
          Timer <= Timer - 1;
        else
          State <= sReadDataBit;

      sReadDataBit: // 3
        begin
          oData <= {Rxd, oData[7:1]};
          BitIndex <= BitIndex + 1;

          if (BitIndex != 7)
            State <= sWaitToDataBit;
          else
            State <= sWaitToStopBit;

          Timer <= DelayToNextBit;
        end

      sWaitToDataBit: // 4
        if (!TimerIsZero)
          Timer <= Timer - 1;
        else
          State <= sReadDataBit;

      sWaitToStopBit: // 5
        if (!TimerIsZero)
          Timer <= Timer - 1;
        else
          State <= sCheckStopBit;

      sCheckStopBit: // 6
        if (Rxd)
          begin
            oReceived <= 1;
            State <= sIdle;
          end
        else
          begin
            oError <= 1;
            State <= sErrorRecovery;
          end

      sErrorRecovery: // 7
        begin
          oError <= 0;
          if (Rxd)
            State <= sIdle;
        end
    endcase

endmodule




























