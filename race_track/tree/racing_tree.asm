;; Drag Tree - Copyright 2025 by Michael Kohn
;; Email: mike@mikekohn.net
;;   Web: http://www.mikekohn.net/
;;
;; Control multiple LEDs with pulses.
;;
;;  50ms: Drop by 1 light, ignore if already red.
;; 100ms: Top light (start).
;; 150ms: Red light.
;; 200ms: All off.

.include "tn85def.inc"
.avr8

; CKSEL = 0001
; CLKPS = 0000
; CKDIV8 = 1
;
; FUSE LOW = 01100001 = 0x61

;  cycles   @2 MHz:
;  (100 * 8)  / 2000000 = 0.4 ms 
;  (100 * 64) / 2000000 = 3.2 ms 

;  50: 15.60
; 100: 31.25
; 150: 46.88
; 200: 62.50

MS50  equ 10 
MS100 equ 25 
MS150 equ 40 
MS200 equ 58 
UPPER equ 65 

; r0  = 0
; r1  = 1
; r15 = 255
; r14 = temp
; r17 = temp
; r18 = temp in main
; r20 = count in interrupt
; r21 = current light value
; r22 =
; r23 =
; r24 =
; r25 =
; r26 =
; r27 =
; r28 =
; r29 =
; r30 = Z
; r31 = Z
;

; note: CLKSEL 10

.org 0x000
  rjmp start
.org 0x00a
  rjmp service_interrupt

;; FIXME - erase this padding..
.org 0x020

start:
  ;; Disable interrupts
  cli

  ;; Set up stack ptr
  ;ldi r17, RAMEND>>8
  ;out SPH, r17
  ldi r17, RAMEND & 255
  out SPL, r17

  ;; r0 = 0, r1 = 1, r15 = 255
  eor r0, r0
  eor r1, r1
  inc r1
  ldi r17, 0xff
  mov r15, r17

  ;; Set up PORTB
  ;; PB0: LED 0 (red)
  ;; PB1: LED 1 (green)
  ;; PB2: LED 2 (yellow)
  ;; PB3: LED 3 (yellow)
  ;; PB4: INPUT
  ldi r17, 0x0f
  out DDRB, r17
  out PORTB, r0             ; turn off all of port B

  ;; Set up TIMER0 (100 * prescale 8, 100us per interrupt).
  ldi r17,  100
  out OCR0A, r17

  ldi r17, (1<<OCIE0A)
  out TIMSK, r17                 ; enable interrupt compare A
  ldi r17, (1<<WGM01)
  out TCCR0A, r17                ; normal counting (0xffff is top, count up)
  ldi r17, (1<<CS01)|(1<<CS00)   ; CTC OCR0A  Clear Timer on Compare
  out TCCR0B, r17                ; prescale = 8 from clock source

  ; Enable interrupts
  sei

  ;rcall delay

  ldi r21, 0x01

main:
  out PORTB, r21

wait_pulse_on:
  sbis PINB, 4
  rjmp wait_pulse_on

  ldi r20, 0

wait_pulse_off:
  sbic PINB, 4
  rjmp wait_pulse_off

  mov r18, r20

  ;; If higher than ~40 (4ms), ignore it.
  cpi r18, UPPER 
  brhs main

  ;; If lower than ~40 (4ms), ignore it.
  cpi r18, MS200 
  brlo not_40ms

  ;; Reset all off.
  ldi r21, 0x00
  rjmp main
not_40ms:

  ;; If lower than ~30 (3ms), ignore it.
  cpi r18, MS150 
  brlo not_30ms

  ;; Reset to red.
  ldi r21, 0x01
  rjmp main
not_30ms:

  ;; If lower than ~20 (2ms), ignore it.
  cpi r18, MS100 
  brlo not_20ms

  ;; Reset to yellow (start).
  ldi r21, 0x08
  rjmp main
not_20ms:

  ;; If lower than ~10 (1ms), ignore it.
  cpi r18, MS50
  brlo main

  ;; If already red, ignore it.
  cpi r21, 0x01
  breq main
  lsr r21
  rjmp main

;; This appears to be 1/3 of a second.
;; 13us * 256 = 3.32ms
;; 3.32ms * 100 = 332ms
delay:
  ldi r17, 100
delay_repeat_outer:
  ldi r20, 0
delay_repeat:
  cpi r20, 255
  brne delay_repeat
  dec r17
  brne delay_repeat_outer
  ret

service_interrupt:
  in r7, SREG
  inc r20

exit_interrupt:
  out SREG, r7
  reti

