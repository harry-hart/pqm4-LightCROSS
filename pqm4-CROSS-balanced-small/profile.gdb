# --- Poor Man's Profiler GDB Script ---

# --- Parameters ---
# Adjust these values as needed
# Total number of PC samples to collect
set $NUM_SAMPLES = 10
# Time between samples in seconds (e.g., 0.01 = 10ms)
set $SAMPLE_INTERVAL = 0.01
# GDB server address and port (Default for OpenOCD)
set $GDB_SERVER_ADDR = "localhost:3333"
# Output file for PC samples
set $LOG_FILE = "pc_samples.log"

# --- GDB Setup ---
# Prevent GDB from stopping output with --Type <CR> to continue--
set pagination off
# Prevent GDB from asking "Are you sure you want to quit?"
set confirm off
# Configure logging: overwrite the file each time, log to file AND screen
set logging file $LOG_FILE
set logging overwrite on
# Set to 'on' if you ONLY want output in the file
set logging redirect off 
# Debugging
set verbose on

# --- Connect to Target ---
printf "Connecting to GDB server at %s...\n", $GDB_SERVER_ADDR
# Use 'target extended-remote' if 'target remote' causes issues, but 'remote' is standard
eval "target extended-remote %s", $GDB_SERVER_ADDR
echo Connected!\n

# --- Handle Initial Halt ---
# GDB usually halts the target upon connection. Resume it immediately.
echo Resuming target to start profiling...\n
continue
# Small delay to ensure target is running before first sample attempt
shell sleep 0.1

# --- Profiling Loop Definition ---
# We define a command to contain the loop logic
define profile_loop
  set $count = 0
  # Turn logging on specifically for capturing PC values inside the loop
  set logging on
  echo Starting profiling loop for \
  echo $NUM_SAMPLES
  echo  samples...\n
  while $count < $NUM_SAMPLES
    # Wait for the specified interval (uses host OS sleep)
    shell sleep $SAMPLE_INTERVAL

    # Interrupt the target (halt execution)
    # Use 'interrupt -async' if your GDB supports it and regular interrupt hangs
    interrupt

    # Check if target is actually halted (basic check)
    # GDB commands often implicitly check state. If interrupt fails badly,
    # subsequent commands might error out, or the script might hang.
    # More robust checking is complex and GDB version dependent.

    # Log the Program Counter
    # This logs the full line, e.g., "pc             0x8001234 <symbol+offset>"
    # The file $LOG_FILE will contain these lines.
    info reg pc

    # Resume target execution
    continue

    # Increment sample counter
    set $count = $count + 1

    # Optional: Print progress to the GDB console every 100 samples
    # This might slightly increase the time per sample due to I/O
    if $count % 100 == 0
       echo Sample \
       echo $count
       echo / \
       echo $NUM_SAMPLES
       echo  collected...\n
    end

  end
  # Turn logging off after the loop
  set logging off
  echo \nProfiling loop finished. \
  echo $NUM_SAMPLES
  echo  samples collected in \
  echo $LOG_FILE
  echo \n
end
# --- End of Loop Definition ---

# --- Execute Profiling ---
profile_loop

# --- Cleanup ---
# Halt the target after profiling is done (optional, but good practice)
echo Halting target after profiling.\n
interrupt

# Detach from the target and quit GDB
echo Detaching and quitting GDB.\n
detach
quit

# --- End of Script ---
