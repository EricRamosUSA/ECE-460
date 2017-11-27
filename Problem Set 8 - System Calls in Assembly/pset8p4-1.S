.global _start

	.text
_start:
	movq	$400, %rax			# not a valid system call
	movq	$1, %rdi
	movq	$message, %rsi
	movq	$20, %rdx
	syscall
	# exit(0)
	movq	$60, %rax			# system call 60 is exit
	movq	$0, %rdi			# code 0
	syscall
message:
	.ascii "Please do a syscall\n"
