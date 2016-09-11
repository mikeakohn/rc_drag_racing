.8051
.include "../include/wixel.inc"

;; r0 -
;; r1 -
;; r2 - Save r7 in clear left/right
;; r3 - Count mode: 0=not counting, 1=counting
;; r4 - Chip select for after light turns green.
;; r5 - Temporary in receive_radio funtion.
;; r6 - Temporary in receive_radio function.
;; r7 - Chip selects for displays.

;; Could use the faster RAM, but for now use it like this.
DIGITS_0 equ 0xf040
PACKET_LEN equ 3
DMA_CONFIG equ 0xf000
PACKET equ 0xf010

  ;; Reset Vector
.org 0x400
  ljmp start
  ;; 0) RF TX done / RX ready (RFTXRX)IEN0.RFTXRXIE TCON.RFTXRXIF10
.org 0x0403
  reti
  ;; 1) ADC end of conversion (ADC)IEN0.ADCIE TCON.ADCIF10
.org 0x040b
  reti
  ;; 2) USART0 RX complete (URX0)IEN0.URX0IE TCON.URX0IF10
.org 0x0413
  reti
  ;; 3) USART1 RX complete (URX1)IEN0.URX1IE TCON.URX1IF10
.org 0x041b
  reti
  ;; 4) AES encryption/decryption complete (ENC)IEN0.ENCIE S0CON.ENCIF
.org 0x0423
  reti
  ;; 5) Sleep Timer compare (ST)IEN0.STIE IRCON.STIF11
.org 0x042b
  reti
  ;; 6) Port 2 inputs (P2INT)IEN2.P2IE IRCON2.P2IF11
.org 0x0433
  reti
  ;; 7) USART0 TX complete (UTX0)IEN2.UTX0IE IRCON2.UTX0IF
.org 0x043b
  reti
  ;; 8) DMA transfer complete (DMA)IEN1.DMAIE IRCON.DMAIF
.org 0x0443
  reti
  ;; 9) Timer 1 (16-bit) capture/Compare/overflow (T1)IEN1.T1IE IRCON.T1IF10,11
.org 0x044b
  ljmp interrupt_timer_1
  ;; 10) Timer 2 (MAC Timer) overflow (T2)IEN1.T2IE IRCON.T2IF10, 11
.org 0x0453
  reti
  ;; 11) Timer 3 (8-bit) compare/overflow (T3)IEN1.T3IE IRCON.T3IF
.org 0x045b
  reti
  ;; 12) Timer 4 (8-bit) compare/overflow (T4)IEN1.T4IE IRCON.T4IF10
.org 0x0463
  reti
  ;; 13) Port 0 inputs (P0INT)IEN1.P0IE IRCON.P0IF11
.org 0x046b
  reti
  ;; 14) USART1 TX complete (UTX1)IEN2.UTX1IE IRCON2.UTX1IF
.org 0x0473
  reti
  ;; 15) Port 1 inputs (P1INT)IEN2.P1IE IRCON2.P1IF11
.org 0x047b
  reti
  ;; 16) RF general interrupts (RF)IEN2.RFIE S1CON.RFIF11
.org 0x0483
  reti
  ;; 17) Watchdog overflow in timer mode (WDT)IEN2.WDTIE IRCON2.WDTIF
.org 0x048b
  reti

start:
  ;; Dont't count
  mov r3, #0

  ;; Put clock in 24MHz mode
  mov A, SLEEP
  anl A, #0xfb
  mov SLEEP, A

wait_clock:
  ;; Switch to 24MHz clock
  mov A, SLEEP
  anl A, #0x40
  jz wait_clock

  mov A, #0x80
  mov CLKCON, A

  mov A, SLEEP
  orl A, #0x04
  mov SLEEP, A

  // Init radio registers
