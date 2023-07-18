
;; Drag Race End Of Track - Copyright 2010 by Michael Kohn
;; Email: mike@mikekohn.net
;;   Web: http://www.mikekohn.net/
;;
;; Use IR to detect which car wins a drag race and turn on an LED light
;; for a few seconds after to show it.

.avr8
.include "tn13def.inc"

;  cycles  sample rate   @128 KHz:
;    256 

; r0  = 0
; r1  = 1
; r15 = 255
; r14 = temp
; r17 = temp
; r20 = counter
; r21 = LED settings
;

; note: CLKSEL 11

.org 0x000
  rjmp start
  reti
  reti
  rjmp service_interrupt

start:
  ;; I'm busy.  Don't interrupt me!
  cli

  ;; Set up stack ptr
  ;ldi r17, RAMEND>>8
  ;out SPH, r17
  ldi r17, RAMEND&255
  out SPL, r17

  ;; r0 = 0, r1 = 1, r15 = 255
  eor r0, r0
  eor r1, r1
  inc r1
  eor r15, r15
  dec r15

  ldi r22, 128

  ;; Set up PORTB
  ldi r17, (1<<PB3)|(1<<PB2)
  out DDRB, r17
  ldi r17, (1<<PB3)|(1<<PB2)
  out PORTB, r17

  ;ldi r17,  255                   ; interrupt every 256 cycles
  ;out OCR0A, r17

  ldi r17, (1<<TOIE0)
  out TIMSK0, r17                ; enable interrupt compare A 
  ;ldi r17, (1<<WGM01)
  out TCCR0A, r0                 ; normal counting (0xff is top, count up)
  ;ldi r17, (1<<CS02)|(1<<CS00)   ;
  ldi r17, (1<<CS01)
  out TCCR0B, r17                ; prescale = 1024 from clock source

  ; Fine, I can be interrupted now
  sei

main:
  tst r22
  brne main

  in r23, PINB

  sbrs r23, PB4
  rjmp not_left
  sbi PORTB, PB3
  ldi r22, 128

not_left:
  sbrs r23, PB1
  rjmp not_right
  sbi PORTB, PB2
  ldi r22, 128

not_right:
  rjmp main               ; main() { while(1); }

;; The Great Interrupt Routine!
service_interrupt:
  in r7, SREG

  tst r22
  breq exit_interrupt

  dec r22
  brne exit_interrupt
  cbi PORTB, PB2
  cbi PORTB, PB3

exit_interrupt:
  out SREG, r7
  reti

signature:
.db "Drag Race End Of Track - Copyright 2010 - Michael Kohn - Version 0.01",0

