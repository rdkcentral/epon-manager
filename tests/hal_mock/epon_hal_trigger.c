/**
 * @file epon_hal_trigger.c
 * @brief Interactive HAL mock event trigger utility
 * 
 * Command-line tool to trigger HAL callbacks for testing EPON Manager integration.
 * Connects to a running EPON Manager instance via shared memory or socket and
 * sends simulated HAL events.
 */

#include "../../include/epon_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define EPON_HAL_MOCK_SOCKET_PATH "/tmp/epon_hal_mock.sock"
#define MAX_RESPONSE_LEN 256

static void print_usage(const char *prog_name) {
    printf("EPON HAL Mock Event Trigger Utility\n");
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("Options:\n");
    printf("  -s, --status <value>      Trigger ONU status callback\n");
    printf("                            Values: los, signal, register, deregister\n");
    printf("  -a, --alarm <type>        Trigger alarm callback\n");
    printf("                            Types: los, dying_gasp, oam_lost, power_low,\n");
    printf("                                   power_high, temperature, fec\n");
    printf("  -c, --clear <type>        Clear alarm (use with -a)\n");
    printf("  -i, --interface <name>    Trigger interface status callback\n");
    printf("                            Format: <name>:<up|down> (e.g., veip0:up)\n");
    printf("  -l, --list                List all available events\n");
    printf("  -r, --repeat <count>      Repeat event N times (default: 1)\n");
    printf("  -d, --delay <ms>          Delay between repeats in milliseconds (default: 1000)\n");
    printf("  -h, --help                Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s -s register              # Trigger ONU registration\n", prog_name);
    printf("  %s -a los                   # Trigger LOS alarm (raised)\n", prog_name);
    printf("  %s -a los -c                # Clear LOS alarm\n", prog_name);
    printf("  %s -i veip0:up              # Trigger veip0 link up\n", prog_name);
    printf("  %s -s los -r 5 -d 500       # Trigger LOS status 5 times, 500ms apart\n", prog_name);
    printf("\n");
}

static void list_events(void) {
    printf("=== Available ONU Status Events ===\n");
    printf("  los          - Loss of signal (PHY down)\n");
    printf("  signal       - Downstream signal detected\n");
    printf("  register     - LLID-0 online (registered)\n");
    printf("  deregister   - LLID-0 offline (deregistered)\n\n");
    
    printf("=== Available Alarm Types ===\n");
    printf("  los              - Loss of signal\n");
    printf("  lofi             - Loss of frame/lock\n");
    printf("  dying_gasp       - Imminent power loss\n");
    printf("  error_symbol     - Errored symbol period\n");
    printf("  error_frame      - Errored frame\n");
    printf("  oam_lost         - OAM session lost\n");
    printf("  power_low        - Optical power below threshold\n");
    printf("  power_high       - Optical power above threshold\n");
    printf("  equipment        - Equipment failure\n");
    printf("  temperature      - Temperature threshold exceeded\n");
    printf("  fec              - FEC uncorrectable errors\n");
    printf("  laser_bias       - Laser bias current out of range\n");
    printf("  voltage          - Supply voltage out of range\n\n");
    
    printf("=== Interface Status Events ===\n");
    printf("  Format: <interface_name>:<up|down>\n");
    printf("  Examples: veip0:up, veip0:down, veip1:up\n\n");
}

static epon_onu_status_t parse_status(const char *status_str) {
    if (strcasecmp(status_str, "los") == 0) {
        return EPON_ONU_STATUS_LOS;
    } else if (strcasecmp(status_str, "signal") == 0) {
        return EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED;
    } else if (strcasecmp(status_str, "register") == 0) {
        return EPON_ONU_STATUS_REGISTRATION;
    } else if (strcasecmp(status_str, "deregister") == 0) {
        return EPON_ONU_STATUS_DEREGISTRATION;
    }
    return -1;
}

