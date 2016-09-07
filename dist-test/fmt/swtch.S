2600 # Context switch
2601 #
2602 #   void swtch(struct context **old, struct context *new);
2603 #
2604 # Save current register context in old
2605 # and then load register context from new.
2606 
2607 .globl swtch
2608 swtch:
2609   movl 4(%esp), %eax
2610   movl 8(%esp), %edx
2611 
2612   # Save old callee-save registers
2613   pushl %ebp
2614   pushl %ebx
2615   pushl %esi
2616   pushl %edi
2617 
2618   # Switch stacks
2619   movl %esp, (%eax)
2620   movl %edx, %esp
2621 
2622   # Load new callee-save registers
2623   popl %edi
2624   popl %esi
2625   popl %ebx
2626   popl %ebp
2627   ret
2628 
2629 
2630 
2631 
2632 
2633 
2634 
2635 
2636 
2637 
2638 
2639 
2640 
2641 
2642 
2643 
2644 
2645 
2646 
2647 
2648 
2649 
