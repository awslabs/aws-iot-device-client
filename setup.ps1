# Function to set console colors
<#
.SYNOPSIS
Sests console text back and foreground colors to the specified values

.PARAMETER foregroundColor
foreground color code

.PARAMETER backgroundColor
background color code
#>
function Set-ConsoleColor {
    param(
        [System.ConsoleColor]$foregroundColor,
        [System.ConsoleColor]$backgroundColor
    )
    $Host.UI.RawUI.ForegroundColor = $foregroundColor
    $Host.UI.RawUI.BackgroundColor = $backgroundColor
}

<#
.SYNOPSIS
Prints provided prompts, waits for the input and returns it

.PARAMETER strPrompt
test prompt to be shown
#>
function Read-Text {
    param(
        [System.String]$strPrompt
    )
    Set-ConsoleColor -foregroundColor Magenta -backgroundColor Black
    $tmpEntry =  Read-Host $strPrompt
    Set-ConsoleColor -foregroundColor White -backgroundColor Black
    return $tmpEntry
}
<#
.SYNOPSIS
Writes string to standard output using font color designated for regular output text messages

.PARAMETER strPrompt
string to be printed
#>
function Write-Text {
    param(
        [System.String]$strPrompt
    )
    Write-Host $strPrompt -ForegroundColor Green
}
<#
.SYNOPSIS
Writes string to standard output using font color designated for warning messages

.PARAMETER strPrompt
string to be printed
#>
function Write-Warning {
    param(
        [System.String]$strPrompt
    )
    Write-Host $strPrompt -ForegroundColor Red
}

<#
.SYNOPSIS
Return system variable value

.DESCRIPTION
Read system variable by the provided name and return its' value

.PARAMETER strName
system variable name
#>
function Get-EnvVariable {
    param(
        [System.String]$strName
    )
    return Get-ChildItem Env:$strName | Select-Object -ExpandProperty Value
}

<#
.SYNOPSIS
Sets permissions to the specified file object

.DESCRIPTION
Converts Linux-style file permissions (e.g. 0x754) into Windows ACL and sets them to the specified file object.
owner - windows user under which the script is executed
group - built-in Users group
others - built-in Everyone group

.PARAMETER permissions
Linux-style permissions