static epon_hal_alarm_t parse_alarm(const char *alarm_str) {
    if (strcasecmp(alarm_str, "los") == 0) return EPON_HAL_ALARM_LOS;
    if (strcasecmp(alarm_str, "lofi") == 0) return EPON_HAL_ALARM_LOFI;
    if (strcasecmp(alarm_str, "dying_gasp") == 0) return EPON_HAL_ALARM_DYING_GASP;
    if (strcasecmp(alarm_str, "error_symbol") == 0) return EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD;
    if (strcasecmp(alarm_str, "error_frame") == 0) return EPON_HAL_ALARM_ERROR_FRAME;
    if (strcasecmp(alarm_str, "oam_lost") == 0) return EPON_HAL_ALARM_OAM_SESSION_LOST;
    if (strcasecmp(alarm_str, "power_low") == 0) return EPON_HAL_ALARM_POWER_LOW;
    if (strcasecmp(alarm_str, "power_high") == 0) return EPON_HAL_ALARM_POWER_HIGH;
    if (strcasecmp(alarm_str, "equipment") == 0) return EPON_HAL_ALARM_EQUIPMENT_FAILURE;
    if (strcasecmp(alarm_str, "temperature") == 0) return EPON_HAL_ALARM_TEMPERATURE;
    if (strcasecmp(alarm_str, "fec") == 0) return EPON_HAL_ALARM_FEC_THRESHOLD;
    if (strcasecmp(alarm_str, "laser_bias") == 0) return EPON_HAL_ALARM_LASER_BIAS_CURRENT;
    if (strcasecmp(alarm_str, "voltage") == 0) return EPON_HAL_ALARM_SUPPLY_VOLTAGE;
    return -1;
}

static const char* status_to_string(epon_onu_status_t status) {
    switch (status) {
        case EPON_ONU_STATUS_LOS: return "LOS";
        case EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED: return "SIGNAL_DETECTED";
        case EPON_ONU_STATUS_REGISTRATION: return "REGISTERED";
        case EPON_ONU_STATUS_DEREGISTRATION: return "DEREGISTERED";
        default: return "UNKNOWN";
    }
}

static const char* alarm_to_string(epon_hal_alarm_t alarm) {
    switch (alarm) {
        case EPON_HAL_ALARM_LOS: return "LOS";
        case EPON_HAL_ALARM_LOFI: return "LOFI";
        case EPON_HAL_ALARM_DYING_GASP: return "DYING_GASP";
        case EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD: return "ERROR_SYMBOL_PERIOD";
        case EPON_HAL_ALARM_ERROR_FRAME: return "ERROR_FRAME";
        case EPON_HAL_ALARM_OAM_SESSION_LOST: return "OAM_SESSION_LOST";
        case EPON_HAL_ALARM_POWER_LOW: return "POWER_LOW";
        case EPON_HAL_ALARM_POWER_HIGH: return "POWER_HIGH";
        case EPON_HAL_ALARM_EQUIPMENT_FAILURE: return "EQUIPMENT_FAILURE";
        case EPON_HAL_ALARM_TEMPERATURE: return "TEMPERATURE";
        case EPON_HAL_ALARM_FEC_THRESHOLD: return "FEC_THRESHOLD";
        case EPON_HAL_ALARM_LASER_BIAS_CURRENT: return "LASER_BIAS_CURRENT";
        case EPON_HAL_ALARM_SUPPLY_VOLTAGE: return "SUPPLY_VOLTAGE";
        default: return "UNKNOWN";
    }
}