.include "../include/radio_registers.inc"

  SET_REGISTER(CHANNR, 128)
  SET_REGISTER(PKTLEN, PACKET_LEN)
  SET_REGISTER(MCSM0, 0x14)
  SET_REGISTER(MCSM1, 0x00)
  ;SET_REGISTER(IOCFG2, 0b011011);

  SET_REGISTER(DMA_CONFIG + 0, 0xdf);
  SET_REGISTER(DMA_CONFIG + 1, RFD);
  SET_REGISTER(DMA_CONFIG + 2, PACKET >> 8);
  SET_REGISTER(DMA_CONFIG + 3, PACKET & 0xff);
  SET_REGISTER(DMA_CONFIG + 4, 0b10000000);
  SET_REGISTER(DMA_CONFIG + 5, PACKET_LEN + 1 + 2);
  SET_REGISTER(DMA_CONFIG + 6, 19);
  SET_REGISTER(DMA_CONFIG + 7, 0x10);

  mov DMA1CFGH, #(DMA_CONFIG >> 8)
  mov DMA1CFGL, #(DMA_CONFIG & 0xff)

  mov DMAARM, #(1 << DMA_CHANNEL_RADIO)

  mov RFST, #SRX

  ;; P0.5 is SPI CLK
  ;; P0.3 is SPI DATA
  mov P0SEL, #(1 << 5) | (1 << 3)

  ;; P0.4 is SPI /CS (right)
  ;; P0.2 is SPI /CS (left)
  mov P0DIR, #(1 << 4) | (1 << 2)
  mov P0, #(1 << 4) | (1 << 2)

  ;; P2.1 is red LED
  mov P2DIR, #(1 << 1)
  mov P2, #(1 << 1)

  ;; Clear diplays
  lcall clear_displays

  ;; P1.0 is debug LED output right
  ;; P1.1 is debug LED output left
  ;; P1.6 is light input left
  ;; P1.7 is light input right
  mov P1DIR, #(1 << 0) | (1 << 1)
  mov P1, #0

  ;; Setup SPI
  ;; SPI, no receiver, master, MSb first
  ;; @24MHz: BAUD_E=13, BAUD_M=85
  ;; Equation: (((256 + 85) * (2^13)) / 2^28) * 24000000
  mov U0CSR, #0
  mov U0GCR, #(0x20 |13)
  mov U0BAUD, #85

  ;; Init display
  lcall set_brightness
  ;mov DPTR, #DIGITS_0
  ;lcall update_display

  ;; Setup Timer 1
  ;; CNT = 18750, DIV=128
  ;; 24,000,000 / 18750 / 128 = 10 times a second interrupt
  ;mov T1CC0L, #(9375 & 0xff)
  ;mov T1CC0H, #(9375 >> 8)
  mov T1CC0L, #(18750 & 0xff)
  mov T1CC0H, #(18750 >> 8)
  ;; IM=1 (enable interrupt), MODE=1 (compare mode)
  mov T1CCTL0, #(1 << 6) | (1 << 2)
  mov IEN1, #(1 << 1)
  mov IE, #0x80
  ;; IDV=3 (128), MODE=2 (modulo)
  mov T1CTL, #(3 << 2) | 2

main:

check_left:
  jb P1.6, left_led_off
  clr P1.1
  ;mov r7, #(1 << 2)
  sjmp check_right
left_led_off:
  setb P1.1

check_right:
  jb P1.7, right_led_off
  clr P1.0
  ;mov r7, #(1 << 4)
  sjmp done_light_check
right_led_off:
  setb P1.0
done_light_check:

  ;; Check if byte is waiting
  lcall receive_radio
  jz main

  xrl P2, #0x02

  mov r6, A
  add A, #0xff
  jnz not_state_1
  mov r3, #0
  lcall clear_displays
  ;mov r4, #0
  ljmp main
not_state_1:
  mov A, r6
  add A, #0xfe
  jnz not_state_2
  ;mov A, r4
  ;mov r7, A
  mov r3, #1
  ljmp main

not_state_2:
  ljmp main