.PARAMETER filename
full path to the object
#>
function Set-LinuxStylePermissions {
    param (
        [System.Int16]$permissions,
        [System.String]$filename
    )

    if (-Not (Test-Path $filename)) {
        return
    }

    # Convert Linux-style permissions to Windows-style permissions
    $userSystemRights = [System.Security.AccessControl.FileSystemRights]::None
    $groupSystemRights = [System.Security.AccessControl.FileSystemRights]::None
    $otherSystemRights = [System.Security.AccessControl.FileSystemRights]::None

    if ($permissions -band 0x400) { $userSystemRights = $userSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Read }
    if ($permissions -band 0x200) { $userSystemRights = $userSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Write }
    if ($permissions -band 0x100) { $userSystemRights = $userSystemRights -bor [System.Security.AccessControl.FileSystemRights]::ExecuteFile }
    if ($permissions -band 0x40) {  $groupSystemRights = $groupSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Read }
    if ($permissions -band 0x20) {  $groupSystemRights = $groupSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Write }
    if ($permissions -band 0x10) {  $groupSystemRights = $groupSystemRights -bor [System.Security.AccessControl.FileSystemRights]::ExecuteFile }
    if ($permissions -band 0x4) {   $otherSystemRights = $otherSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Read }
    if ($permissions -band 0x2) {   $otherSystemRights = $otherSystemRights -bor [System.Security.AccessControl.FileSystemRights]::Write }
    if ($permissions -band 0x1) {   $otherSystemRights = $otherSystemRights -bor [System.Security.AccessControl.FileSystemRights]::ExecuteFile }

    # Get the current ACL of the file
    $acl = Get-Acl -Path $filename

    # Disable inheritance
    $acl.SetAccessRuleProtection($true, $false)

    $currentUserName = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
    $acl.SetOwner([System.Security.Principal.NTAccount]$currentUserName)

    # Use these permissions set to clear permissions prior to setting up permissions, which have been provided
    $removeRights = $removeRights -bor [System.Security.AccessControl.FileSystemRights]::Read
    $removeRights = $removeRights -bor [System.Security.AccessControl.FileSystemRights]::Write
    $removeRights = $removeRights -bor [System.Security.AccessControl.FileSystemRights]::ExecuteFile

    # Set ACLs for current user
    if ($userSystemRights -ne [System.Security.AccessControl.FileSystemRights]::None) {
        $allowRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            $currentUserName, 
            $userSystemRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        $aclRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            $currentUserName, 
            $removeRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        # Remove rules prior to setting up new ones
        $acl.RemoveAccessRule($aclRule) | Out-Null
    
        $acl.AddAccessRule($allowRule)
    }

    # Set ACLs for USers group
    if ($groupSystemRights -ne [System.Security.AccessControl.FileSystemRights]::None) {
        $allowRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            "BUILTIN\Users", 
            $groupSystemRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        $aclRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            "BUILTIN\Users", 
            $removeRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        # Remove rules prior to setting up new ones
        $acl.RemoveAccessRule($aclRule) | Out-Null

        $acl.AddAccessRule($allowRule)
    }

    # Set ACLs for Everyone
    if ($otherSystemRights -ne [System.Security.AccessControl.FileSystemRights]::None) {
        $allowRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            "Everyone", 
            $otherSystemRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        $aclRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
            "Everyone", 
            $removeRights, 
            [System.Security.AccessControl.InheritanceFlags]::None,
            [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Allow)

        # Remove rules prior to setting up new ones
        $acl.RemoveAccessRule($aclRule) | Out-Null
    
        $acl.AddAccessRule($allowRule)
    }

    # Add Full Control permissions to administrators group
    $adminRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
        "BUILTIN\Administrators", 
        [System.Security.AccessControl.FileSystemRights]::FullControl, 
        [System.Security.AccessControl.InheritanceFlags]::None,
        [System.Security.AccessControl.PropagationFlags]::None,
        [System.Security.AccessControl.AccessControlType]::Allow)
    $acl.AddAccessRule($adminRule)

    # Set the modified ACL back to the file
    Set-Acl -Path $filename -AclObject $acl | Out-Null
}

if ((Get-EnvVariable "USERNAME") -eq "") {
    Write-Text "WARNING: Only run this setup script as system user if you plan to run the AWS IoT Device Client as if you plan to run the AWS IoT Device Client as a service. Otherwise, you should run this script as \
  the user that will execute the client."
}

# Build Configuration File
$BUILD_CONFIG = Read-Text "Do you want to interactively generate a configuration file for the AWS IoT Device Client? [y/n]"

if ((Get-EnvVariable "USERPROFILE") -eq "") {
    $OUTPUT_DIR = (Get-EnvVariable "SYSTEMROOT") + "\.aws-iot-device-client\"
} else {
    $OUTPUT_DIR = (Get-EnvVariable "USERPROFILE") + "\.aws-iot-device-client\"
}

# Config Defaults
$CONF_OUTPUT_PATH = $OUTPUT_DIR + "aws-iot-device-client.conf"
$HANDLER_DIR = $OUTPUT_DIR + "jobs\"
$PUBSUB_DIR = $OUTPUT_DIR + "pubsub\"
$PUB_FILE = $PUBSUB_DIR + "publish-file.txt"
$SUB_FILE = $PUBSUB_DIR + "subscribe-file.txt"
$DD_INTERVAL = 300
$LOG_TYPE = "STDOUT"
$LOG_LEVEL = "DEBUG"
$LOGS_DIR = (Get-EnvVariable "LOCALAPPDATA") + "\aws-iot-device-client\logs\"
$LOG_LOCATION = $LOGS_DIR + "aws-iot-device-client.log"
$SDK_LOGS_ENABLED = $false
$SDK_LOG_LEVEL = "TRACE"
$SDK_LOG_LOCATION = $LOGS_DIR + "sdk.log"
$SAMPLE_SHADOW_DIR = $OUTPUT_DIR + "sample-shadow\"
$SAMPLE_SHADOW_INPUT_FILE = $SAMPLE_SHADOW_DIR + "shadow-input.txt"
$SAMPLE_SHADOW_OUTPUT_FILE = $SAMPLE_SHADOW_DIR + "shadow-output.txt"

