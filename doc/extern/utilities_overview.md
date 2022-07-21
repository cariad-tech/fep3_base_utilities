<!--
  Copyright @ 2021 VW Group. All rights reserved.
  
      This Source Code Form is subject to the terms of the Mozilla
      Public License, v. 2.0. If a copy of the MPL was not distributed
      with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
  
  If it is not possible or desirable to put the notice in a particular file, then
  You may include the notice in a location (such as a LICENSE file in a
  relevant directory) where a recipient would be likely to look for such a notice.
  
  You may add additional accurate notices of copyright ownership.
  
  -->

# FEP Base Utilities

## FEP Control
![](fep_control_help.png)
&nbsp;

## Use FEP Control for Logging
The FEP Control tool is able to start monitoring the log messages of the system.
To use FEP Control as a monitor you need to add the following command:
    
        fep> startMonitoring demo_system
    
![](logging_monitor_console.png)
&nbsp;

##Use FEP Control for shutting down a single participant
The FEP Control tool is able to send the participant state machine commands to a participant.
One command is to shutdown the participant:
    
        fep> setParticipantState demo_system demo_cpp_receiver shutdowned
    
![](single_participant_shutdown_command.png)
After sending this command, the participant is not reachable anymore because its process was exited
in a clean and well defined behavior.
&nbsp;

## Use FEP Control to observe a participants RPC Interfaces
The FEP Control tool is able to observe the participants RPC info interfaces.
![](observing_participant.png)
&nbsp;

## Use FEP Control to call RPC services
The FEP Control tool is able to call RPC services of participants directly.

Call RPC service without arguments:

        fep> callRPC my_system my_participant participant_info participant_info.arya.fep3.iid getName

Response exmaple:

        {
            "id" : 1,
            "jsonrpc" : "2.0",
            "result" : "my_participant"
        }

Call RPC service with arguments, for example `getLoggerFilter`:

        fep> callRPC my_system my_participant logging_service logging_service.arya.fep3.iid getLoggerFilter '{"logger_name":"participant"}'

Response exmaple:

        {
            "id" : 1,
            "jsonrpc" : "2.0",
            "result" :
            {
                    "enable_sinks" : "console",
                    "severity" : 1
            }
        }

If setting logger severity by calling `setLoggerFilter`, the request arguments have to be given as a JSON string:

        fep> callRPC my_system my_participant logging_service logging_service.arya.fep3.iid setLoggerFilter '{"logger_name":"participant","severity":1,"enable_sinks":"console,rpc"}'

Response exmaple:

        {
                "id" : 1,
                "jsonrpc" : "2.0",
                "result" : 0
        }
&nbsp;

##Use FEP Control for batch execution
If the user wants to run multiple commands from a script, it would not be efficient to run a fep_control process with autodiscovery for each command.
In this case, the piping feature of FEP Control tool may come in handy, i. e. it can process multiple commands sequentially if the user simply feeds
them to it with a pipe:
    
        type run_the_system.txt | fep_control.exe
    
or in Linux
    
        cat commands.txt | fep_control
    
or simply
    
        fep_control < commands.txt
    
Provided we have
![](commands_file.png)
the result of the piping is the following:
![](batch_execution.png)
