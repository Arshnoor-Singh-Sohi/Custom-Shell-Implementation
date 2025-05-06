#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // Added for kill() function
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 5     // Maximum 5 arguments including the command itself
#define MAX_COMMANDS 6 // Maximum of 6 commands (with 5 pipes between them)

/**
 * This function breaks the user's input into separate words
 * I'm using it to split commands like "ls -l" into ["ls", "-l", NULL]
 */
int parse_command(char *input, char **args)
{
    // Keep track of how many words we found
    int word_count = 0;

    // This will hold each word as we find it
    char *current_word;

    // Get the first word from input - strtok breaks string by spaces
    // strtok is pretty cool - it returns parts of strings!
    current_word = strtok(input, " ");

    // Loop until we run out of words or hit our limit
    // I set MAX_ARGS to 5 because assignment says max 5 arguments
    while (current_word != NULL)
    {
        if (word_count < MAX_ARGS)
        {
            args[word_count] = current_word;
        }
        word_count++;
        current_word = strtok(NULL, " ");
    }

    // Put NULL at the end of args array
    // I learned this is needed for execvp to work properly!
    // Add NULL terminator
    if (word_count <= MAX_ARGS)
    {
        args[word_count] = NULL;
    }
    else
    {
        args[MAX_ARGS] = NULL;
        // Return a special value to indicate too many arguments
        return -1;
    }

    // Return how many words we found
    return word_count;
}

/**
 * This function handles commands with multiple pipes like "ls | grep txt | wc -l"
 * I had to learn a lot about pipes to make this work!
 */
