global default_isr_stub
global timer_stub
global keyboard_stub

global exception_div_stub
global exception_debug_stub
global exception_nmi_stub
global exception_breakpoint_stub
global exception_overflow_stub
global exception_bound_stub
global exception_invalid_op_stub
global exception_device_na_stub
global exception_double_fault_stub
global exception_tss_stub
global exception_segment_stub
global exception_stack_stub
global exception_gpf_stub
global exception_page_fault_stub

section .text

%macro PUSH 0
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
%endmacro

%macro POP 0
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
%endmacro

%macro EXCEPTION_NO_ERR 2
%1:
    PUSH
    mov rdi, rsp
    extern %2
    call %2
    POP
    iretq
%endmacro

%macro EXCEPTION_ERR 2
%1:
    PUSH
    mov rdi, rsp
    extern %2
    call %2
    POP
    add rsp, 8
    iretq
%endmacro

EXCEPTION_NO_ERR exception_div_stub, handle_exception_divide_by_zero
EXCEPTION_NO_ERR exception_debug_stub, handle_exception_debug
EXCEPTION_NO_ERR exception_nmi_stub, handle_exception_nmi
EXCEPTION_NO_ERR exception_breakpoint_stub, handle_exception_breakpoint
EXCEPTION_NO_ERR exception_overflow_stub, handle_exception_overflow
EXCEPTION_NO_ERR exception_bound_stub, handle_exception_bound
EXCEPTION_NO_ERR exception_invalid_op_stub, handle_exception_invalid_op
EXCEPTION_NO_ERR exception_device_na_stub, handle_exception_device_na
EXCEPTION_ERR exception_double_fault_stub, handle_exception_double_fault
EXCEPTION_ERR exception_tss_stub, handle_exception_tss
EXCEPTION_ERR exception_segment_stub, handle_exception_segment
EXCEPTION_ERR exception_stack_stub, handle_exception_stack
EXCEPTION_ERR exception_gpf_stub, handle_exception_gpf
EXCEPTION_ERR exception_page_fault_stub, handle_exception_page_fault

default_isr_stub:
    PUSH
    POP
    iretq

timer_stub:
    PUSH
    extern isr_timer_handler
    call isr_timer_handler
    POP
    iretq

keyboard_stub:
    PUSH
    extern isr_keyboard_handler
    call isr_keyboard_handler
    POP
    iretq