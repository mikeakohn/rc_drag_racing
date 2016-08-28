.8051
.include "../include/wixel.inc"

;; r0 -
;; r1 -
;; r2 -
;; r3 -
;; r4 -
;; r5 -
;; r6 -
;; r7 - Chip selects for displays.

;; Could use the faster RAM, but for now use it like this.
DIGITS_0 equ 0xff00
DIGITS_1 equ 0xff04

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
  ;; Put clock in 24MHz mode
  mov A, SLEEP
  anl A, #0xfb
  mov SLEEP, A

wait_clock:
  mov A, SLEEP
  anl A, #0x40
  jz wait_clock

  mov A, #0x80
  mov CLKCON, A

  mov A, SLEEP
  orl A, #0x04
  mov SLEEP, A

  ;; P0.5 is CLK
  ;; P0.3 is DATA
  mov P0SEL, #(1 << 5) | (1 << 3)

  ;; P0.4 is /CS (right)
  ;; P0.2 is /CS (left)
  mov P0DIR, #(1 << 4) | (1 << 2)
  mov P0, #(1 << 4) | (1 << 2)

  ;; P2.1 is red LED
  mov P2DIR, #(1 << 1)
  mov P2, #(1 << 1)

  ;; Clear diplays
  lcall clear_displays

  ;; P1.0 is debug LED output right
  ;; P1.1 is debug LED output left
  ;; P1.6 is IR input left
  ;; P1.7 is IR input right
  mov P1DIR, #(1 << 0) | (1 << 1)
  mov P1, #0

  ;; Setup UART
  ;; SPI, no receiver, master, MSb first
  ;; @24MHz: BAUD_E=13, BAUD_M=85
  ;; Equation: (((256 + 85) * (2^13)) / 2^28) * 24000000
  mov U0CSR, #0
  mov U0GCR, #(0x20 |13)
  mov U0BAUD, #85

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

  mov DPTR, #DIGITS_0
  lcall update_display

main:

check_left:
  jb P1.6, left_led_off
  clr P1.1
  sjmp check_right
left_led_off:
  setb P1.1

check_right:
  jb P1.7, right_led_off
  clr P1.0
  sjmp done_ir_check
right_led_off:
  setb P1.0

done_ir_check:

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
  xrl P2, #0x02
  lcall inc_display
  pop ACC
  pop psw
  reti