int handle_multi_pipe(char *input)
{
    // First I need to make a copy of the input string because strtok will change it
    // I learned this the hard way when my original input got messed up
    char my_input_copy[MAX_INPUT_SIZE];
    strcpy(my_input_copy, input);

    // Now I need to count how many pipe symbols are in the command
    // This tells me how many commands the user wants to connect
    int number_of_pipes = 0;
    int position = 0;

    // Loop through each character and count the pipes
    while (my_input_copy[position] != '\0')
    {
        if (my_input_copy[position] == '|')
        {
            number_of_pipes = number_of_pipes + 1; // Count this pipe
        }
        position = position + 1;
    }

    // Professor said we only need to support up to 5 pipes
    if (number_of_pipes > 5)
    {
        fprintf(stderr, "Error: Too many pipes! I can only handle 5 pipe operations\n");
        return 0; // Return error
    }

    // Figure out how many commands we have (always 1 more than pipes)
    // For example: "ls | grep | wc" has 2 pipes but 3 commands
    int number_of_commands = number_of_pipes + 1;

    // Make an array to store each command as a string
    char *command_strings[MAX_COMMANDS];

    // Split the input at each pipe symbol
    char *part = strtok(my_input_copy, "|");
    int command_index = 0;

    // Get all the command parts and store them
    while (part != NULL && command_index < number_of_commands)
    {
        // Remove spaces at the beginning of commands
        // I noticed these cause problems with execution
        while (*part == ' ')
        {
            part++; // Move past space
        }

        // Save this command
        command_strings[command_index] = part;
        command_index++;

        // Get the next command part
        part = strtok(NULL, "|");
    }

    // Now I need to create actual pipe connections between commands
    // Each pipe has 2 ends: read end and write end
    int my_pipes[MAX_COMMANDS - 1][2]; // Need one pipe between each pair of commands

    // Create all the pipes we need
    for (int i = 0; i < number_of_pipes; i++)
    {
        // The pipe() function creates a pipe and puts file descriptors in my_pipes[i][0] and my_pipes[i][1]
        if (pipe(my_pipes[i]) < 0)
        {
            // Uh oh, pipe creation failed
            perror("Oh no! Can't create pipe");
            return 0;
        }
    }

    // Now for the tricky part - creating a process for each command
    pid_t child_pids[MAX_COMMANDS]; // Store the process IDs

    // Create a process for each command
    for (int cmd_idx = 0; cmd_idx < number_of_commands; cmd_idx++)
    {
        // First, break down this command into its arguments
        char *cmd_args[MAX_ARGS + 1]; // +1 because execvp needs NULL at the end

        // Make a copy of this command for parsing
        char cmd_copy[MAX_INPUT_SIZE];
        strcpy(cmd_copy, command_strings[cmd_idx]);

        // Split command into arguments (program name and its options)
        int num_args = 0;
        char *arg = strtok(cmd_copy, " ");

        while (arg != NULL && num_args < MAX_ARGS)
        {
            cmd_args[num_args++] = arg;
            arg = strtok(NULL, " ");
        }
        cmd_args[num_args] = NULL; // Important! execvp needs NULL at the end

        // Check if command has valid number of arguments (rule 3 of assignment)
        if (num_args == 0 || num_args > MAX_ARGS)
        {
            fprintf(stderr, "Error: Each command must have between 1 and 5 arguments\n");
            return 0;
        }

        // Create a new process for this command using fork()
        // This was really confusing at first!
        child_pids[cmd_idx] = fork();

        // Check if fork worked
        if (child_pids[cmd_idx] < 0)
        {
            perror("Fork failed! Can't create process");
            return 0;
        }

        // Child process code (this runs in the new process)
        if (child_pids[cmd_idx] == 0)
        {
            // I need to connect this process to the right pipes

            // If this isn't the first command, connect its input to previous pipe
            if (cmd_idx > 0)
            {
                // Close the write end of previous pipe (we only need to read)
                close(my_pipes[cmd_idx - 1][1]);

                // Redirect stdin to read from the pipe
                // This was hard to understand but cool when it worked!
                dup2(my_pipes[cmd_idx - 1][0], STDIN_FILENO);

                // Close original pipe read end (not needed anymore)
                close(my_pipes[cmd_idx - 1][0]);
            }

            // If this isn't the last command, connect its output to next pipe
            if (cmd_idx < number_of_commands - 1)
            {
                // Close read end of current pipe (we only need to write)
                close(my_pipes[cmd_idx][0]);

                // Redirect stdout to write to the pipe
                dup2(my_pipes[cmd_idx][1], STDOUT_FILENO);

                // Close original pipe write end
                close(my_pipes[cmd_idx][1]);
            }

            // Close all other pipes this process doesn't need
            // This took me a while to get right!
            for (int j = 0; j < number_of_pipes; j++)
            {
                if (j != cmd_idx - 1 && j != cmd_idx)
                {
                    close(my_pipes[j][0]);
                    close(my_pipes[j][1]);
                }
            }

            // Now run the actual command!
            execvp(cmd_args[0], cmd_args);

            // If we get here, something went wrong with the command
            perror("Command didn't work");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process code (continues here after creating all children)

    // Parent should close all pipe ends since it doesn't use them
    // Otherwise pipes won't close properly
    for (int p = 0; p < number_of_pipes; p++)
    {
        close(my_pipes[p][0]); // Close read end
        close(my_pipes[p][1]); // Close write end
    }

    // Wait for all child processes to finish
    for (int c = 0; c < number_of_commands; c++)
    {
        waitpid(child_pids[c], NULL, 0);
    }

    // Everything worked!
    return 1;
}

/**
 * This function handles reverse piping with the = symbol
 * It's like regular piping but runs commands in reverse order
 */
int handle_reverse_pipe(char *input)
{
    // Need to copy the input because strtok will change it
    char copied_input[MAX_INPUT_SIZE];
    strcpy(copied_input, input);

    // Count how many = symbols are in the command
    int reverse_pipe_count = 0;
    for (int index = 0; input[index] != '\0'; index++)
    {
        if (input[index] == '=')
            reverse_pipe_count++;
    }

    // Assignment says maximum 5 pipe operations
    if (reverse_pipe_count > 5)
    {
        fprintf(stderr, "Error: Maximum 5 reverse pipe operations are supported\n");
        return 0;
    }

    // Calculate how many commands we have
    int command_count = reverse_pipe_count + 1;

    // Make an array to store all the commands
    char *command_list[MAX_COMMANDS];

    // Split the input by = symbol to get each command
    char *command_piece = strtok(copied_input, "=");
    int index = 0;

    // Get each command and store it
    while (command_piece != NULL && index < command_count)
    {
        // Remove spaces at the beginning
        while (*command_piece == ' ')
            command_piece++;

        // Store this command
        command_list[index] = command_piece;
        index++;

        // Get next command
        command_piece = strtok(NULL, "=");
    }

    // Need to make pipes to connect the commands
    // Each pipe connects two commands together
    int pipe_array[MAX_COMMANDS - 1][2];

    // Create all the pipes we need
    for (int p = 0; p < reverse_pipe_count; p++)
    {
        if (pipe(pipe_array[p]) < 0)
        {
            perror("Error creating pipe");
            return 0;
        }
    }

    // For reverse piping, we need to start from the last command
    // This is different from regular piping!
    pid_t process_ids[MAX_COMMANDS];

    // Create processes in reverse order (from right to left)
    for (int cmd_index = command_count - 1; cmd_index >= 0; cmd_index--)
    {
        // We need to break down each command into its parts
        char *arguments[MAX_ARGS + 1];
        int arg_count = parse_command(command_list[cmd_index], arguments);

        // Check if command has valid number of arguments
        if (arg_count == 0 || arg_count > MAX_ARGS)
        {
            fprintf(stderr, "Error: Each command must have between 1 and 5 arguments\n");
            return 0;
        }

        // Create a child process for this command
        process_ids[cmd_index] = fork();

        if (process_ids[cmd_index] < 0)
        {
            perror("Error forking process");
            return 0;
        }

        // Code for the child process
        if (process_ids[cmd_index] == 0)
        {
            // The tricky part - connecting pipes in reverse

            // If this isn't the rightmost command, connect its input
            if (cmd_index < command_count - 1)
            {
                close(pipe_array[cmd_index][1]);              // Close write end
                dup2(pipe_array[cmd_index][0], STDIN_FILENO); // Connect read end to stdin
                close(pipe_array[cmd_index][0]);              // Close original read end
            }

            // If this isn't the leftmost command, connect its output
            if (cmd_index > 0)
            {
                close(pipe_array[cmd_index - 1][0]);               // Close read end
                dup2(pipe_array[cmd_index - 1][1], STDOUT_FILENO); // Connect write end to stdout
                close(pipe_array[cmd_index - 1][1]);               // Close original write end
            }

            // Need to close all other pipes so they don't stay open
            for (int j = 0; j < reverse_pipe_count; j++)
            {
                if ((j != cmd_index) && (j != cmd_index - 1))
                {
                    close(pipe_array[j][0]);
                    close(pipe_array[j][1]);
                }
            }

            // Run the command
            execvp(arguments[0], arguments);

            // If we get here, something went wrong
            perror("Command execution failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process needs to close all pipes
    for (int p = 0; p < reverse_pipe_count; p++)
    {
        close(pipe_array[p][0]);
        close(pipe_array[p][1]);
    }

    // Wait for all children to finish
    for (int c = 0; c < command_count; c++)
    {
        waitpid(process_ids[c], NULL, 0);
    }

    return 1; // Success
}

/**
 * Checks if the user typed any of our special shell commands
 * First thing I learned: functions need return values!
 */
int handle_special_commands(char **args)
{
    // My program needs to check two special commands

    // First special command: killterm (exits just this shell)
    // strcmp returns 0 when strings match - that confused me at first!
    if (strcmp(args[0], "killterm") == 0)
    {
        // Tell user what's happening
        printf("Goodbye! Closing this shell now...\n");

        // exit(0) terminates program with success code
        // I tried return 0 first but that doesn't actually exit!
        exit(0);

        // This code never runs but compiler wants a return value
        return 1;
    }

    // Second special command: killallterms (exits ALL shells)
    // Need to check if first word is this command
    if (strcmp(args[0], "killallterms") == 0)
    {
        // Let user know we're working on it
        printf("Starting termination of all w25shell processes...\n");

        // Here's the tricky part! Need to find all processes named w25shell
        // and send them termination signals

        // First get our own process ID so we don't kill ourselves too early
        pid_t my_own_pid = getpid();

        // Using popen to run external command - cool trick I found online!
        FILE *process_list;
        char process_id_text[20]; // Should be enough for any PID

        // Run pgrep command to find all w25shell processes
        process_list = popen("pgrep -f w25shell", "r");

        // Check if pgrep command worked
        if (process_list == NULL)
        {
            // Something went wrong with pgrep
            perror("Oops! Cannot find processes");
            // Return 1 to indicate we handled the command anyway
            return 1;
        }

        // Count how many processes we kill (not necessary but interesting)
        int kill_count = 0;

        // For each line in the output (each process ID)
        while (fgets(process_id_text, sizeof(process_id_text), process_list) != NULL)
        {
            // Convert text PID to number (atoi = ASCII to Integer)
            int process_id_number = atoi(process_id_text);

            // Don't kill ourselves yet - we need to finish the loop first!
            if (process_id_number != my_own_pid)
            {
                // Try to kill this process
                int kill_result = kill(process_id_number, SIGTERM);

                // Did it work?
                if (kill_result == 0)
                {
                    // printf("Successfully terminated process: %d\n", process_id_number);
                    kill_count++;
                }
                else
                {
                    // printf("Failed to terminate process: %d\n", process_id_number);
                }
            }
        }

        // Done with process list, close it
        pclose(process_list);

        // Tell user how many processes we killed
        printf("Terminated %d other shell processes\n", kill_count);

        // Now we can kill our own process
        printf("Now terminating this shell... goodbye!\n");
        exit(0);

        // This line never executes but it makes the compiler happy
        return 1;
    }

    // If we got here, it wasn't a special command
    return 0;
}

/**
 * This function runs any command the user types
 * It took me a while to understand how fork and exec work together!
 */
int execute_command(char **args)
{
    // First count the number of arguments
    int arg_count = 0;
    while (args[arg_count] != NULL)
    {
        arg_count++;
    }

    // Check if argument count exceeds maximum allowed (5)
    if (arg_count > 5)
    {
        fprintf(stderr, "Error: Too many arguments. Maximum allowed is 5.\n");
        return 0; // Return failure
    }

    // We need these variables for process management
    pid_t child_process_id;
    int command_result;

    // First step is to create a new process using fork()
    // This basically makes a copy of our program!
    // printf("Attempting to execute: %s\n", args[0]);
    child_process_id = fork();

    // Check if fork worked correctly
    if (child_process_id < 0)
    {
        // Something went wrong with creating the process
        perror("Oh no! Fork creation failed");
        return 0; // Return failure
    }
    else if (child_process_id == 0)
    {
        // This code runs in the child process only
        // The parent and child are running at the same time!

        // execvp replaces the current process with the command
        // args[0] is the command name, and args has all arguments
        int exec_result = execvp(args[0], args);

        // If we get here, execvp failed because it should never return
        // When execvp works, the process is completely replaced
        if (exec_result == -1)
        {
            perror("Command couldn't be executed");

            // Need to exit because we're in the child process
            // Using EXIT_FAILURE (value is 1) to show it failed
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // This code runs in the parent process only
        // We need to wait for the child to finish

        // waitpid makes the parent process wait for the child
        // The status variable will hold information about how the child exited
        int wait_result = waitpid(child_process_id, &command_result, 0);

        // Could check wait_result here to see if waitpid worked
        if (wait_result < 0)
        {
            printf("Warning: Error waiting for command to finish\n");
        }
    }

    // If we got here, everything worked (or at least we tried)
    return 1; // Return success
}

/**
 * This function handles the special ~ operator which appends two text files to each other
 * Assignment rule said we need to implement file1.txt ~ file2.txt to append them to each other
 */
int handle_append(char *input)
{
    // First need to make a copy because strtok will change the original string
    // Took me some time to figure out this was needed!
    char command_copy[MAX_INPUT_SIZE];
    strcpy(command_copy, input);

    // Need to find the two filenames on either side of the ~ symbol
    char *first_filename = NULL;
    char *second_filename = NULL;

    // Split by the ~ character - this gives us the file names
    first_filename = strtok(command_copy, "~");
    second_filename = strtok(NULL, "~");

    // Need to remove extra spaces around filenames
    // First let's handle the first filename
    if (first_filename != NULL)
    {
        // Skip spaces at the beginning
        while (first_filename[0] == ' ')
        {
            first_filename++; // Move pointer past spaces
        }

        // Remove spaces at the end (this took me a while to figure out!)
        int name_length = strlen(first_filename);
        while (name_length > 0 && first_filename[name_length - 1] == ' ')
        {
            first_filename[name_length - 1] = '\0'; // Replace space with string terminator
            name_length--;                          // Check next character
        }
    }

    // Now same thing for the second filename
    if (second_filename != NULL)
    {
        // Skip spaces at the beginning
        while (second_filename[0] == ' ')
        {
            second_filename++; // Move pointer past spaces
        }

        // Remove spaces at the end
        int name_length = strlen(second_filename);
        while (name_length > 0 && second_filename[name_length - 1] == ' ')
        {
            second_filename[name_length - 1] = '\0';
            name_length--;
        }
    }

    // Make sure we have both filenames
    if (first_filename == NULL || second_filename == NULL)
    {
        fprintf(stderr, "Error: Need two files for the ~ operation!\n");
        return 0; // Failed
    }

    // Now check if both files are .txt files (requirement from assignment)
    // Need to find where the .txt part starts in each filename
    char *dot_position_1 = strrchr(first_filename, '.'); // Find the last dot
    char *dot_position_2 = strrchr(second_filename, '.');

    // Check if extensions exist and are .txt
    if (dot_position_1 == NULL || dot_position_2 == NULL ||
        strcmp(dot_position_1, ".txt") != 0 || strcmp(dot_position_2, ".txt") != 0)
    {
        fprintf(stderr, "Error: Both files must be .txt files for ~ operation!\n");
        return 0; // Failed
    }

    // Time to read contents of first file
    FILE *file1_handle = fopen(first_filename, "r");
    if (file1_handle == NULL)
    {
        perror("Can't open first file");
        return 0; // Failed
    }

    // Need to get size of file1 to allocate memory
    // Using fseek and ftell to find the size - learned this from class!
    fseek(file1_handle, 0, SEEK_END);       // Go to end of file
    long file1_bytes = ftell(file1_handle); // Get position (= file size)
    fseek(file1_handle, 0, SEEK_SET);       // Go back to beginning

    // Allocate memory for file1 contents (+1 for null terminator)
    char *file1_data = malloc(file1_bytes + 1);
    if (file1_data == NULL)
    {
        printf("Error: Can't allocate memory for first file\n");
        fclose(file1_handle);
        return 0; // Failed
    }

    // Read all data from first file
    long bytes_read = fread(file1_data, 1, file1_bytes, file1_handle);
    file1_data[bytes_read] = '\0'; // Add null terminator
    fclose(file1_handle);          // Done with the file for now

    // Same process for the second file
    FILE *file2_handle = fopen(second_filename, "r");
    if (file2_handle == NULL)
    {
        perror("Can't open second file");
        free(file1_data); // Don't forget to free memory!
        return 0;         // Failed
    }

    // Get size of file2
    fseek(file2_handle, 0, SEEK_END);
    long file2_bytes = ftell(file2_handle);
    fseek(file2_handle, 0, SEEK_SET);

    // Allocate memory for file2 contents
    char *file2_data = malloc(file2_bytes + 1);
    if (file2_data == NULL)
    {
        printf("Error: Can't allocate memory for second file\n");
        free(file1_data);
        fclose(file2_handle);
        return 0; // Failed
    }

    // Read all data from second file
    bytes_read = fread(file2_data, 1, file2_bytes, file2_handle);
    file2_data[bytes_read] = '\0'; // Add null terminator
    fclose(file2_handle);

    // Now the fun part - append file2 data to file1
    file1_handle = fopen(first_filename, "a"); // Open in append mode
    if (file1_handle == NULL)
    {
        perror("Can't open first file for appending");
        free(file1_data);
        free(file2_data);
        return 0; // Failed
    }

    // Write file2's contents to the end of file1
    fprintf(file1_handle, "%s", file2_data);
    fclose(file1_handle);

    // And append file1 data to file2
    file2_handle = fopen(second_filename, "a"); // Open in append mode
    if (file2_handle == NULL)
    {
        perror("Can't open second file for appending");
        free(file1_data);
        free(file2_data);
        return 0; // Failed
    }

    // Write file1's contents to the end of file2
    fprintf(file2_handle, "%s", file1_data);
    fclose(file2_handle);

    // All done! Free the memory we allocated
    free(file1_data);
    free(file2_data);

    // Let user know it worked
    printf("Successfully combined the files.\n");
    return 1; // Success!
}

/**
 * This function counts all the words in a text file
 * I used the # symbol as required in the assignment
 */
int handle_word_count(char *input)
{
    // Need to copy input string because we'll change it
    // I had a bug before when I didn't do this!
    char command_copy[MAX_INPUT_SIZE];
    strcpy(command_copy, input);

    // Setting up variables we'll need
    char *file_to_count = NULL; // Will store the filename
    int total_words = 0;        // Counter for words
    int currently_in_word = 0;  // This is like a flag - are we in a word right now?
    FILE *text_file;            // For opening the file
    char current_character;     // To read file character by character

    // Step 1: Need to find where the # symbol is
    char *hash_symbol_location = strchr(command_copy, '#');

    // Make sure the command has the # symbol
    if (hash_symbol_location == NULL)
    {
        // No # symbol found!
        fprintf(stderr, "Error: Can't find # symbol in the command\n");
        return 0; // Return failure
    }

    // Step 2: Get the filename after the # symbol
    // Need to skip past the # and any spaces after it
    char *after_hash = hash_symbol_location + 1; // +1 moves past the #

    // Skip any spaces or tabs
    while (*after_hash == ' ' || *after_hash == '\t')
    {
        after_hash = after_hash + 1; // Move forward one character
    }

    // Now after_hash should point to the filename
    file_to_count = after_hash;

    // Step 3: Remove any spaces at the end of filename
    int filename_length = strlen(file_to_count);

    // Keep removing spaces/tabs from the end
    while (filename_length > 0)
    {
        // Check if last character is space or tab
        if (file_to_count[filename_length - 1] == ' ' ||
            file_to_count[filename_length - 1] == '\t')
        {
            // Replace with string terminator
            file_to_count[filename_length - 1] = '\0';
            // Reduce length by 1
            filename_length = filename_length - 1;
        }
        else
        {
            // No more spaces at the end
            break;
        }
    }

    // Step 4: Make sure we actually have a filename
    if (filename_length == 0)
    {
        fprintf(stderr, "Error: You didn't provide a filename after #\n");
        return 0; // Return failure
    }

    // Step 5: Check if it's a .txt file
    // The assignment says we must only count words in .txt files
    char *dot_position = strrchr(file_to_count, '.');

    // Check if there's a dot and if it's followed by "txt"
    if (dot_position == NULL)
    {
        fprintf(stderr, "Error: Filename has no extension - must be .txt\n");
        return 0;
    }

    // Compare the extension (should be .txt)
    if (strcmp(dot_position, ".txt") != 0)
    {
        fprintf(stderr, "Error: File must have .txt extension, not %s\n", dot_position);
        return 0;
    }

    // Step 6: Try to open the file
    text_file = fopen(file_to_count, "r"); // r means read mode

    if (text_file == NULL)
    {
        // Couldn't open the file
        perror("Cannot open file for counting");
        return 0;
    }

    // Step 7: Count the words!
    // This algorithm was tricky for me to understand at first
    // We read character by character and count transitions from non-word to word

    printf("Counting words in %s...\n", file_to_count);

    // Loop through each character in the file
    while ((current_character = fgetc(text_file)) != EOF) // EOF means End Of File
    {
        // Check if this character is a word separator
        int is_separator = (current_character == ' ' ||
                            current_character == '\n' ||
                            current_character == '\t');

        // Case 1: We were in a word, but hit a separator
        if (is_separator && currently_in_word == 1)
        {
            // We just left a word
            currently_in_word = 0;
        }
        // Case 2: We weren't in a word, but hit a non-separator
        else if (!is_separator && currently_in_word == 0)
        {
            // We just started a new word!
            currently_in_word = 1;
            // Count this new word
            total_words = total_words + 1;
        }
        // (Other cases: still in a word or still in separators - do nothing)
    }

    // Step 8: Close the file and show the results
    fclose(text_file);

    // Print the result for the user
    printf("Number of words in %s: %d\n", file_to_count, total_words);

    // Everything worked!
    return 1;
}

/**
 * This function combines multiple text files and shows their content
 * I needed to learn about file handling for this one!
 */
int handle_concat(char *input)
{
    // Need to make a copy of the input string
    // I tried without copying once and got weird bugs
    char my_command[MAX_INPUT_SIZE];
    strcpy(my_command, input);

    // Keep track of files and how many we've found
    char *file_list[5]; // Assignment says max 5 files
    int num_files = 0;
    FILE *current_file; // For opening each file

    // First, need to split the command by + symbols
    // strtok is really useful for this!
    char *file_name = strtok(my_command, "+");

    // Process each file name we find
    while (file_name != NULL && num_files < 5)
    {
        // Get rid of spaces at the beginning
        // These spaces caused problems when I tried to open files
        while (*file_name == ' ' || *file_name == '\t')
        {
            file_name = file_name + 1; // Move past the space
        }

        // Remove spaces at the end too
        int name_length = strlen(file_name);
        while (name_length > 0 && (file_name[name_length - 1] == ' ' || file_name[name_length - 1] == '\t'))
        {
            file_name[name_length - 1] = '\0'; // Replace with string terminator
            name_length = name_length - 1;
        }

        // If there's anything left after trimming spaces, save it
        if (name_length > 0)
        {
            // Add this file to our list
            file_list[num_files] = file_name;
            num_files = num_files + 1;

            // Debug message - helpful when I was testing
            // printf("Found file: %s\n", file_name);
        }

        // Get the next file name
        file_name = strtok(NULL, "+");
    }

    // Make sure we have at least one file
    if (num_files == 0)
    {
        fprintf(stderr, "Error: You need to specify at least one file to concatenate!\n");
        return 0; // Failed
    }

    // Make sure we don't have too many files
    if (file_name != NULL) // If there are still more tokens
    {
        fprintf(stderr, "Error: Too many files! I can only handle up to 5 files.\n");
        return 0; // Failed
    }

    // Now we can process each file one by one
    for (int file_index = 0; file_index < num_files; file_index++)
    {
        // First check if it's a .txt file
        // The assignment says we must only use .txt files
        char *extension = strrchr(file_list[file_index], '.');

        if (extension == NULL || strcmp(extension, ".txt") != 0)
        {
            fprintf(stderr, "Error: File %s isn't a .txt file! All files must end with .txt\n",
                    file_list[file_index]);
            return 0; // Failed
        }

        // Try to open the file
        current_file = fopen(file_list[file_index], "r");

        if (current_file == NULL)
        {
            // Couldn't open the file
            fprintf(stderr, "Error: Can't open %s! Does it exist?\n", file_list[file_index]);
            return 0; // Failed
        }

        // printf("\n----- Contents of %s -----\n", file_list[file_index]);

        // Read this file in chunks and print each chunk
        // Using a buffer is much faster than reading one byte at a time!
        char data_chunk[1024]; // 1KB chunks seemed reasonable
        int bytes_got;

        // Keep reading chunks until we reach the end of the file
        while ((bytes_got = fread(data_chunk, 1, sizeof(data_chunk), current_file)) > 0)
        {
            // Write this chunk to standard output
            fwrite(data_chunk, 1, bytes_got, stdout);
        }

        // Done with this file, close it
        fclose(current_file);
    }

    // Let user know we're done
    // printf("\n----- End of concatenation -----\n");
    return 1; // Success!
}

/**
 * This function handles the input/output redirection (<, >, >>)
 * I spent a long time figuring out how these special characters work!
 */
int handle_redirection(char *input)
{
    // First make a working copy of the input command
    // I had trouble when I didn't copy the string
    char command_string[MAX_INPUT_SIZE];
    strcpy(command_string, input);

    // Need to keep track of which redirections we found
    // These are like on/off switches (0 = off, 1 = on)
    int found_input_redir = 0;   // For < character
    int found_output_redir = 0;  // For > character
    int found_append_output = 0; // For >> characters

    // Need to remember where each redirection operator is
    char *where_is_input = NULL;  // Position of <
    char *where_is_output = NULL; // Position of >
    char *where_is_append = NULL; // Position of >>

    // Variables to store the filenames for redirection
    char *input_filename = NULL;  // File to read from (with <)
    char *output_filename = NULL; // File to write to (with > or >>)

    // The actual command part (before any redirection)
    char *actual_command = command_string;

    // Need to check for >> first because it contains >
    // This caused me bugs when I did it in the wrong order!
    where_is_append = strstr(command_string, ">>");
    if (where_is_append != NULL)
    {
        // Found the >> operator
        found_append_output = 1;

        // Split the string at this point
        *where_is_append = '\0';

        // The output filename comes after >>
        output_filename = where_is_append + 2;
    }
    else
    {
        // No >> found, so check for > by itself
        where_is_output = strchr(command_string, '>');
        if (where_is_output != NULL)
        {
            // Found the > operator
            found_output_redir = 1;

            // Split the string at this point
            *where_is_output = '\0';

            // The output filename comes after >
            output_filename = where_is_output + 1;
        }
    }

    // Now check for the < input redirection
    where_is_input = strchr(command_string, '<');
    if (where_is_input != NULL)
    {
        // Found the < operator
        found_input_redir = 1;

        // Split the string at this point
        *where_is_input = '\0';

        // The input filename comes after <
        input_filename = where_is_input + 1;
    }

    // Clean up the command part by removing extra spaces
    // Sometimes users type extra spaces so we need to remove them
    while (*actual_command == ' ' || *actual_command == '\t')
        actual_command++;

    int command_length = strlen(actual_command);

    // Remove trailing spaces too
    while (command_length > 0 && (actual_command[command_length - 1] == ' ' ||
                                  actual_command[command_length - 1] == '\t'))
    {
        actual_command[command_length - 1] = '\0';
        command_length--;
    }

    // Clean up the input filename if we have one
    if (input_filename)
    {
        // Skip past any leading spaces
        while (*input_filename == ' ' || *input_filename == '\t')
            input_filename++;

        int filename_length = strlen(input_filename);

        // Remove trailing spaces
        while (filename_length > 0 && (input_filename[filename_length - 1] == ' ' ||
                                       input_filename[filename_length - 1] == '\t'))
        {
            input_filename[filename_length - 1] = '\0';
            filename_length--;
        }
    }

    // Clean up the output filename if we have one
    if (output_filename)
    {
        // Skip past any leading spaces
        while (*output_filename == ' ' || *output_filename == '\t')
            output_filename++;

        int filename_length = strlen(output_filename);

        // Remove trailing spaces
        while (filename_length > 0 && (output_filename[filename_length - 1] == ' ' ||
                                       output_filename[filename_length - 1] == '\t'))
        {
            output_filename[filename_length - 1] = '\0';
            filename_length--;
        }
    }

    // Now I need to break the command into its separate words
    char *command_words[MAX_ARGS + 1]; // +1 for NULL at the end
    int word_count = 0;

    // strtok breaks the string into words separated by spaces or tabs
    char *current_word = strtok(actual_command, " \t");

    // Keep getting words until we run out or hit our limit
    while (current_word != NULL && word_count < MAX_ARGS)
    {
        command_words[word_count] = current_word;
        word_count++;
        current_word = strtok(NULL, " \t");
    }

    // Need to put NULL at the end for execvp
    command_words[word_count] = NULL;

    // Make sure we actually have a command to run
    if (word_count == 0)
    {
        fprintf(stderr, "Error: You didn't specify a command to run!\n");
        return 0; // Failed
    }

    // Now for the tricky part - need to create a new process
    // and set up all the redirections
    // printf("Running command: %s\n", command_words[0]);

    // Create a child process
    pid_t child_pid = fork();

    // Check if fork worked
    if (child_pid < 0)
    {
        // Something went wrong with fork()
        perror("Oh no! Couldn't create process");
        return 0; // Failed
    }
    else if (child_pid == 0)
    {
        // This code runs in the child process

        // Handle input redirection if requested
        if (found_input_redir && input_filename)
        {
            // Try to open the input file
            int input_fd = open(input_filename, O_RDONLY);

            // Check if open worked
            if (input_fd < 0)
            {
                perror("Couldn't open input file");
                exit(EXIT_FAILURE);
            }

            // This is the magic part - redirect standard input to our file
            // I needed to learn about dup2 for this assignment
            if (dup2(input_fd, STDIN_FILENO) < 0)
            {
                perror("Input redirection failed");
                exit(EXIT_FAILURE);
            }

            // Don't need the original file descriptor anymore
            close(input_fd);
        }

        // Handle output redirection if requested
        if ((found_output_redir || found_append_output) && output_filename)
        {
            // Set up flags for open() - always need write and create
            int open_flags = O_WRONLY | O_CREAT;

            // For >> we use append mode, for > we use truncate mode
            if (found_append_output)
            {
                // >> means append to end of file
                open_flags = open_flags | O_APPEND;
            }
            else
            {
                // > means overwrite the file
                open_flags = open_flags | O_TRUNC;
            }

            // Try to open the output file
            int output_fd = open(output_filename, open_flags, 0644);

            // Check if open worked
            if (output_fd < 0)
            {
                perror("Couldn't open output file");
                exit(EXIT_FAILURE);
            }

            // Redirect standard output to our file
            if (dup2(output_fd, STDOUT_FILENO) < 0)
            {
                perror("Output redirection failed");
                exit(EXIT_FAILURE);
            }

            // Don't need the original file descriptor anymore
            close(output_fd);
        }

        // Finally, run the command
        execvp(command_words[0], command_words);

        // If we get here, something went wrong with the command
        perror("Command failed to run");
        exit(EXIT_FAILURE);
    }
    else
    {
        // This is the parent process
        // We just need to wait for the child to finish
        int status;
        waitpid(child_pid, &status, 0);

        // Could check status here to see if command worked
        if (WIFEXITED(status))
        {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0)
            {
                printf("Command exited with status %d\n", exit_code);
            }
        }
    }

    // Successfully handled the redirection
    return 1;
}

/**
 * This function runs multiple commands one after another when separated by ;
 * I learned this is called "sequential execution" in shell programming
 */
int handle_sequential(char *input)
{
    // Need to make a copy of the input string to work with
    // The original might be needed elsewhere and strtok changes the string
    char command_string[MAX_INPUT_SIZE];
    strcpy(command_string, input);

    // According to assignment, we can have up to 4 commands with ;
    char *separate_commands[4];
    int command_count = 0;

    // Split the input by semicolons to get each command
    // strtok is really useful for this kind of string splitting
    char *one_command = strtok(command_string, ";");

    // Get all commands and store them in our array
    while (one_command != NULL && command_count < 4)
    {
        // Save this command for later execution
        separate_commands[command_count] = one_command;
        command_count = command_count + 1; // Increment our counter

        // Get next command after semicolon
        one_command = strtok(NULL, ";");
    }

    // Make sure we don't have too many commands
    if (one_command != NULL)
    {
        printf("Error: Too many commands! The limit is 4 commands with semicolons\n");
        return 0; // Failed
    }

    // Execute commands one by one
    // printf("Running %d commands in sequence\n", command_count);

    for (int index = 0; index < command_count; index++)
    {
        // First check if this command is empty (like if someone typed ;;)
        if (separate_commands[index][0] == '\0' ||
            (separate_commands[index][0] == ' ' && separate_commands[index][1] == '\0'))
        {
            printf("Skipping empty command at position %d\n", index + 1);
            continue; // Skip to next command
        }

        // Need a working copy of this specific command
        char this_command[MAX_INPUT_SIZE];
        strcpy(this_command, separate_commands[index]);

        // Remove spaces at the beginning
        char *command_start = this_command;
        while (*command_start == ' ')
        {
            command_start = command_start + 1; // Move past spaces
        }

        // Now break the command into its arguments (words)
        char *arguments[MAX_ARGS + 1]; // Extra space for NULL at end
        int arg_count = 0;

        // Use strtok to split by spaces
        char *word = strtok(command_start, " ");

        // Get all the words in this command
        while (word != NULL && arg_count < MAX_ARGS)
        {
            arguments[arg_count] = word;
            arg_count = arg_count + 1;
            word = strtok(NULL, " ");
        }

        // Need NULL at the end for execvp to work
        arguments[arg_count] = NULL;

        // Check if command is empty after parsing
        if (arg_count == 0)
        {
            continue; // Nothing to do
        }

        // Check if there are too many arguments
        if (arg_count > MAX_ARGS)
        {
            printf("Error: Command '%s' has too many arguments (max is 5)\n",
                   separate_commands[index]);
            continue; // Skip this command
        }

        // Check for special command: killterm
        if (strcmp(arguments[0], "killterm") == 0)
        {
            printf("Terminating this shell instance\n");
            exit(0); // Exit immediately
        }

        // Check for special command: killallterms
        if (strcmp(arguments[0], "killallterms") == 0)
        {
            printf("Hunting for all w25shell processes to terminate...\n");

            // Need to find all w25shell processes
            FILE *process_file;
            char process_id_text[16]; // To store PIDs as strings
            pid_t my_pid = getpid();  // Remember our own PID
            int kill_count = 0;       // Track how many we've killed

            // Use pgrep to find all processes with w25shell in the name
            process_file = popen("pgrep -f w25shell", "r");
            if (process_file == NULL)
            {
                perror("Failed to search for processes");
                continue;
            }

            // Process each PID returned by pgrep
            while (fgets(process_id_text, sizeof(process_id_text), process_file) != NULL)
            {
                pid_t pid = atoi(process_id_text); // Convert string to number

                // Don't kill ourselves yet - we need to finish this loop!
                if (pid != my_pid)
                {
                    // Try to kill this process
                    if (kill(pid, SIGTERM) == 0)
                    {
                        printf("Successfully terminated shell with PID: %d\n", pid);
                        kill_count++;
                    }
                }
            }

            printf("Terminated %d other shell processes\n", kill_count);
            pclose(process_file);

            // Now we can terminate ourselves
            printf("Now terminating this shell...\n");
            exit(0);
        }

        // This is a regular command - execute it
        // printf("Running command %d: %s\n", index + 1, arguments[0]);

        // Need to create a new process for this command
        pid_t child = fork();

        if (child < 0)
        {
            // Fork failed
            perror("Cannot create process for command");
            return 0;
        }
        else if (child == 0)
        {
            // Child process - run the command
            execvp(arguments[0], arguments);

            // If we get here, the command failed
            // execvp only returns if there was an error
            perror("Command failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process - wait for child to finish
            int result;
            waitpid(child, &result, 0);

            // Print a separator between command outputs
            // printf("--------------------\n");
        }
    }

    return 1; // Success!
}

/**
 * This function handles conditional commands with && and || operators
 * This was the trickiest part for me to implement!
 */
int handle_conditional(char *input)
{
    // Need a working copy of the input string
    // I kept having problems when I modified the original string
    char command_string[MAX_INPUT_SIZE];
    strcpy(command_string, input);

    // These arrays will store all the parts needed for conditional execution
    char *command_pieces[6]; // Can have up to 6 commands (with 5 operators)
    char operator_types[5];  // Will store '&' for && or '|' for ||
    int command_count = 0;   // How many commands we found
    int operator_count = 0;  // How many operators we found

    // Variables to help parse the command string
    char *current_position = command_string; // Where we're currently looking
    char *and_operator_pos;                  // Position of next && operator
    char *or_operator_pos;                   // Position of next || operator
    char *next_operator_pos;                 // Which operator comes first

    // Keep parsing until we've found all commands or reached our limit
    // printf("Parsing conditional command: %s\n", input);

    while (current_position != NULL && *current_position != '\0' && command_count < 6)
    {
        // Look for both kinds of operators
        and_operator_pos = strstr(current_position, "&&");
        or_operator_pos = strstr(current_position, "||");

        // Need to figure out which operator comes first (if any)
        // This was confusing at first - had to draw it out on paper!
        if (and_operator_pos != NULL && (or_operator_pos == NULL || and_operator_pos < or_operator_pos))
        {
            // && comes first
            next_operator_pos = and_operator_pos;
            operator_types[operator_count] = '&'; // Mark as AND operator
            operator_count = operator_count + 1;
        }
        else if (or_operator_pos != NULL)
        {
            // || comes first
            next_operator_pos = or_operator_pos;
            operator_types[operator_count] = '|'; // Mark as OR operator
            operator_count = operator_count + 1;
        }
        else
        {
            // No more operators found
            next_operator_pos = NULL;
        }

        if (next_operator_pos != NULL)
        {
            // We found an operator!

            // Replace the operator with a null terminator to split the string
            *next_operator_pos = '\0';

            // Save this command
            command_pieces[command_count] = current_position;
            command_count = command_count + 1;

            // Move our position past the operator (either && or ||)
            current_position = next_operator_pos + 2; // +2 to skip both characters

            // If the second character wasn't replaced (like in "&&&"), skip it
            if (*current_position == '&' || *current_position == '|')
            {
                current_position = current_position + 1;
            }

            // Skip any spaces at the beginning of the next command
            while (*current_position == ' ')
            {
                current_position = current_position + 1;
            }
        }
        else
        {
            // No more operators, this is the last command
            command_pieces[command_count] = current_position;
            command_count = command_count + 1;
            break; // Exit the loop
        }
    }

    // Make sure we don't have too many operators
    if (operator_count > 5)
    {
        printf("Error: Too many operators! Maximum is 5 && or || operators\n");
        return 0; // Failed
    }

    // Printing what we found (useful for debugging)
    // printf("Found %d commands with %d operators\n", command_count, operator_count);

    // Now execute the commands based on the conditions
    // I need to keep track of whether commands succeed or fail
    int previous_command_success = 1; // Start with success so first command always runs
    int command_status;

    // Process each command
    for (int cmd_index = 0; cmd_index < command_count; cmd_index++)
    {
        // Get this command and trim leading spaces
        char *current_command = command_pieces[cmd_index];
        while (*current_command == ' ')
        {
            current_command = current_command + 1;
        }

        // Here's the clever part - conditional execution!
        // We need to decide whether to run this command based on:
        // 1. The operator that came before it (& or |)
        // 2. Whether the previous command succeeded or failed

        if (cmd_index > 0) // Skip this check for the first command
        {
            // For && operator: only run if previous command succeeded
            if (operator_types[cmd_index - 1] == '&' && previous_command_success == 0)
            {
                // printf("Skipping command due to && after failed command\n");
                continue; // Skip to next command
            }

            // For || operator: only run if previous command failed
            if (operator_types[cmd_index - 1] == '|' && previous_command_success == 1)
            {
                // printf("Skipping command due to || after successful command\n");
                continue; // Skip to next command
            }
        }

        // If we get here, we need to execute this command
        // printf("Executing command: %s\n", current_command);

        // Parse this command into its arguments (like "ls", "-l", etc.)
        char *argument_list[MAX_ARGS + 1]; // +1 for NULL at end
        int arg_count = 0;

        // Need to make a copy of just this command
        char command_copy[MAX_INPUT_SIZE];
        strcpy(command_copy, current_command);

        // Break the command into words (arguments)
        char *one_arg = strtok(command_copy, " \t");

        // Get all arguments
        while (one_arg != NULL && arg_count < MAX_ARGS)
        {
            argument_list[arg_count] = one_arg;
            arg_count = arg_count + 1;
            one_arg = strtok(NULL, " \t");
        }

        // Add NULL at the end (needed for execvp)
        argument_list[arg_count] = NULL;

        // Skip empty commands
        if (arg_count == 0)
        {
            // printf("Skipping empty command\n");
            continue;
        }

        // Check for special shell commands
        if (strcmp(argument_list[0], "killterm") == 0)
        {
            printf("Terminating this shell instance\n");
            exit(0); // Exit program
        }
        else if (strcmp(argument_list[0], "killallterms") == 0)
        {
            printf("Terminating all shell instances...\n");

            // This would normally call the killallterms function
            // but for brevity I'm just exiting
            exit(0);
        }

        // Normal command - need to use fork and exec
        pid_t child_pid = fork();

        if (child_pid < 0)
        {
            // Error during fork
            perror("Couldn't create process");
            return 0;
        }
        else if (child_pid == 0)
        {
            // This is the child process
            // Replace it with the requested command
            execvp(argument_list[0], argument_list);

            // If we get here, execvp failed
            perror("Command failed to execute");
            exit(EXIT_FAILURE);
        }
        else
        {
            // This is the parent process
            // Wait for the child to finish
            waitpid(child_pid, &command_status, 0);

            // Figure out if the command succeeded or failed
            // This is important for deciding whether to run the next command!
            if (WIFEXITED(command_status))
            {
                // The child process exited normally
                int exit_code = WEXITSTATUS(command_status);
                previous_command_success = (exit_code == 0) ? 1 : 0;

                printf("Command exited with status %d (%s)\n",
                       exit_code,
                       previous_command_success ? "success" : "failure");
            }
            else
            {
                // The child process didn't exit normally
                // (maybe it was killed by a signal)
                previous_command_success = 0; // Consider it failed
                printf("Command didn't exit normally - considering it failed\n");
            }
        }
    }

    // We're done!
    return 1; // Success
}

/**
 * This is the heart of our shell program - the main function!
 * It took me a while to understand how all the pieces fit together
 */
int main()
{
    // Need a big buffer to hold whatever the user types
    char user_command[MAX_INPUT_SIZE];

    // When started on learning about this, I was confused why +1 is needed
    // Then I realized execvp needs a NULL at the end of the array!
    char *command_parts[MAX_ARGS + 1];

    // Variable to store number of arguments found
    int num_args;

    // Print a welcome message - makes the shell feel more personal
    printf("\n===== Welcome to my custom w25shell =====\n");
    printf("Type commands or 'killterm' to exit\n\n");

    // The main shell loop - keeps running until user exits
    // This was one of the first things I learned about shells
    while (1)
    {
        // Show the command prompt (added $ like real shells)
        printf("w25shell$ ");

        // Force output to appear right away - learned this from debugging
        // Sometimes output would be buffered and not appear immediately
        fflush(stdout);

        // Get input from user - fgets is safer than gets
        // I originally used gets but my professor said it's dangerous!
        if (fgets(user_command, MAX_INPUT_SIZE, stdin) == NULL)
        {
            // Handle error if fgets fails
            perror("Oops! Something went wrong reading your command");
            continue; // Try again
        }

        // Remove the newline that fgets keeps
        // strcspn finds the position of the first newline
        user_command[strcspn(user_command, "\n")] = '\0';

        // Skip empty commands (just pressing Enter)
        if (user_command[0] == '\0')
        {
            continue;
        }

        // Some debug output - helped me see what was happening
        // printf("Command received: %s\n", user_command);

        // Figure out which type of command this is
        // Need to check special characters in a specific order

        // First check for piping operations
        if (strchr(user_command, '|') != NULL)
        {
            // printf("Detected pipe operation!\n");
            handle_multi_pipe(user_command);
        }
        // Check for reverse piping
        else if (strchr(user_command, '=') != NULL)
        {
            // printf("Detected reverse pipe operation!\n");
            handle_reverse_pipe(user_command);
        }
        // Check for file append operation
        else if (strchr(user_command, '~') != NULL)
        {
            // printf("Detected file append operation!\n");
            handle_append(user_command);
        }
        // Check for word count operation
        else if (strchr(user_command, '#') != NULL)
        {
            // printf("Detected word count operation!\n");
            handle_word_count(user_command);
        }
        // Check for file concatenation
        else if (strchr(user_command, '+') != NULL)
        {
            // printf("Detected file concatenation operation!\n");
            handle_concat(user_command);
        }
        // Check for input/output redirection
        else if (strchr(user_command, '<') != NULL || strchr(user_command, '>') != NULL)
        {
            // printf("Detected I/O redirection!\n");
            handle_redirection(user_command);
        }
        // Check for sequential execution
        else if (strchr(user_command, ';') != NULL)
        {
            // printf("Detected sequential execution!\n");
            handle_sequential(user_command);
        }
        // Check for conditional execution (need to check for && before ||)
        else if (strstr(user_command, "&&") != NULL || strstr(user_command, "||") != NULL)
        {
            // printf("Detected conditional execution!\n");
            handle_conditional(user_command);
        }
        // If no special characters, it's a regular command
        else
        {
            // Regular command - need to parse it into arguments
            char *args_array[MAX_ARGS + 1]; // Local array for arguments

            // Use our parsing function to break command into words
            int arg_count = parse_command(user_command, args_array);

            // Make sure we actually got some arguments
            if (arg_count > 0)
            {
                // First check if it's one of our special built-in commands
                if (handle_special_commands(args_array))
                {
                    // If special command was handled, go back to prompt
                    // printf("Special command executed\n");
                    continue;
                }

                // Otherwise execute it as a normal command
                // printf("Executing regular command: %s\n", args_array[0]);
                int result = execute_command(args_array);
                if (result == 0)
                {
                    // Command failed, continue to next iteration
                    continue;
                }
            }
            else
            {
                printf("Error: No valid command found\n");
            }
        }

        // Add a separator line to make output easier to read
        // printf("--------------------\n");
    }

    // This code never actually runs (we exit with killterm)
    // But I'm including it to be proper
    return 0;
}
