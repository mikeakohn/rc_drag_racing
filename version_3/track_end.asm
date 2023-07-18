;; R/C Drag Racing Track MSP430
;;
;; Copyright 2019 - By Michael Kohn
;; http://www.mikekohn.net/
;; mike@mikekohn.net
;;
;; Ending track for the MSP430 version of the R/C Drag Racing Track

.msp430
.include "msp430x2xx.inc"

RAM equ 0x0200
DIGIT_0 equ RAM+0
DIGIT_1 equ RAM+1
DIGIT_2 equ RAM+2
DIGIT_3 equ RAM+3

;  r4 =
;  r5 =
;  r6 = display needs update
;  r7 =
;  r8 =
;  r9 = is race running boolean
; r10 = UART input
; r11 =
; r12 =
; r13 = SPI /CS for both displays
; r14 = temp
; r15 = function paramter

  .org 0xc000
start:
  ;; Turn off watchdog
  mov.w #(WDTPW|WDTHOLD), &WDTCTL

  ;; Turn off interrupts
  dint

  ;; Setup stack pointer
  mov.w #0x0400, SP

  ;; Set MCLK to 1 MHz with DCO 
  mov.b #DCO_3, &DCOCTL
  mov.b #RSEL_7, &BCSCTL1
  mov.b #0, &BCSCTL2

  ;; Set SMCLK to 32.768kHz external crystal
  mov.b #XCAP_3, &BCSCTL3

  ;; Setup output pins
  ;; P1.1 = RX
  ;; P1.2 = TX
  ;; P1.3 = /CS Left
  ;; P1.4 = /CS Right
  ;; P1.5 = SPI CLK
  ;; P1.6 = SPI SOMI (input)
  ;; P1.7 = SPI SIMO (output)
  ;; P2.1 = LED Left
  ;; P2.2 = Laser Input Left
  ;; P2.3 = Laser Input Right
  ;; P2.4 = LED right
  mov.b #0x18, &P1DIR
  mov.b #0x18, &P1OUT
  mov.b #0xe6, &P1SEL
  mov.b #0xe6, &P1SEL2
  mov.b #0x12, &P2DIR
  mov.b #0x00, &P2OUT
  ;mov.b #0x00, &P2SEL
  ;mov.b #0x00, &P2SEL2

  ;; Setup Timer A
  ;mov.w #512, &TACCR0
  ;mov.w #TASSEL_2|MC_1, &TACTL ; SMCLK, DIV1, COUNT to TACCR0
  ;mov.w #CCIE, &TACCTL0
  ;mov.w #0, &TACCTL1

  ;; Setup Timer B
  mov.w #3277, &TBCCR0  ; 3277 ticks = 1/10 second
  mov.w #TBSSEL_1|MC_1, &TBCTL ; ACLK, DIV1, COUNT to TBCCR0
  mov.w #CCIE, &TBCCTL0
  mov.w #0, &TBCCTL1

  ;; Setup UART (9600 @ 1 MHz)
  mov.b #UCSSEL_2|UCSWRST, &UCA0CTL1
  mov.b #0, &UCA0CTL0
  mov.b #UCBRS_2, &UCA0MCTL
  mov.b #109, &UCA0BR0
  mov.b #0, &UCA0BR1
  bic.b #UCSWRST, &UCA0CTL1

  ;; Setup SPI
  mov.b #UCSWRST, &UCB0CTL1            ; set reset
  bis.b #UCSSEL_2, &UCB0CTL1           ; SMCLK
  mov.b #UCCKPH|UCMSB|UCMST|UCSYNC, &UCB0CTL0 ; MSB first, Master, 3 pin SPI, Sync mode
  mov.b #5, &UCB0BR0
  mov.b #0, &UCB0BR1
  bic.b #UCSWRST, &UCB0CTL1            ; clear reset

  mov.w #0x0, r9

  ;; Turn on interrupts
  eint

  call #reset_display

main:
  mov.b &P2IN, r4
  ;; Check UART for data.
  bit.b #UCA0RXIFG, &IFG2
  jnz read_uart
  ;; Check if display is incrementing.
  cmp.b #1, r6
  jz increment_display
  ;; Check for break in left laser.
check_left_laser:
  bit.b #0x04, &P2IN
  jz laser_break_left
  bic.b #0x02, &P2OUT
  ;; Check for break in right laser.
check_right_laser:
  bit.b #0x08, &P2IN
  jz laser_break_right
  bic.b #0x10, &P2OUT
  jmp main

read_uart:
  mov.b &UCA0RXBUF, r10
  cmp #'R', r10
  jnz read_uart_not_reset
  call #reset_display
  jmp main
