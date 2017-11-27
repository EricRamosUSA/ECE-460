.global _start

	.text
_start:
	movq	$1, %rax
	movq	$1, %rdi
	movq	$message, %rsi
	movq	$20, %rdx
	syscall
message:
	.ascii "Please do a syscall\n"
