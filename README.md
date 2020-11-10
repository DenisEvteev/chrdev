# chrdev

Add character device to the module. Implement 'open', 'read', 'write', 'release' functionality. Device shall operate in 'echo' mode. Data passed to 'write' shall be returned from 'read'. Establish meaningful limits to prevent device 'abuse'.

Verify operation executing the following commands in two terminals:
    cat > /dev/echo
    cat < /dev/echo
Text typed in first terminal shall appear in second terminal.

Try to transfer big binary file through 'echo' device. Verify that received copy matches sent file.
