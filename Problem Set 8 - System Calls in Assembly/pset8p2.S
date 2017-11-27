.global _start

	.text
_start:
	# write(1, "Please do a syscall", 20)
	movq	$1, %rax			# syscall # 1 is write
	movq	$1, %rdi			# file handle 1 is stdout
	movq	$message, %rsi		# address of string to output
	movq	$20, %rdx			# num bytes
	syscall						# system call

	# exit(0)
	movq	$60, %rax			# syscall # 60 is exit
	xor		%rdi, %rdi			# return code 0
	syscall
message:
	.ascii "Please do a syscall\n"