if ($BUILD_CONFIG -ieq "y") {
    $CONFIGURED = 0
    while ($CONFIGURED -ne 1) {
        $ENDPOINT = (Read-Text "Specify AWS IoT endpoint to use")
        $CERT = (Read-Text "Specify path to public PEM certificate")
        $PRIVATE_KEY = (Read-Text "Specify path to private key")
        $ROOT_CA = (Read-Text "Specify path to ROOT CA certificate")
        $THING_NAME = (Read-Text "Specify thing name (Also used as Client ID)")

        ### Logging Config ###
        $CONFIGURE_LOGS = (Read-Text "Would you like to configure the logger? [y/n]")
        if ($CONFIGURE_LOGS -ieq "y") {
            $logLevels = @("DEBUG", "INFO", "WARN". "ERROR")            
            $LOG_LEVEL_TMP = (Read-Text ("Specify desired log level [DEBUG/INFO/WARN/ERROR]. Default is " + $LOG_LEVEL))
            if ($LOG_LEVEL_TMP -ne "" -and $logLevels -contains $LOG_LEVEL_TMP) {
                $LOG_LEVEL = $LOG_LEVEL_TMP
            }

            $logTypes = @("STDOUT", "FILE")
            $LOG_TYPE_TMP = (Read-Text ("Specify log type [STDOUT for standard output, FILE for file]. Default is " + $LOG_TYPE))
            if ($LOG_TYPE_TMP -ne "" -and $logTypes -contains $LOG_TYPE_TMP) {
                $LOG_TYPE = $LOG_TYPE_TMP
            }

            if ($LOG_TYPE -eq "FILE") {
                $LOG_LOCATION_TMP = Read-Text ("Specify path to desired log file (if no path is provided, will default to '" + $LOG_LOCATION + "'")
                if ($LOG_LOCATION_TMP -ine "") {
                    $LOG_LOCATION = $LOG_LOCATION_TMP
                }
            }
        }
        ### SDK Logging Config ###
        $CONFIGURE_SDK_LOGS = Read-Text "Would you like to configure the SDK logging? [y/n]"
        if ($CONFIGURE_SDK_LOGS -ieq "y") {
            $SDK_LOGS_ENABLED = $true
            $logLevels = @("TRACE", "DEBUG". "INFO", "WARN", "ERROR", "FATAL")
            $SDK_LOG_LEVEL_TMP = Read-Text ("Specify desired SDK log level [TRACE/DEBUG/INFO/WARN/ERROR/FATAL]. Default is " + $SDK_LOG_LEVEL)
            if ($SDK_LOG_LEVEL_TMP -ne "" -and $logLevels -contains $SDK_LOG_LEVEL_TMP) {
                $SDK_LOG_LEVEL = $SDK_LOG_LEVEL_TMP
            }

            $SDK_LOG_LOCATION_TMP = Read-Text ("Specify path to desired SDK log file (if no path is provided, will default to " + $SDK_LOG_LOCATION)
            if ($SDK_LOG_LOCATION_TMP -ine "") {
                $SDK_LOG_LOCATION = $SDK_LOG_LOCATION_TMP
            }
        }
    
        ### Jobs Config ###
        $JOBS_ENABLED = $false
        $ENABLE_JOBS = Read-Text "Enable Jobs feature? [y/n]"
        if ($ENABLE_JOBS -ieq "y") {
            $JOBS_ENABLED = $true
            $HANDLER_DIR_TEMP = Read-Text ("Specify absolute path to Job handler directory (if no path is provided, will default to '" + $HANDLER_DIR + "'")
            if ($HANDLER_DIR_TEMP -ne "") {
                $HANDLER_DIR = $HANDLER_DIR_TEMP
            }
        } else {
            $JOBS_ENABLED = $false
        }
    
        ### ST Config ###
        $ENABLE_ST = Read-Text "Enable Secure Tunneling feature? [y/n]"
        if ($ENABLE_ST -ieq "y") {
            $ST_ENABLED = $true
        } else {
            $ST_ENABLED = $false
        }

         ### DD Config ###
         $ENABLE_DD = Read-Text "Enable Device Defender feature? [y/n]"
        if ($ENABLE_DD -ieq "y") {
            $DD_ENABLED = $true
            $INTERVAL_TEMP = Read-Text "Specify an interval for Device Defender in seconds (default is 300)"
            if ($INTERVAL_TEMP -ne "") {
                $DD_INTERVAL = [int16]$INTERVAL_TEMP
            }
        } else {
            $DD_ENABLED = $false
        }
     
        ### FP Config ###
        $FP_ENABLED = $false
        $ENABLE_FP = Read-Text "Enable Fleet Provisioning feature? [y/n]"
        if ($ENABLE_FP -ieq "y") {
            $FP_ENABLED = $true
            $TEMPLATE_NAME_TEMP = Read-Text "Specify Fleet Provisioning Template name you want to use for Provisioning your device"
            if ($TEMPLATE_NAME_TEMP -ne "") {
                $FP_TEMPLATE_NAME = $TEMPLATE_NAME_TEMP
            }
            $TEMPLATE_PARAMS_TEMP = Read-Text "Specify Fleet Provisioning Template parameters you want to use for Provisioning your device"
            if ($TEMPLATE_PARAMS_TEMP -ne "") {
                $FP_TEMPLATE_PARAMS = $TEMPLATE_PARAMS_TEMP
            } else {
                $FP_TEMPLATE_PARAMS = "{}"
            }
            $CSR_FILE_TEMP = Read-Text "Specify absolute path to Certificate Signing Request (CSR) file used for creating new certificate while provisioning device by keeping private key secure"
            if ($CSR_FILE_TEMP -ne "") {
                $FP_CSR_FILE = $CSR_FILE_TEMP
            }
    
            $DEVICE_KEY_TEMP = Read-Text "Specify absolute path to Device Private Key file"
            if ($DEVICE_KEY_TEMP -ne "") {
                $FP_DEVICE_KEY = $DEVICE_KEY_TEMP
            }
        } else {
            $FP_ENABLED = $false
        }
        
        ### PUBSUB Config ###
        $PUBSUB_ENABLED = $false
        $ENABLE_PUBSUB = Read-Text "Enable Pub Sub sample feature? [y/n]"
        if ($ENABLE_PUBSUB -ieq "y") {
            $PUBSUB_ENABLED = $true
            $PUB_TOPIC = Read-Text "Specify a topic for the feature to publish to"
            $PUB_FILE_TMP = Read-Text ("Specify the path of a file for the feature to publish (if no path is provided, will default to '" + $PUB_FILE + "')")
            if ($PUB_FILE_TMP -ne "") {
                $PUB_FILE = $PUB_FILE_TMP
            }

            $SUB_TOPIC = Read-Text "Specify a topic for the feature to subscribe to"
            $SUB_FILE_TMP = Read-Text ("Specify the path of a file for the feature to write to (if no path is provided, will default to '" + $SUB_FILE + "')")
            if ($SUB_FILE_TMP -ne "") {
                $SUB_FILE = $SUB_FILE_TMP
            }
        } else {
            $PUBSUB_ENABLED = $false
        }
    
        ### ConfigShadow Config ###
        $CONFIG_SHADOW_ENABLED = $false
        $ENABLE_CONFIG_SHADOW = Read-Text "Enable Config Shadow feature? [y/n]"
        if ($ENABLE_CONFIG_SHADOW -ieq "y") {
            $CONFIG_SHADOW_ENABLED = $true
        } else {
            $CONFIG_SHADOW_ENABLED = $false
        }
        
        ### SampleShadow Config ###
        $SAMPLE_SHADOW_ENABLED = $false
        $ENABLE_SAMPLE_SHADOW = Read-Text "Enable Sample Shadow feature? [y/n]"
        if ($ENABLE_SAMPLE_SHADOW -ieq "y") {
            $SAMPLE_SHADOW_ENABLED = $true
            $SAMPLE_SHADOW_NAME = Read-Text "Specify a shadow name for the feature to create or update"

            $SAMPLE_SHADOW_INPUT_FILE_TMP = Read-Text ("Specify the path of a file for the feature to read from (if no path is provided, will default to '" + $SAMPLE_SHADOW_INPUT_FILE + "')")
            if ($SAMPLE_SHADOW_INPUT_FILE_TMP -ne "") {
                $SAMPLE_SHADOW_INPUT_FILE = $SAMPLE_SHADOW_INPUT_FILE_TMP
            }

            $SAMPLE_SHADOW_OUTPUT_FILE_TMP = Read-Text ("Specify a the path of a file for the feature to write shadow document to (if no path is provided, will default to '" + $SAMPLE_SHADOW_OUTPUT_FILE + "')")
            if ($SAMPLE_SHADOW_OUTPUT_FILE_TMP -ne "") {
                $SAMPLE_SHADOW_OUTPUT_FILE = $SAMPLE_SHADOW_OUTPUT_FILE_TMP
            }
        } else {
            $SAMPLE_SHADOW_ENABLED = $false
        }

        ### Prepare Config File content
        $CONFIG_OUTPUT= [ordered]@{
            "endpoint" = $ENDPOINT
            "cert" = $CERT
            "key" = $PRIVATE_KEY
            "root-ca" = $ROOT_CA
            "thing-name" = $THING_NAME
            "logging" = [ordered]@{
                "level" =	$LOG_LEVEL
                "type" = $LOG_TYPE
                "file" = $LOG_LOCATION
                "enable-sdk-logging" =	$SDK_LOGS_ENABLED
                "sdk-log-level" =	$SDK_LOG_LEVEL
                "sdk-log-file" = $SDK_LOG_LOCATION
            }
            "jobs" = [ordered]@{
                "enabled" =	$JOBS_ENABLED
                "handler-directory" = $HANDLER_DIR
            }
            "tunneling" =	@{
                "enabled" = $ST_ENABLED
            }
            "device-defender" =	[ordered]@{
                "enabled" =	$DD_ENABLED
                "interval" = $DD_INTERVAL
            }
            "fleet-provisioning" = [ordered]@{
                "enabled" =	$FP_ENABLED
                "template-name" = $FP_TEMPLATE_NAME
                "template-parameters" = $FP_TEMPLATE_PARAMS
                "csr-file" = $FP_CSR_FILE
                "device-key" = $FP_DEVICE_KEY
            }
            "samples" = [ordered]@{
                "pub-sub" = [ordered]@{
                    "enabled" = $PUBSUB_ENABLED
                    "publish-topic" = $PUB_TOPIC
                    "publish-file" = $PUB_FILE
                    "subscribe-topic" = $SUB_TOPIC
                    "subscribe-file" = $SUB_FILE
                }
            }
            "config-shadow" = @{
                "enabled" =	$CONFIG_SHADOW_ENABLED
            }
            "sample-shadow" = [ordered]@{
                "enabled" = $SAMPLE_SHADOW_ENABLED
                "shadow-name" = $SAMPLE_SHADOW_NAME
                "shadow-input-file" = $SAMPLE_SHADOW_INPUT_FILE
                "shadow-output-file" = $SAMPLE_SHADOW_OUTPUT_FILE
            }
        }
    
        $JSONDATA = $CONFIG_OUTPUT | ConvertTo-Json

        Write-Text $JSONDATA
        $GOOD_TO_GO = Read-Text ("Does the following configuration appear correct? If yes, configuration will be written to '" + $CONF_OUTPUT_PATH + "' [y/n]")
        if ($GOOD_TO_GO -ieq "y") {
            $CONFIGURED = 1
           
            ### Create config file folder
            $CONFIG_LOCATION_PATH = Split-Path -Path $CONF_OUTPUT_PATH -Parent            
            if (-Not (Test-Path $CONFIG_LOCATION_PATH)) {
                New-Item -Path $CONFIG_LOCATION_PATH -ItemType Directory -Force | Out-Null
            }

            $CONFIG_OUTPUT | ConvertTo-Json | Out-File -FilePath $CONF_OUTPUT_PATH -Encoding UTF8

            Set-LinuxStylePermissions -permissions 0x745 -filename $CONFIG_LOCATION_PATH
            Set-LinuxStylePermissions -permissions 0x640 -filename $CONF_OUTPUT_PATH

            Write-Text ("Configuration has been successfully written to '" + $CONF_OUTPUT_PATH + "'")

            ### Create a Pub-Sub feature folder
            if ($PUBSUB_ENABLED -ieq $true) {
                Write-Text "Creating default pubsub directory..."
                if (-Not (Test-Path $PUBSUB_DIR)) {
                    New-Item -Path $PUBSUB_DIR -ItemType Directory -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $PUBSUB_DIR
            }
          
            ### Move Sample Jobs if Job feature is enabled
            if ($JOBS_ENABLED -ieq $true) {
                $COPY_HANDLERS = Read-Text ("Do you want to copy the sample job handlers to the specified handler directory ('" + $HANDLER_DIR + "')? [y/n]")      
                if ($COPY_HANDLERS -ieq "y") {
                    if (-Not (Test-Path $HANDLER_DIR)) {
                        New-Item -Path $HANDLER_DIR -ItemType Directory -Force | Out-Null
                    }
                    Set-LinuxStylePermissions -permissions 0x700 -filename $HANDLER_DIR
        
                    Copy-Item -Path ".\sample-job-handlers\win32\*" -Destination $HANDLER_DIR -Recurse -Force

                    # Ser permissions
                    $jobFiles = Get-ChildItem -Path $HANDLER_DIR -File
                    foreach ($jobFile in $jobFiles) {
                        Set-LinuxStylePermissions -permissions 0x700 -filename $jobFile
                    }
                }
            }

            ### Create Log File Location
            if ($LOG_LOCATION -ne "") {
                Write-Text "Creating log directory..."
                $LOG_LOCATION_PATH = Split-Path -Path $LOG_LOCATION -Parent
                if (Test-Path $LOG_LOCATION_PATH) {
                    Write-Text ("Folder '" + $LOG_LOCATION_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $LOG_LOCATION_PATH + "' folder")
                    New-Item -ItemType Directory $LOG_LOCATION_PATH -Force | Out-Null
                }

                Set-LinuxStylePermissions -permissions 0x745 -filename $LOG_LOCATION_PATH

                ## Create Log file & set permissions
                if (Test-Path $LOG_LOCATION) {
                    Write-Text ("Application log file '" + $LOG_LOCATION + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $LOG_LOCATION + "' file")
                    New-Item -Path $LOG_LOCATION -ItemType File -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x600 -filename $LOG_LOCATION
            }

            ### Create SDK Log File Location
            if ($SDK_LOG_LOCATION -ne "") {
                Write-Text "Creating SDK log directory..."
                $SDK_LOG_LOCATION_PATH = Split-Path -Path $SDK_LOG_LOCATION -Parent
                if (Test-Path $SDK_LOG_LOCATION_PATH) {
                    Write-Text ("Folder '" + $SDK_LOG_LOCATION_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SDK_LOG_LOCATION_PATH + "' folder")
                    New-Item -ItemType Directory $SDK_LOG_LOCATION_PATH -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $SDK_LOG_LOCATION_PATH

                ## Create SDK Log file & set permissions
                if (Test-Path $SDK_LOG_LOCATION) {
                    Write-Text ("SDK log file '" + $SDK_LOG_LOCATION + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SDK_LOG_LOCATION + "' file")
                    New-Item -Path $SDK_LOG_LOCATION -ItemType File -Force | Out-Null
                }                
                Set-LinuxStylePermissions -permissions 0x600 -filename $SDK_LOG_LOCATION
            }

            ### Create Sample-Shadow files
            if ($SAMPLE_SHADOW_ENABLED -eq $true) {
                Write-Text "Creating Sample-Shadow Input File Location..."
                $SAMPLE_SHADOW_PATH = Split-Path -Path $SAMPLE_SHADOW_INPUT_FILE -Parent
                if (Test-Path $SAMPLE_SHADOW_PATH) {
                    Write-Text ("Folder '" + $SAMPLE_SHADOW_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SAMPLE_SHADOW_PATH + "' folder")
                    New-Item -ItemType Directory $SAMPLE_SHADOW_PATH -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $SDK_LOG_LOCATION_PATH
                
                if (Test-Path $SAMPLE_SHADOW_INPUT_FILE) {
                    Write-Text ("Sample shadow input file '" + $SAMPLE_SHADOW_INPUT_FILE + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SAMPLE_SHADOW_INPUT_FILE + "' file")
                    New-Item -Path $SAMPLE_SHADOW_INPUT_FILE -ItemType File -Force | Out-Null
                }                
                Set-LinuxStylePermissions -permissions 0x600 -filename $SAMPLE_SHADOW_INPUT_FILE


                Write-Text "Creating Sample-Shadow Output File Location..."
                $SAMPLE_SHADOW_PATH = Split-Path -Path $SAMPLE_SHADOW_OUTPUT_FILE -Parent
                if (Test-Path $SAMPLE_SHADOW_PATH) {
                    Write-Text ("Folder '" + $SAMPLE_SHADOW_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SAMPLE_SHADOW_PATH + "' folder")
                    New-Item -ItemType Directory $SAMPLE_SHADOW_PATH -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $SAMPLE_SHADOW_PATH

                if (Test-Path $SAMPLE_SHADOW_OUTPUT_FILE) {
                    Write-Text ("Sample shadow input file '" + $SAMPLE_SHADOW_OUTPUT_FILE + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SAMPLE_SHADOW_OUTPUT_FILE + "' file")
                    New-Item -Path $SAMPLE_SHADOW_OUTPUT_FILE -ItemType File -Force | Out-Null
                }                
                Set-LinuxStylePermissions -permissions 0x600 -filename $SAMPLE_SHADOW_OUTPUT_FILE
            }

            ### Create Pub-Sub files
            if ($PUBSUB_ENABLED -eq $true) {
                Write-Text "Creating Pub-Sub Subscribe / Output File Location..."
                $PUBSUB_PATH = Split-Path -Path $SUB_FILE -Parent
                if (Test-Path $PUBSUB_PATH) {
                    Write-Text ("Folder '" + $PUBSUB_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $PUBSUB_PATH + "' folder")
                    New-Item -ItemType Directory $PUBSUB_PATH -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $PUBSUB_PATH
                
                if (Test-Path $SUB_FILE) {
                    Write-Text ("Subscribe / Input file '" + $SUB_FILE + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $SUB_FILE + "' file")
                    New-Item -Path $SUB_FILE -ItemType File -Force | Out-Null
                }                
                Set-LinuxStylePermissions -permissions 0x600 -filename $SUB_FILE


                Write-Text "Creating Pub-Sub Publish / Input File Location..."
                $PUBSUB_PATH = Split-Path -Path $PUB_FILE -Parent
                if (Test-Path $PUBSUB_PATH) {
                    Write-Text ("Folder '" + $PUBSUB_PATH + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $PUBSUB_PATH + "' folder")
                    New-Item -ItemType Directory $PUBSUB_PATH -Force | Out-Null
                }
                Set-LinuxStylePermissions -permissions 0x745 -filename $PUBSUB_PATH

                if (Test-Path $PUB_FILE) {
                    Write-Text ("Pub-Sub publish/input file '" + $PUB_FILE + "' already exists.")
                }
                else {
                    Write-Text ("Creating '" + $PUB_FILE + "' file")
                    New-Item -Path $PUB_FILE -ItemType File -Force | Out-Null
                }                
                Set-LinuxStylePermissions -permissions 0x600 -filename $PUB_FILE
            }
        }
    }

    Write-Text ("Configuration file generation is complete! Configuration file is located at " + $CONF_OUTPUT_PATH)
}
