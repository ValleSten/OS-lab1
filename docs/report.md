# Lab 1: Shell in C
## Operating Systems

### Group 39:
- **Rafaat Al-Zina**
- **Lukas Edlund**
- **Valdemar Stenhammar**

---
# Are The Specifications Met
The current implementation has successfully met all the specifications and requirements provided. The implementation not only passes the test but also is functioning according to what is expected from a shell given a command. All the logic added was tested both manually as well as by using the automated tests. The shell was also tested on a remote StuDAT linux computer to ensure that the code works properly on those systems as well.

# Order of implementation
At first CTRL+D handling was added. Afterwards the built-in commands such as “cd” and “exit” were implemented. Around that time, basic command execution and background execution was implemented as well as handling zombie processes.  Finally the implementation of the I/O redirection and CTRL+C was finalized.

# Challenges for each specification

## Ctrl + D (EOF) handling
When writing the logic that exists it was clear that what needed to be checked for is if readline(“>”) would return NULL. When NULL is returned it means the user input was Ctrl + D (EOF). Therefore an if statement was utilized to do the check and in order to exit the terminal properly. The issue encountered here was that it was not functioning properly due to the fact that “break” was used at first to break out of the for loop. This was later resolved by using “return 0” and afterwards it was functioning as intended.

## Basic Command Execution
The main concern and confusion when it came to command execution was the idea that it was supposed to be implemented as it was done with the built-in commands, as in the logic for the commands was to be implemented as well. After reviewing the provided “README.md” it was concluded that a function from the exec family of functions was to be utilized. The second issue was to determine which of the exec functions was best to use. In the end it was decided to use “execvp” since it does not require a full path to the executable.

## Background command execution
The background command execution was relatively straight forward. The first thing that we did was to make the parent process avoid waiting for the child process to finish if it is a background process. This was accomplished simply with an if-statement checking if it is a background process or not. However, this caused all background processes to become zombies as they were not handled properly. The solution to this problem is presented a bit further down in this report, see header No Zombie processes.

## Piping support
Upon starting to implement piping support, the biggest concern was the fact that the list of programs was in fact in reverse order. There were many proposed resolutions such as reversing the list of pointers but ultimately using recursion is what was used. Recursion was used since it also solved another issue concerning how many “pipes” to use in case the input piped more than two commands. Thus after implementing the pipe handling by recursively going through the list of programs,  it became a bit more manageable.

Another subject of difficulty regarding the piping support was the explanation of the pipe function in the README. Not sure if we have misunderstood something, but it seems to us that the README refers to the parent and child processes incorrectly, i.e. the parent should be the child and vice versa. For instance, in step 4:

>The parent process uses dup2() to set the write-end of the pipe to be its standard output, and the closes the original write-end

But this is what we are doing in the child process. Similarly, we do the opposite in the child process as well. This caused a lot of confusion and took some time to figure out in order to make it work properly.

## I/O Redirection
For the implementation of I/O redirection there were some challenges regarding what to do with the call to open() for the output file. There was some confusion on what flags to include and what modes to include. There were some hints in the given lab description to use O_CREATE and O_TRUNC depending on if the output file existed or not. For the mode argument we tested with some of the  mode file bits to see what worked. Also, we tested with shell and saw what permissions that a created file should have.

## Built-in Commands
“cd” was the most challenging of the built-in as it was unclear  how to acquire the path to the “HOME” directory in the case that there were no other arguments provided by the user. This was resolved using “getenv”. The other parts of the “cd” command were straight forward. The “exit” command was also clear cut and no issues were encountered.

## Ctrl + C Handling
When tackling the issue with Ctrl + C, it was not clear how to get the ID for the process running the commands. Upon later reconsideration and review of the course material and the man page for fork(), the most obvious way was to utilize the value returned from fork in the parent process, since it returns the process id for the created child process. As such, an implementation for Ctrl+C was produced. However, after running the automated tests to control that it was in fact working properly, another issue was found. The implementation at that time terminated not only foreground processes, but also the background processes. At first the thought of a flag was introduced to check if the current process was a background or foreground process. Unfortunately that idea was quickly discarded as it would have not worked since as soon as another command which isn’t a background process was run, the flag would have been reset. Thus, the idea that the background processes should ignore the interrupt signals seemed like it would work. After adding a way for the background processes to ignore Ctrl+C, the implementation began working as intended and the automated test was successfully cleared.

## No Zombie processes
The only issue we experienced with zombie processes was during the implementation of background processes. Our solution to background processes was to let the parent avoid waiting for the child process to finish. However, since the `wait` function provides process clean-up, this causes the background processes to become zombies when finished. To solve this issue, we were required to somehow perform the process clean-up manually without relying on the `wait` method. We noticed that when a child process is terminated or stopped, a SIGCHLD signal is produced. Upon this discovery, we simply decided to ignore the SIGCHLD signal which effectively reaps zombie processes.

# Feedback For Automated Test
The material was of great assistance during the course of the lab, as it provided a plethora of different function suggestions which were helpful with the implementation of the shell. Though the “README” was confusing at first, after carefully reviewing it, it provided enough information to be able to successfully implement the lab. Though if the suggested functions also hinted to what parts these functions should be used at would have been greatly appreciated. 

The tutorial for how to use the automated test was brief, and there were some issues due to what packages to install to be able to run the test, it was later resolved and the tests were working perfectly fine. All in all the automated tests were extremely helpful when checking what was missing with the different implementations. As an example, originally Ctrl + C would not only kill foreground processes but also the background processes. After using the automated tests it was brought to light that the implementation at that time was incorrect. However there was the issue where the automated test was actually bugged and gave an error when Ctrl + C was working properly which led to confusion, but in general it was quite helpful overall. 
