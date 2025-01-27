# Kernel Module
The kprobe_ioctl.c file is a kernel module written in C that demonstrates how to use kprobes and ioctl for communication between user space and kernel space.

## Compilation
Use the Makefile:

Open the Makefile and modify the following line to set your module's file name:

```obj-m := kprobe_ioctl.o```
* * This ensures the correct kernel module is compiled.
Compile the Kernel Module:

## Module Insertion and Removal
Insert the Kernel Module:

Use the insmod command to insert the module into the kernel:

```sudo insmod kprobe_ioctl.ko```
Check the kernel logs to verify successful insertion:

```dmesg | tail -n 20```

## Remove the Kernel Module:

Use the rmmod command to remove the module:

```sudo rmmod kprobe_ioctl```


# User Space Utility
The userspace_utility.cpp file is a user-space program that interacts with the kernel module via ioctl and reads JSON input for finite state machine (FSM) configuration.

### Prerequisites
Install the JSON library:
Install the JsonCpp library using (Fedora):

```sudo dnf install jsoncpp-devel```

##Compilation

Compile the User Space Program:
Use the following command to compile userspace_utility.cpp:

```g++ userspace_utility.cpp -o userspace_utility -ljsoncpp```

##Running the Program
Run the Utility:

Pass the necessary options and arguments to the program.
* * Turn off logging:

```sudo ./userspace_utility --off```

Enable logging and pass a JSON file:

Create a JSON file 

Save the file as fsm.json.
structure
```["write", "read", "open"]```
Run the program with the JSON file:


```sudo ./userspace_utility --log --file fsm.json```
### JSON Structure
The JSON file defines the FSM states (e.g., system calls) as an array of strings. Example:



The user-space utility reads this JSON and passes the FSM configuration to the kernel module via ioctl.


