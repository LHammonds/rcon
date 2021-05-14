# NAME
rcon - Execute rcon commands

# SYNOPSIS
rcon <-f "filename.ini"> [-a IPNumber] [-p Port] <command>

# DESCRIPTION
Linux command line utility to send rcon commands.

Any command line parameter will take precedence over the corresponding option in the INI file.

# OPTIONS
**-f -a -p** and **command** are all necessary.  However, only **-f** and **command** are required as command-line arguments.  If address and port are in the file, it is not required to pass them as arguments.

**-?** Shows the usage syntax and quits.

**-v** Enables verbose mode which means more detailed messages.

**-f "filename.ini"** Location to .ini file containing at least  the  password.

**-a IPNumber** IP Address of the rcon server. This option is not needed if added to the .ini file.

**-p Port** Port on the rcon server. This option is not needed if added to the .ini file.

**command** rcon command to send to the rcon server.

# FILE FORMAT
```
[rcon]
password=YOUR_RCON_PASSWORD
ipaddress=127.0.0.1
port=27015
```
# EXAMPLES
Run a DoExit command:
```
rcon -f "/etc/rcon.ini" -a 127.0.0.1 -p 27015 "DoExit"
```

Run a DoExit command and use the address and port defined in ini file:
```
rcon -f "/etc/rcon.ini" "DoExit"
```

Run a Broadcast message command using an island-instance ini file:
```
rcon -f "/etc/rcon-island.ini" "Broadcast About to save the world!"
```

Run a SaveWorld command using a ragnarok-instance ini file:
```
rcon -f "/etc/rcon-ragnarok.ini" "SaveWorld"
```

# EXIT CODES
```
0 = Good
1 = Unknown option
2 = Missing file parameter
3 = Missing command parameter
4 = Missing address parameter/config
5 = Missing port parameter/config
6 = Invalid file or format
7 = Connect failed
8 = Error sending password
9 = Could not authenticate
10 = Command send error
```

# SEE ALSO
<https://www.ryanschulze.net/archives/1052>

# BUGS
No known bugs.

# AUTHOR
- Version 1.0 - [ASY]Zyrain
- Version 1.1 - LHammonds
- Man pages - LHammonds
