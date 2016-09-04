.8051
.include "../include/wixel.inc"

;; r0 -
;; r1 -
;; r2 -
;; r3 - sound counter
;; r4 - interrupt count low byte
;; r5 - interrupt count high byte
;; r6 - lights state
;; r7 -

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

;INTERRUPT_COUNT equ 0xff00

start:
  ;; Initialize INTERRUPT_COUNT
  ;mov DPTR, #INTERRUPT_COUNT
  ;mov A, #0
  ;mov @DPTR, A
  ;inc DPTR
  ;mov @DPTR, A
  mov A, #0
  mov r4, A
  mov r5, A

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

  ;; P0.5 is yellow top
  ;; P0.4 is yellow middle
  ;; P0.3 is yellow bottom
  ;; P0.2 is green
  ;; P0.1 is red left
  ;; P0.0 is red right
  mov P0SEL, #0x00
  mov P0DIR, #0x3f
  mov P0, #0x00

  ;; P2.1 is red LED
  mov P2DIR, #(1 << 1)
  mov P2, #(1 << 1)

  ;; P1.0 is speaker
  ;; P1.5 is start button
  ;; P1.6 is light input left
  ;; P1.7 is light input right
  mov P1DIR, #(1 << 0)
  mov P1, #0
  mov P2INP, #0x40  ; set P1 to pulldowns
  ;setb P1SEL.5

  ;; Setup Timer 1
  ;; CNT = 18750, DIV=128
  ;; 24,000,000 / 18750 / 128 = 10 times a second interrupt
  ;; 24,000,000 / (880 * 2) = 1760 times a second interrupt
  mov T1CC0L, #(13636 & 0xff)
  mov T1CC0H, #(13636 >> 8)
  ;; IM=1 (enable interrupt), MODE=1 (compare mode)
  mov T1CCTL0, #(1 << 6) | (1 << 2)
  mov IEN1, #(1 << 1)
  mov IE, #0x80
  ;; IDV=3 (128), MODE=2 (modulo)
  ;mov T1CTL, #(3 << 2) | 2
  mov T1CTL, #2

  ;; lights state is in starting state (no lights, waiting for button)
  mov r6, #0xff

main:

check_left:
  jb P1.6, left_led_off
  setb P0.1
  sjmp check_right
left_led_off:
  clr P0.1

check_right:
  jb P1.7, right_led_off
  setb P0.0
  sjmp done_light_check
right_led_off:
  clr P0.0
done_light_check:

  cjne r6, #0xff, main
  jnb P1.5, main
  mov r6, #0x00

  ljmp main

interrupt_timer_1:
  push psw
  push ACC

  ;; if r6 is 0xff or 0xfe, don't drop lights or make sound.
  mov A, r6
  add A, #0x01
  jz interrupt_timer_1_exit
  mov A, r6
  add A, #0x02
  jz interrupt_timer_1_exit

  ;; Play A440 or A880
  cjne r6, #0x02, interrupt_timer_1_a440
  xrl P1, #0x01
  sjmp interrupt_timer_1_done_sound
interrupt_timer_1_a440:
  mov A, r5
  add A, #0xfd
  jc interrupt_timer_1_no_sound

  mov A, r6
  anl A, #0x1c
  jz interrupt_timer_1_no_sound
  inc r3
  mov A, r3
  anl A, #1
  cjne A, #1, interrupt_timer_1_done_sound
  xrl P1, #0x01
  sjmp interrupt_timer_1_done_sound

interrupt_timer_1_no_sound:
  clr P1.0

interrupt_timer_1_done_sound:

  ;; Increment interrupt count and leave if not 1 second has passed
  mov A, #1
  add A, r4
  mov r4, A
  mov A, r5
  addc A, #0
  mov r5, A

  cjne r4, #0xe0, interrupt_timer_1_exit
  cjne r5, #0x06, interrupt_timer_1_exit

  ;; Clear interrupt count
  mov A, #0
  mov r4, A
  mov r5, A

  ;; Toggle debug light once a second
  xrl P2, #0x02

  ;; 1 second has passed so update lights
  anl P0, #0x03
  cjne r6, #0, interrupt_timer_1_drop_lights
  mov r6, #0x20
  sjmp interrupt_timer_1_exit

  ;; Drop lights on tree
interrupt_timer_1_drop_lights:
  mov A, r6
  orl P0, A
  clr C
  rrc A
  mov r6, A
  cjne A, #0, interrupt_timer_1_exit
  mov r6, #0xff

interrupt_timer_1_exit:
  pop ACC
  pop psw
  reti