static int send_command(const char *command) {
    int sockfd;
    struct sockaddr_un addr;
    char response[MAX_RESPONSE_LEN];
    
    /* Create socket */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    /* Connect to HAL mock control socket */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, EPON_HAL_MOCK_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: Failed to connect to EPON Manager (socket: %s)\n", EPON_HAL_MOCK_SOCKET_PATH);
        fprintf(stderr, "       Make sure EPON Manager is running with HAL mock library.\n");
        fprintf(stderr, "       Error: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    /* Send command */
    if (write(sockfd, command, strlen(command)) < 0) {
        fprintf(stderr, "Error: Failed to send command: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    /* Read response */
    ssize_t n = read(sockfd, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        /* Response is just "OK\n" for acknowledgment */
    }
    
    close(sockfd);
    return 0;
}

int main(int argc, char *argv[]) {
    int opt;
    char *status_str = NULL;
    char *alarm_str = NULL;
    char *interface_str = NULL;
    bool alarm_clear = false;
    int repeat_count = 1;
    int delay_ms = 1000;
    
    static struct option long_options[] = {
        {"status",    required_argument, 0, 's'},
        {"alarm",     required_argument, 0, 'a'},
        {"clear",     no_argument,       0, 'c'},
        {"interface", required_argument, 0, 'i'},
        {"list",      no_argument,       0, 'l'},
        {"repeat",    required_argument, 0, 'r'},
        {"delay",     required_argument, 0, 'd'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    
    while ((opt = getopt_long(argc, argv, "s:a:ci:lr:d:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                status_str = optarg;
                break;
            case 'a':
                alarm_str = optarg;
                break;
            case 'c':
                alarm_clear = true;
                break;
            case 'i':
                interface_str = optarg;
                break;
            case 'l':
                list_events();
                return 0;
            case 'r':
                repeat_count = atoi(optarg);
                if (repeat_count < 1) repeat_count = 1;
                break;
            case 'd':
                delay_ms = atoi(optarg);
                if (delay_ms < 0) delay_ms = 0;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    printf("=== EPON HAL Mock Event Trigger ===\n\n");
    
    /* Trigger ONU status callback */
    if (status_str) {
        epon_onu_status_t status = parse_status(status_str);
        if (status == (epon_onu_status_t)-1) {
            fprintf(stderr, "Error: Invalid status '%s'\n", status_str);
            fprintf(stderr, "Valid values: los, signal, register, deregister\n");
            return 1;
        }
        
        for (int i = 0; i < repeat_count; i++) {
            printf("[%d/%d] Triggering ONU status: %s\n", i+1, repeat_count, status_to_string(status));
            
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "STATUS:%d", status);
            if (send_command(cmd) < 0) {
                return 1;
            }
            
            if (i < repeat_count - 1 && delay_ms > 0) {
                usleep(delay_ms * 1000);
            }
        }
        printf("✓ Status callback triggered successfully\n\n");
    }
    
    /* Trigger alarm callback */
    if (alarm_str) {
        epon_hal_alarm_t alarm = parse_alarm(alarm_str);
        if (alarm == (epon_hal_alarm_t)-1) {
            fprintf(stderr, "Error: Invalid alarm type '%s'\n", alarm_str);
            fprintf(stderr, "Use -l to list available alarm types\n");
            return 1;
        }
        
        for (int i = 0; i < repeat_count; i++) {
            printf("[%d/%d] Triggering alarm: %s (%s)\n", 
                   i+1, repeat_count, alarm_to_string(alarm), alarm_clear ? "CLEAR" : "RAISED");
            
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "ALARM:%d:%d", alarm, alarm_clear ? 0 : 1);
            if (send_command(cmd) < 0) {
                return 1;
            }
            
            if (i < repeat_count - 1 && delay_ms > 0) {
                usleep(delay_ms * 1000);
            }
        }
        printf("✓ Alarm callback triggered successfully\n\n");
    }
    
    /* Trigger interface status callback */
    if (interface_str) {
        char *colon = strchr(interface_str, ':');
        if (!colon) {
            fprintf(stderr, "Error: Invalid interface format '%s'\n", interface_str);
            fprintf(stderr, "Format: <name>:<up|down> (e.g., veip0:up)\n");
            return 1;
        }
        
        *colon = '\0';
        char *if_name = interface_str;
        char *if_status = colon + 1;
        
        epon_interface_link_status_t link_status;
        if (strcasecmp(if_status, "up") == 0) {
            link_status = EPON_ONU_INTF_STATUS_LINK_UP;
        } else if (strcasecmp(if_status, "down") == 0) {
            link_status = EPON_ONU_INTF_STATUS_LINK_DOWN;
        } else {
            fprintf(stderr, "Error: Invalid interface status '%s'\n", if_status);
            fprintf(stderr, "Valid values: up, down\n");
            return 1;
        }
        
        for (int i = 0; i < repeat_count; i++) {
            printf("[%d/%d] Triggering interface: %s -> %s\n", 
                   i+1, repeat_count, if_name, link_status == EPON_ONU_INTF_STATUS_LINK_UP ? "UP" : "DOWN");
            
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "INTERFACE:%s:%d", if_name, link_status);
            if (send_command(cmd) < 0) {
                return 1;
            }
            
            if (i < repeat_count - 1 && delay_ms > 0) {
                usleep(delay_ms * 1000);
            }
        }
        printf("✓ Interface callback triggered successfully\n\n");
    }
    
    if (!status_str && !alarm_str && !interface_str) {
        fprintf(stderr, "Error: No event specified\n");
        fprintf(stderr, "Use -s, -a, or -i to trigger events\n");
        fprintf(stderr, "Use -h for help or -l to list available events\n");
        return 1;
    }
    
    return 0;
}