read_uart_not_reset:
  cmp #'S', r10
  jnz read_uart_not_start
  mov.w #0, &TBR
  mov.b #1, r9
  jmp main
read_uart_not_start:
  cmp #'l', r10
  jnz read_uart_not_fault_left
  bic.b #0x08, r13
  mov.b #0x76, r15
  call #spi_send_byte_left
  jmp main
read_uart_not_fault_left:
  cmp #'r', r10
  jnz read_uart_not_fault_right
  bic.b #0x10, r13
  mov.b #0x76, r15
  call #spi_send_byte_right
  jmp main
read_uart_not_fault_right:
  jmp main

laser_break_left:
  bic.b #0x08, r13
  bis.b #0x02, &P2OUT
  jmp check_right_laser

laser_break_right:
  bic.b #0x10, r13
  bis.b #0x10, &P2OUT
  jmp main

increment_display:
  mov.b #0, r6
  ;; Increment digit 0
  add.b #1, &DIGIT_0
  cmp.b #10, &DIGIT_0
  jnz increment_display_done
  mov.b #0, &DIGIT_0
  ;; Increment digit 1
  add.b #1, &DIGIT_1
  cmp.b #10, &DIGIT_1
  jnz increment_display_done
  mov.b #0, &DIGIT_1
  ;; Increment digit 2
  cmp.b #' ', &DIGIT_2
  jnz increment_display_not_space_2
  mov.b #0, &DIGIT_2
increment_display_not_space_2:
  add.b #1, &DIGIT_2
  cmp.b #10, &DIGIT_2
  jnz increment_display_done
  mov.b #0, &DIGIT_2
  ;; Increment digit 3
  cmp.b #' ', &DIGIT_3
  jnz increment_display_not_space_3
  mov.b #0, &DIGIT_3
increment_display_not_space_3:
  add.b #1, &DIGIT_3
  cmp.b #10, &DIGIT_3
  jnz increment_display_done
  mov.b #0, &DIGIT_3
increment_display_done:
  call #draw_display
  jmp main

; Clear display
clear_display:
  mov.b #0x18, r13
  mov.b #0x76, r15
  call #spi_send_byte_both
  ret

; Reset display
reset_display:
  mov.w #0, r9
  call #clear_display

  mov.b #' ', &DIGIT_3
  mov.b #' ', &DIGIT_2
  mov.b #0, &DIGIT_1
  mov.b #0, &DIGIT_0

  mov.b #0x77, r15
  call #spi_send_byte_both
  mov.b #0x04, r15
  call #spi_send_byte_both

  call #draw_display
  ret

draw_display:
  mov.b &DIGIT_3, r15
  call #spi_send_byte_both
  mov.b &DIGIT_2, r15
  call #spi_send_byte_both
  mov.b &DIGIT_1, r15
  call #spi_send_byte_both
  mov.b &DIGIT_0, r15
  call #spi_send_byte_both
  ret

; uart_send_byte(r15)
uart_send_byte:
  bit.b #UCA0TXIFG, &IFG2
  jz uart_send_byte
  mov.b r15, &UCA0TXBUF
  ret

; spi_send_byte_both(r15)
spi_send_byte_both:
  bic.b r13, &P1OUT
  mov.b &UCB0RXBUF, r14
  mov.b r15, &UCB0TXBUF
spi_send_byte_both_busy:
  bit.b #UCB0RXIFG, &IFG2
  jz spi_send_byte_both_busy
  bis.b r13, &P1OUT
  ret

; spi_send_byte_left(r15)
spi_send_byte_left:
  bic.b #0x08, &P1OUT
  mov.b &UCB0RXBUF, r14
  mov.b r15, &UCB0TXBUF
spi_send_byte_left_busy:
  bit.b #UCB0RXIFG, &IFG2
  jz spi_send_byte_left_busy
  bis.b #0x08, &P1OUT
  ret

; spi_send_byte_right(r15)
spi_send_byte_right:
  bic.b #0x10, &P1OUT
  mov.b &UCB0RXBUF, r14
  mov.b r15, &UCB0TXBUF
spi_send_byte_right_busy:
  bit.b #UCB0RXIFG, &IFG2
  jz spi_send_byte_right_busy
  bis.b #0x10, &P1OUT
  ret

;; Not used.
timer_interrupt_a:
  reti

;; 1/10 second interrupt
timer_interrupt_b:
  cmp.b #1, r9
  jnz timer_interrupt_b_exit
  mov.b #1, r6
timer_interrupt_b_exit:
  reti

vectors:
.org 0xfff2
  dw timer_interrupt_a     ; Timer0_A3 TACCR0, CCIFG
.org 0xfffa
  dw timer_interrupt_b     ; Timer1_A3 TACCR0, CCIFG
.org 0xfffe
  dw start                 ; Reset

