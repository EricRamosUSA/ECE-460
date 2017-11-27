.global _start

	.text
_start:
	movq	$1, %rax			# write syscall number
	movq	$1, %rdi
	movq	$message+0xF4240, %rsi
	movq	$20, %rdx
	syscall
	# exit(0)
	movq	$60, %rax			# syscall 60 is exit
	movq	$0, %rdi			# code 0
	syscall
message:
	.ascii "Please do a syscall\n"
