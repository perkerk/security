#### Notepad Helper

An NT service that detects when an interactive user has logged on to the system and launches Notepad.exe into the user's session in the security context of the logged in user.

NOTE: Full support for event logging isn't implemented, this writes messages to the default Application log. A message file should be compiled into the resources and used for correct logging.

#### Basic admin commands:

`sc create "Notepad Helper" binPath= "C:\path\NotepadService.exe"`

`sc start "Notepad Helper"`

`sc stop "Notepad Helper"`

`sc delete "Notepad Helper"`

#### PS:

Add a service argument to use notepad++ or other application as an alternate start editor




