# Function to set console colors
function Set-ConsoleColor {
    param(
        [System.ConsoleColor]$foregroundColor,
        [System.ConsoleColor]$backgroundColor
    )
    $Host.UI.RawUI.ForegroundColor = $foregroundColor
    $Host.UI.RawUI.BackgroundColor = $backgroundColor
}

function Read-Text {
    param(
        [System.String]$strPrompt
    )
    Set-ConsoleColor -foregroundColor Magenta -backgroundColor Black
    $tmpEntry =  Read-Host $strPrompt
    Set-ConsoleColor -foregroundColor White -backgroundColor Black
    return $tmpEntry
}

function Write-Text {
    param(
        [System.String]$strPrompt
    )
    Write-Host $strPrompt -ForegroundColor Green
}

function Write-Warning {
    param(
        [System.String]$strPrompt
    )
    Write-Host $strPrompt -ForegroundColor Red
}

function Get-EnvVariable {
    param(
        [System.String]$strName
    )
    return Get-ChildItem Env:$strName | Select-Object -ExpandProperty Value
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
$CONF_OUTPUT_PATH = $OUTPUT_DIR + "config\aws-iot-device-client.conf"
$HANDLER_DIR = $OUTPUT_DIR + "jobs"
$PUBSUB_DIR = $OUTPUT_DIR + "pubsub\"
$PUB_FILE = $PUBSUB_DIR + "publish-file.txt"
$SUB_FILE = $PUBSUB_DIR + "subscribe-file.txt"
$DD_INTERVAL = 300
$LOG_TYPE = "STDOUT"
$LOG_LEVEL = "DEBUG"
$LOGS_DIR = $OUTPUT_DIR + "logs\"
$LOG_LOCATION = $LOGS_DIR + "aws-iot-device-client.log"
$SDK_LOGS_ENABLED = "false"
$SDK_LOG_LEVEL = "TRACE"
$SDK_LOG_LOCATION = $LOGS_DIR + "sdk.log"

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
            $SDK_LOGS_ENABLED = "true"
            $logLevels = @("TRACE", "DEBUG". "INFO", "WARN", "ERROR", "FATAL")
            $SDK_LOG_LEVEL_TMP = Read-Text ("Specify desired SDK log level [TRACE/DEBUG/INFO/WARN/ERROR/FATAL]. Default is " + $SDK_LOG_LEVEL)
            if ($SDK_LOG_LEVEL_TMP -ne "" -and $logTypes -contains $SDK_LOG_LEVEL_TMP) {
                $SDK_LOG_LEVEL = $SDK_LOG_LEVEL_TMP
            }

            $SDK_LOG_LOCATION_TMP = Read-Text ("Specify path to desired SDK log file (if no path is provided, will default to " + $SDK_LOG_LOCATION)
            if ($SDK_LOG_LOCATION_TMP -ine "") {
                $SDK_LOG_LOCATION = $SDK_LOG_LOCATION_TMP
            }
        }
    
        ### Jobs Config ###
        $JOBS_ENABLED = "false"
        $ENABLE_JOBS = Read-Text "Enable Jobs feature? [y/n]"
        if ($ENABLE_JOBS -ieq "y") {
            $JOBS_ENABLED = "true"
            $HANDLER_DIR_TEMP = Read-Text ("Specify absolute path to Job handler directory (if no path is provided, will default to '" + $HANDLER_DIR + "'")
            if ($HANDLER_DIR_TEMP -ne "") {
                $HANDLER_DIR = $HANDLER_DIR_TEMP
            }
        } else {
            $JOBS_ENABLED = "false"
        }
    
        ### ST Config ###
        $ENABLE_ST = Read-Text "Enable Secure Tunneling feature? [y/n]"
        if ($ENABLE_ST -ieq "y") {
            $ST_ENABLED = "true"
        } else {
            $ST_ENABLED = "false"
        }

         ### DD Config ###
         $ENABLE_DD = Read-Text "Enable Device Defender feature? [y/n]"
        if ($ENABLE_DD -ieq "y") {
            $DD_ENABLED = "true"
            $INTERVAL_TEMP = Read-Text "Specify an interval for Device Defender in seconds (default is 300)"
            if ($INTERVAL_TEMP -ne "") {
                $DD_INTERVAL = $INTERVAL_TEMP
            }
        } else {
            $DD_ENABLED = "false"
        }
     
        ### FP Config ###
        $FP_ENABLED = "false"
        $ENABLE_FP = Read-Text "Enable Fleet Provisioning feature? [y/n]"
        if ($ENABLE_FP -ieq "y") {
            $FP_ENABLED = "true"
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
            $FP_ENABLED = "false"
        }
        
        ### PUBSUB Config ###
        $PUBSUB_ENABLED = "false"
        $ENABLE_PUBSUB = Read-Text "Enable Pub Sub sample feature? [y/n]"
        if ($ENABLE_PUBSUB -ieq "y") {
            $PUBSUB_ENABLED = "true"
            $PUB_TOPIC = Read-Text "Specify a topic for the feature to publish to"
            $PUB_FILE_TMP = Read-Text ("Specify the path of a file for the feature to publish (if no path is provided, will default to '" + $PUB_FILE + "')")
            if ($PUB_FILE_TMP -ne "") {
                $PUB_FILE = $PUB_FILE_TMP
                $ = 1
            }

            $SUB_TOPIC = Read-Text "Specify a topic for the feature to subscribe to"
            $SUB_FILE_TMP = Read-Text ("Specify the path of a file for the feature to write to (if no path is provided, will default to '" + $SUB_FILE + "')")
            if ($SUB_FILE_TMP -ne "") {
                $SUB_FILE = $SUB_FILE_TMP
            }
        } else {
            $PUBSUB_ENABLED = "false"
        }
    
        ### ConfigShadow Config ###
        $CONFIG_SHADOW_ENABLED = "false"
        $ENABLE_CONFIG_SHADOW = Read-Text "Enable Config Shadow feature? [y/n]"
        if ($ENABLE_CONFIG_SHADOW -ieq "y") {
            $CONFIG_SHADOW_ENABLED = "true"
        } else {
            $CONFIG_SHADOW_ENABLED = "false"
        }
        
        ### SampleShadow Config ###
        $SAMPLE_SHADOW_ENABLED = "false"
        $ENABLE_SAMPLE_SHADOW = Read-Text "Enable Sample Shadow feature? [y/n]"
        if ($ENABLE_SAMPLE_SHADOW -ieq "y") {
            $SAMPLE_SHADOW_ENABLED = "true"
            $SAMPLE_SHADOW_NAME = Read-Text "Specify a shadow name for the feature to create or update"
            $SAMPLE_SHADOW_INPUT_FILE = Read-Text "Specify the path of a file for the feature to read from"
            $SAMPLE_SHADOW_OUTPUT_FILE = Read-Text "Specify a the path of a file for the feature to write shadow document to"
        } else {
            $SAMPLE_SHADOW_ENABLED = "false"
        }

        ### Prepare Config File content
        $CONFIG_OUTPUT= [ordered]@{
            "endpoint" = $ENDPOINT
            "cert" = $CERT
            "key" = $PRIVATE_KEY
            "root-ca" = $ROOT_CA
            "thing-name" = $THING_NAME
            "logging" = @{
                "level" =	$LOG_LEVEL
                "type" = $LOG_TYPE
                "file" = $LOG_LOCATION
                "enable-sdk-logging" =	$SDK_LOGS_ENABLED
                "sdk-log-level" =	$SDK_LOG_LEVEL
                "sdk-log-file" = $SDK_LOG_LOCATION
            }
            "jobs" = @{
                "enabled" =	$JOBS_ENABLED
                "handler-directory" = $HANDLER_DIR
            }
            "tunneling" =	@{
                "enabled" = $ST_ENABLED
            }
            "device-defender" =	@{
                "enabled" =	$DD_ENABLED
                "interval" = $DD_INTERVAL
            }
            "fleet-provisioning" = @{
                "enabled" =	$FP_ENABLED
                "template-name" = $FP_TEMPLATE_NAME
                "template-parameters" = $FP_TEMPLATE_PARAMS
                "csr-file" = $FP_CSR_FILE
                "device-key" = $FP_DEVICE_KEY
            }
            "samples" = @{
                "pub-sub" = @{
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
            "sample-shadow" = @{
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
                New-Item -ItemType Directory $CONFIG_LOCATION_PATH
            }

            $CONFIG_OUTPUT | ConvertTo-Json | Out-File -FilePath $CONF_OUTPUT_PATH

            #####
            ## TODO: fix permissions
            # Define the user or group you want to grant permissions to
            #$identity = "DOMAIN\UserOrGroupName"

            # Define the access control list (ACL) rule
            #$permission = New-Object System.Security.AccessControl.FileSystemAccessRule($identity,"FullControl","Allow")

            # Get the current ACL for the file
            #$acl = Get-Acl $file

            # Add the permission to the ACL
            #$acl.SetAccessRule($permission)

            # Set the modified ACL back to the file
            #Set-Acl -Path $file -AclObject $acl

            #chmod 745 "$OUTPUT_DIR"
            #chmod 640 "$CONF_OUTPUT_PATH"
            Write-Text ("Configuration has been successfully written to '" + $CONF_OUTPUT_PATH + "'")

            ### Create a Pub-Sub feature folder
            if ($PUBSUB_ENABLED -ieq "true") {
                Write-Text "Creating default pubsub directory..."
                if (-Not (Test-Path $PUBSUB_DIR)) {
                    New-Item -ItemType Directory $PUBSUB_DIR
                    ###
                    # TODO: change permissions
                    #chmod 745 ${PUBSUB_DIR}
                }
            }
          
            ### Move Sample Jobs if Job feature is enabled
            if ($JOBS_ENABLED -ieq "true") {
                $COPY_HANDLERS = Read-Text ("Do you want to copy the sample job handlers to the specified handler directory ('" + $HANDLER_DIR + "')? [y/n]")      
                if ($COPY_HANDLERS -ieq "y") {
                    if (-Not (Test-Path $HANDLER_DIR)) {
                        New-Item -ItemType Directory $HANDLER_DIR
                        ###
                        # TODO: change permissions
                        #chmod 700 ${HANDLER_DIR}
                    }
        
                    Copy-Item -Path .\sample-job-handlers\* -Destination $HANDLER_DIR -Recurse -Force
                    ###
                    # TODO: change permissions
                    #chmod 700 ${HANDLER_DIR}/*
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
                    New-Item -ItemType Directory $LOG_LOCATION_PATH
                    Write-Text "Created '" + $LOG_LOCATION_PATH "' folder" 
                }
            }

            ### Create SDK Log File Location
            if ($SDK_LOG_LOCATION -ne "") {
                Write-Text "Creating SDK log directory..."
                $SDKLOG_LOCATION_PATH = Split-Path -Path $SDK_LOG_LOCATION -Parent
                if (Test-Path $SDKLOG_LOCATION_PATH) {
                    Write-Text ("Folder '" + $SDKLOG_LOCATION_PATH + "' already exists.")
                }
                else {
                    New-Item -ItemType Directory $SDKLOG_LOCATION_PATH
                    Write-Text "Created '" + $SDKLOG_LOCATION_PATH "' folder" 
                }
            }
        }
    }

    Write-Text "Configuration file generation is complete!"
}