update_display:
  mov A, #0x76
  lcall send_spi_0
  movx A, @DPTR
  jnz show_first_digit
  ;; skip leading 0's
  mov A, #0x79
  lcall send_spi_0
  inc DPTR
  movx A, @DPTR
  jz skip_two_zeros

  mov A, #1
  lcall send_spi_0
  ljmp show_second_digit

skip_two_zeros:
  mov A, #2
  lcall send_spi_0
  ljmp show_third_digit

show_first_digit:
  lcall send_spi_0
  inc DPTR
show_second_digit:
  movx A, @DPTR
  lcall send_spi_0
show_third_digit:
  inc DPTR
  movx A, @DPTR
  lcall send_spi_0
  inc DPTR
  movx A, @DPTR
  lcall send_spi_0
  mov A, #0x77
  lcall send_spi_0
  mov A, #0x04
  lcall send_spi_0
  ret

set_brightness:
  mov r7, #0
  mov A, #0x7a
  lcall send_spi_0
  mov A, #0xff
  lcall send_spi_0
  mov A, #0x76
  lcall send_spi_0
  ret

clear_left:
  mov A, r7
  mov r2, A
  mov r7, #(1 << 4)
  mov A, #0x76
  lcall send_spi_0
  mov A, r2
  mov r7, A
  ret

clear_right:
  mov A, r7
  mov r2, A
  mov r7, #(1 << 2)
  mov A, #0x76
  lcall send_spi_0
  mov A, r2
  mov r7, A
  ret

clear_displays:
  mov r7, #0
  mov A, #0
  mov DPTR, #DIGITS_0
  movx @DPTR, A
  inc DPTR
  movx @DPTR, A
  inc DPTR
  movx @DPTR, A
  inc DPTR
  movx @DPTR, A
  ret

inc_display:
  mov A, r3
  jz inc_display_exit
  ;; Inc digit 3
  mov DPTR, #DIGITS_0+3
  movx A, @DPTR
  inc A
  movx @DPTR, A
  cjne A, #10, stop_carrying
  ;; Update digit 3
  clr A
  movx @DPTR, A
  ;; Inc digit 2
  mov DPTR, #DIGITS_0+2
  movx A, @DPTR
  inc A
  movx @DPTR, A
  cjne A, #10, stop_carrying
  ;; Update digit 2
  clr A
  movx @DPTR, A
  ;; Inc digit 1
  mov DPTR, #DIGITS_0+1
  movx A, @DPTR
  inc A
  movx @DPTR, A
  cjne A, #10, stop_carrying
  ;; Update digit 1
  clr A
  movx @DPTR, A
  ;; Inc digit 0
  mov DPTR, #DIGITS_0+0
  movx A, @DPTR
  inc A
  movx @DPTR, A
  cjne A, #10, stop_carrying
  ;; Update digit 0
  clr A
  movx @DPTR, A
stop_carrying:
inc_display_exit:
  mov DPTR, #DIGITS_0
  lcall update_display
  ret

send_spi_0:
  ;;clr P0.4
  ;;clr P0.2
  mov P0, r7
  mov U0DBUF, A
wait_spi:
  mov A, U0CSR
  anl A, #0x01
  jnz wait_spi
  setb P0.4
  setb P0.2
  ret

interrupt_timer_1:
  push psw
  push ACC
  ;xrl P2, #0x02
  lcall inc_display
  pop ACC
  pop psw
  reti

receive_radio:
  ;; Check if byte is waiting
  mov r6, RFIF
  mov A, r6
  anl A, #(1 << 4)
  jnz receive_radio_has_data
  ret
receive_radio_has_data:

  mov DPTR, #PACKET + 1
  movx A, @DPTR
  mov r5, A

  mov DPTR, #LQI
  movx A, @DPTR
  anl A, #0x80
  jnz receive_radio_crc_okay
  mov r5, #0
receive_radio_crc_okay:

  ;; reset RFIF
  mov A, r6
  anl A, #0xef
  mov RFIF, A
  mov RFST, #SRX
  mov DMAARM, #(1 << DMA_CHANNEL_RADIO)

  ;; return command byte read
  mov A, r5
  ret


