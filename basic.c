#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SLOTS 50
#define MAX_USERS 100
#define SLOTS_FILE "parking_data.txt"
#define USERS_FILE "users.txt"

// Rates & Fine Configurations
#define BASE_HOURLY_RATE 5.0f
#define FINE_HOURLY_RATE 6.0f

// ANSI Color Codes for UI styling
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"

typedef struct {
    int id;
    int is_occupied; // 0 = Free, 1 = Occupied
    char vehicle_no[20];
    int expected_hours;     // Booked duration
    time_t entry_timestamp; // Epoch timestamp when parked
    char parked_by_user[30];
} ParkingSlot;

typedef struct {
    char username[30];
    char password[30];
} UserAccount;

// Global Data
ParkingSlot slots[MAX_SLOTS];
int slot_count = 0;
float hourly_rate = BASE_HOURLY_RATE;

UserAccount users[MAX_USERS];
int user_count = 0;
char current_logged_user[30] = "";

// Helper Declarations
void clear_screen(void);
void print_header(const char *title);
void pause_terminal(void);
float calculate_billing(ParkingSlot slot, int *elapsed_hrs, int *overtime_hrs);

// File I/O
void load_data(void);
void save_parking_data(void);
void load_users(void);
void save_users(void);

// Auth & Menus
int admin_login(void);
int client_login(void);
void client_register(void);
void admin_menu(void);
void client_menu(void);

// Admin Functions
void add_parking_slot(void);
void view_parking_slots(void);
void update_parking_slot(void);
void delete_parking_slot(void);
void generate_report(void);

// Client Functions
void view_available_slots(void);
void park_vehicle(void);
void exit_parking(void);
void view_parking_fee(void);

int main(void) {
    int choice;
    load_data();
    load_users();

    while (1) {
        clear_screen();
        print_header("PARKING MANAGEMENT SYSTEM");
        printf(" Select Option:\n\n");
        printf("  [" COLOR_CYAN "1" COLOR_RESET "] Admin Login\n");
        printf("  [" COLOR_CYAN "2" COLOR_RESET "] Client Login\n");
        printf("  [" COLOR_CYAN "3" COLOR_RESET "] Register Client Account\n");
        printf("  [" COLOR_CYAN "4" COLOR_RESET "] Exit System\n\n");
        printf(" Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }

        switch (choice) {
            case 1:
                if (admin_login()) {
                    strcpy(current_logged_user, "admin");
                    admin_menu();
                }
                break;

            case 2:
                if (client_login()) {
                    client_menu();
                }
                break;

            case 3:
                client_register();
                break;

            case 4:
                save_parking_data();
                save_users();
                printf("\n" COLOR_GREEN "All data saved successfully. Exiting... Goodbye!" COLOR_RESET "\n");
                return 0;

            default:
                printf("\n" COLOR_RED "Invalid selection! Please try again." COLOR_RESET "\n");
                pause_terminal();
                break;
        }
    }

    return 0;
}

// Utility Functions
void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void print_header(const char *title) {
    printf(COLOR_CYAN "======================================================" COLOR_RESET "\n");
    printf(COLOR_BOLD "              %s" COLOR_RESET "\n", title);
    printf(COLOR_CYAN "======================================================" COLOR_RESET "\n\n");
}

void pause_terminal(void) {
    printf("\nPress [ENTER] to continue...");
    getchar(); // Clear leftover newline
    getchar();
}

// Billing Calculation Logic with Fine
float calculate_billing(ParkingSlot slot, int *elapsed_hrs, int *overtime_hrs) {
    if (!slot.is_occupied) {
        *elapsed_hrs = 0;
        *overtime_hrs = 0;
        return 0.0f;
    }

    time_t now = time(NULL);
    double seconds_parked = difftime(now, slot.entry_timestamp);
    
    // Calculate total hours spent (rounded up to nearest whole hour)
    int actual_hrs = (int)(seconds_parked / 3600);
    if ((int)seconds_parked % 3600 > 0 || actual_hrs == 0) {
        actual_hrs++; 
    }

    *elapsed_hrs = actual_hrs;

    if (actual_hrs <= slot.expected_hours) {
        *overtime_hrs = 0;
        return actual_hrs * BASE_HOURLY_RATE;
    } else {
        int extra = actual_hrs - slot.expected_hours;
        *overtime_hrs = extra;
        float base_cost = slot.expected_hours * BASE_HOURLY_RATE;
        float fine_cost = extra * FINE_HOURLY_RATE;
        return base_cost + fine_cost;
    }
}

// Data Persistence Functions (Includes timestamp loading/saving)
void load_data(void) {
    FILE *file = fopen(SLOTS_FILE, "r");
    if (!file) {
        slot_count = 10;
        for (int i = 0; i < slot_count; i++) {
            slots[i].id = i + 1;
            slots[i].is_occupied = 0;
            strcpy(slots[i].vehicle_no, "N/A");
            slots[i].expected_hours = 0;
            slots[i].entry_timestamp = 0;
            strcpy(slots[i].parked_by_user, "N/A");
        }
        save_parking_data();
        return;
    }

    if (fscanf(file, "%f\n", &hourly_rate) != 1) hourly_rate = BASE_HOURLY_RATE;
    if (fscanf(file, "%d\n", &slot_count) != 1) slot_count = 10;

    for (int i = 0; i < slot_count; i++) {
        long long ts = 0;
        fscanf(file, "%d %d %s %d %lld %s\n", 
               &slots[i].id, 
               &slots[i].is_occupied, 
               slots[i].vehicle_no, 
               &slots[i].expected_hours,
               &ts, 
               slots[i].parked_by_user);
        
        slots[i].id = i + 1;
        slots[i].entry_timestamp = (time_t)ts;
    }

    fclose(file);
}

void save_parking_data(void) {
    FILE *file = fopen(SLOTS_FILE, "w");
    if (!file) return;

    fprintf(file, "%.2f\n", hourly_rate);
    fprintf(file, "%d\n", slot_count);

    for (int i = 0; i < slot_count; i++) {
        fprintf(file, "%d %d %s %d %lld %s\n", 
                i + 1, 
                slots[i].is_occupied, 
                slots[i].vehicle_no, 
                slots[i].expected_hours,
                (long long)slots[i].entry_timestamp, 
                slots[i].parked_by_user);
    }

    fclose(file);
}

void load_users(void) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) return;

    if (fscanf(file, "%d\n", &user_count) != 1) user_count = 0;

    for (int i = 0; i < user_count; i++) {
        fscanf(file, "%s %s\n", users[i].username, users[i].password);
    }

    fclose(file);
}

void save_users(void) {
    FILE *file = fopen(USERS_FILE, "w");
    if (!file) return;

    fprintf(file, "%d\n", user_count);
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s %s\n", users[i].username, users[i].password);
    }

    fclose(file);
}

// Authentication Logic
int admin_login(void) {
    char username[30], password[30];

    while (1) {
        clear_screen();
        print_header("ADMIN LOGIN");

        printf(" Username: ");
        scanf("%s", username);
        printf(" Password: ");
        scanf("%s", password);

        if (strcmp(username, "parkAdmin") == 0 && strcmp(password, "park123") == 0) {
            printf("\n" COLOR_GREEN "Login Successful!" COLOR_RESET "\n");
            pause_terminal();
            return 1;
        } else {
            printf("\n" COLOR_RED "Error: Invalid admin credentials!" COLOR_RESET "\n");
            printf(" Try again? [1 = Yes, 0 = Return to Main]: ");
            int retry;
            scanf("%d", &retry);
            if (retry != 1) return 0;
        }
    }
}

void client_register(void) {
    clear_screen();
    print_header("REGISTER NEW CLIENT ACCOUNT");

    if (user_count >= MAX_USERS) {
        printf(COLOR_RED "User registration limit reached!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    char new_username[30], new_password[30];

    printf(" Create Username: ");
    scanf("%s", new_username);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, new_username) == 0) {
            printf("\n" COLOR_RED "Username already exists! Please pick a different one." COLOR_RESET "\n");
            pause_terminal();
            return;
        }
    }

    printf(" Create Password: ");
    scanf("%s", new_password);

    strcpy(users[user_count].username, new_username);
    strcpy(users[user_count].password, new_password);
    user_count++;

    save_users();

    printf("\n" COLOR_GREEN "Account registered successfully! You can now log in." COLOR_RESET "\n");
    pause_terminal();
}

int client_login(void) {
    if (user_count == 0) {
        clear_screen();
        print_header("CLIENT LOGIN");
        printf(COLOR_YELLOW "No registered clients found. Please register an account first." COLOR_RESET "\n");
        pause_terminal();
        return 0;
    }

    char username[30], password[30];

    while (1) {
        clear_screen();
        print_header("CLIENT LOGIN");

        printf(" Username: ");
        scanf("%s", username);
        printf(" Password: ");
        scanf("%s", password);

        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
                strcpy(current_logged_user, username);
                printf("\n" COLOR_GREEN "Welcome back, %s! Login successful." COLOR_RESET "\n", username);
                pause_terminal();
                return 1;
            }
        }

        printf("\n" COLOR_RED "Error: Invalid username or password!" COLOR_RESET "\n");
        printf(" Try again? [1 = Yes, 0 = Return to Main]: ");
        int retry;
        scanf("%d", &retry);
        if (retry != 1) return 0;
    }
}

// Menus
void admin_menu(void) {
    int choice;
    int run = 1;

    while (run) {
        clear_screen();
        print_header("ADMIN MENU");
        printf(" Rates: Base " COLOR_YELLOW "$%.2f/hr" COLOR_RESET " | Overtime Fine: " COLOR_RED "$%.2f/hr" COLOR_RESET "\n\n", BASE_HOURLY_RATE, FINE_HOURLY_RATE);
        printf("  [" COLOR_CYAN "1" COLOR_RESET "] Add Parking Slot\n");
        printf("  [" COLOR_CYAN "2" COLOR_RESET "] View Parking Slots\n");
        printf("  [" COLOR_CYAN "3" COLOR_RESET "] Update Parking Slot\n");
        printf("  [" COLOR_CYAN "4" COLOR_RESET "] Delete Parking Slot\n");
        printf("  [" COLOR_CYAN "5" COLOR_RESET "] Generate Report\n");
        printf("  [" COLOR_CYAN "6" COLOR_RESET "] Log Out\n\n");
        printf(" Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: add_parking_slot(); break;
            case 2: view_parking_slots(); pause_terminal(); break;
            case 3: update_parking_slot(); break;
            case 4: delete_parking_slot(); break;
            case 5: generate_report(); break;
            case 6: run = 0; continue;
            default:
                printf("\n" COLOR_RED "Invalid choice!" COLOR_RESET "\n");
                pause_terminal();
                continue;
        }

        if (run) {
            printf("\n" COLOR_MAGENTA "------------------------------------------------------" COLOR_RESET "\n");
            printf(" Do you want to continue in Admin Menu? [1 = Yes, 0 = Log Out]: ");
            int cont;
            scanf("%d", &cont);
            if (cont != 1) run = 0;
        }
    }
}

void client_menu(void) {
    int choice;
    int run = 1;

    while (run) {
        clear_screen();
        char title_buffer[60];
        snprintf(title_buffer, sizeof(title_buffer), "CLIENT MENU (%s)", current_logged_user);
        print_header(title_buffer);

        printf("  [" COLOR_CYAN "1" COLOR_RESET "] View Available Parking Slots\n");
        printf("  [" COLOR_CYAN "2" COLOR_RESET "] Park Vehicle\n");
        printf("  [" COLOR_CYAN "3" COLOR_RESET "] Exit Parking\n");
        printf("  [" COLOR_CYAN "4" COLOR_RESET "] View Parking Fee / Billing\n");
        printf("  [" COLOR_CYAN "5" COLOR_RESET "] Log Out\n\n");
        printf(" Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: view_available_slots(); pause_terminal(); break;
            case 2: park_vehicle(); break;
            case 3: exit_parking(); break;
            case 4: view_parking_fee(); pause_terminal(); break;
            case 5: run = 0; continue;
            default:
                printf("\n" COLOR_RED "Invalid choice!" COLOR_RESET "\n");
                pause_terminal();
                continue;
        }

        if (run) {
            printf("\n" COLOR_MAGENTA "------------------------------------------------------" COLOR_RESET "\n");
            printf(" Do you want to continue in Client Menu? [1 = Yes, 0 = Log Out]: ");
            int cont;
            scanf("%d", &cont);
            if (cont != 1) run = 0;
        }
    }
}

// Operational Functions
void add_parking_slot(void) {
    clear_screen();
    print_header("ADD PARKING SLOT");
    if (slot_count >= MAX_SLOTS) {
        printf(COLOR_RED "Maximum slot limit reached!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    slots[slot_count].id = slot_count + 1;
    slots[slot_count].is_occupied = 0;
    strcpy(slots[slot_count].vehicle_no, "N/A");
    slots[slot_count].expected_hours = 0;
    slots[slot_count].entry_timestamp = 0;
    strcpy(slots[slot_count].parked_by_user, "N/A");

    slot_count++;
    save_parking_data();
    printf(COLOR_GREEN "Parking Slot #%d added successfully!" COLOR_RESET "\n", slot_count);
}

void view_parking_slots(void) {
    clear_screen();
    print_header("ALL PARKING SLOTS");
    printf(" %-8s | %-14s | %-14s | %-12s | %-12s\n", "Slot ID", "Status", "Vehicle No.", "Elapsed/Booked", "Owner");
    printf(" --------------------------------------------------------------------------------\n");

    for (int i = 0; i < slot_count; i++) {
        const char *status = slots[i].is_occupied ? COLOR_RED "Occupied" COLOR_RESET : COLOR_GREEN "Available" COLOR_RESET;
        if (slots[i].is_occupied) {
            int elapsed, overtime;
            calculate_billing(slots[i], &elapsed, &overtime);
            char duration_str[30];
            snprintf(duration_str, sizeof(duration_str), "%d hrs / %d hrs", elapsed, slots[i].expected_hours);
            printf(" Slot %-3d  | %-23s | %-14s | %-14s | %-12s\n", 
                   slots[i].id, status, slots[i].vehicle_no, duration_str, slots[i].parked_by_user);
        } else {
            printf(" Slot %-3d  | %-23s | %-14s | %-14s | %-12s\n", 
                   slots[i].id, status, "N/A", "N/A", "N/A");
        }
    }
}

void update_parking_slot(void) {
    view_parking_slots();
    int id;
    printf("\n Enter Slot ID to Update: ");
    scanf("%d", &id);

    if (id < 1 || id > slot_count) {
        printf(COLOR_RED "Invalid Slot ID!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    int idx = id - 1;
    printf(" Enter new Status [0 = Available, 1 = Occupied]: ");
    scanf("%d", &slots[idx].is_occupied);

    if (slots[idx].is_occupied) {
        printf(" Enter Vehicle Number: ");
        scanf("%s", slots[idx].vehicle_no);
        printf(" Enter Expected Duration (Hours): ");
        scanf("%d", &slots[idx].expected_hours);
        printf(" Enter Parked By Username: ");
        scanf("%s", slots[idx].parked_by_user);
        slots[idx].entry_timestamp = time(NULL);
    } else {
        strcpy(slots[idx].vehicle_no, "N/A");
        slots[idx].expected_hours = 0;
        slots[idx].entry_timestamp = 0;
        strcpy(slots[idx].parked_by_user, "N/A");
    }

    save_parking_data();
    printf(COLOR_GREEN "Slot #%d updated successfully!" COLOR_RESET "\n", id);
}

void delete_parking_slot(void) {
    view_parking_slots();
    int id;
    printf("\n Enter Slot ID to Delete: ");
    scanf("%d", &id);

    if (id < 1 || id > slot_count) {
        printf(COLOR_RED "Invalid Slot ID!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    for (int i = id - 1; i < slot_count - 1; i++) {
        slots[i] = slots[i + 1];
        slots[i].id = i + 1;
    }
    slot_count--;

    save_parking_data();
    printf(COLOR_GREEN "Slot deleted successfully!" COLOR_RESET "\n");
}

void generate_report(void) {
    clear_screen();
    print_header("PARKING REPORT");

    int occupied = 0;
    float total_revenue = 0.0f;

    for (int i = 0; i < slot_count; i++) {
        if (slots[i].is_occupied) {
            occupied++;
            int elapsed, overtime;
            total_revenue += calculate_billing(slots[i], &elapsed, &overtime);
        }
    }

    printf(" Total Slots      : %d\n", slot_count);
    printf(" Occupied Slots   : " COLOR_RED "%d" COLOR_RESET "\n", occupied);
    printf(" Available Slots  : " COLOR_GREEN "%d" COLOR_RESET "\n", slot_count - occupied);
    printf(" Real-time Revenue: " COLOR_YELLOW "$%.2f" COLOR_RESET "\n", total_revenue);

    pause_terminal();
}

void view_available_slots(void) {
    clear_screen();
    print_header("PARKING SLOTS STATUS");
    printf(" %-10s | %-24s\n", "Slot ID", "Status");
    printf(" ----------------------------------------\n");

    for (int i = 0; i < slot_count; i++) {
        if (slots[i].is_occupied) {
            printf(" Slot %-4d   | " COLOR_RED "[ Unavailable / Occupied ]" COLOR_RESET "\n", slots[i].id);
        } else {
            printf(" Slot %-4d   | " COLOR_GREEN "[ Available ]" COLOR_RESET "\n", slots[i].id);
        }
    }
}

void park_vehicle(void) {
    view_available_slots();

    int id;
    printf("\n Enter Slot ID to Park: ");
    scanf("%d", &id);

    if (id < 1 || id > slot_count) {
        printf(COLOR_RED "Invalid slot selection!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    if (slots[id - 1].is_occupied) {
        printf(COLOR_RED "Slot #%d is already occupied!" COLOR_RESET "\n", id);
        pause_terminal();
        return;
    }

    int idx = id - 1;
    printf(" Enter Vehicle Registration Number: ");
    scanf("%s", slots[idx].vehicle_no);
    printf(" Enter Expected Duration (Hours): ");
    scanf("%d", &slots[idx].expected_hours);

    slots[idx].is_occupied = 1;
    slots[idx].entry_timestamp = time(NULL); // Store current system timestamp
    strcpy(slots[idx].parked_by_user, current_logged_user);

    save_parking_data();

    printf("\n" COLOR_GREEN "Vehicle (%s) parked successfully in Slot #%d!" COLOR_RESET "\n", slots[idx].vehicle_no, id);
    printf(" Entry time recorded. Fine rate ($6/hr) applies if exceeded.\n");
}

void view_parking_fee(void) {
    clear_screen();
    print_header("YOUR PARKED VEHICLES & BILLING");

    int user_vehicles_count = 0;
    float grand_total = 0.0f;

    printf(" Base Rate: " COLOR_YELLOW "$%.2f/hr" COLOR_RESET " | Overtime Rate: " COLOR_RED "$%.2f/hr" COLOR_RESET "\n\n", BASE_HOURLY_RATE, FINE_HOURLY_RATE);
    printf(" %-8s | %-16s | %-12s | %-10s | %-12s\n", "Slot ID", "Registration No.", "Elapsed/Booked", "Overtime", "Amount Due");
    printf(" --------------------------------------------------------------------------\n");

    for (int i = 0; i < slot_count; i++) {
        if (slots[i].is_occupied && strcmp(slots[i].parked_by_user, current_logged_user) == 0) {
            int elapsed = 0, overtime = 0;
            float cost = calculate_billing(slots[i], &elapsed, &overtime);
            grand_total += cost;
            user_vehicles_count++;

            char duration_str[30];
            snprintf(duration_str, sizeof(duration_str), "%d/%d hrs", elapsed, slots[i].expected_hours);

            printf(" Slot %-3d  | %-16s | %-14s | " COLOR_RED "%2d hrs extra" COLOR_RESET " | " COLOR_YELLOW "$%-10.2f" COLOR_RESET "\n", 
                   slots[i].id, slots[i].vehicle_no, duration_str, overtime, cost);
        }
    }

    if (user_vehicles_count == 0) {
        printf("\n" COLOR_YELLOW " You have no active parked vehicles linked to account '%s'." COLOR_RESET "\n", current_logged_user);
    } else {
        printf(" --------------------------------------------------------------------------\n");
        printf(" Total Vehicles Parked : " COLOR_BOLD "%d" COLOR_RESET "\n", user_vehicles_count);
        printf(" Grand Total Billed    : " COLOR_GREEN "$%.2f" COLOR_RESET "\n", grand_total);
    }
}

void exit_parking(void) {
    clear_screen();
    print_header("EXIT PARKING & PAYMENT");

    int user_vehicles_count = 0;
    printf(" Vehicles registered under account '" COLOR_CYAN "%s" COLOR_RESET "':\n\n", current_logged_user);
    printf(" %-8s | %-16s | %-14s | %-10s | %-12s\n", "Slot ID", "Registration No.", "Elapsed/Booked", "Overtime", "Amount Due");
    printf(" --------------------------------------------------------------------------\n");

    for (int i = 0; i < slot_count; i++) {
        if (slots[i].is_occupied && strcmp(slots[i].parked_by_user, current_logged_user) == 0) {
            int elapsed = 0, overtime = 0;
            float cost = calculate_billing(slots[i], &elapsed, &overtime);
            user_vehicles_count++;

            char duration_str[30];
            snprintf(duration_str, sizeof(duration_str), "%d/%d hrs", elapsed, slots[i].expected_hours);

            printf(" Slot %-3d  | %-16s | %-14s | " COLOR_RED "%2d hrs extra" COLOR_RESET " | " COLOR_YELLOW "$%-10.2f" COLOR_RESET "\n", 
                   slots[i].id, slots[i].vehicle_no, duration_str, overtime, cost);
        }
    }

    if (user_vehicles_count == 0) {
        printf("\n" COLOR_YELLOW " You have no active vehicles to check out." COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    printf(" --------------------------------------------------------------------------\n");

    int slot_id;
    printf("\n Enter Slot ID of the vehicle you want to exit: ");
    scanf("%d", &slot_id);

    if (slot_id < 1 || slot_id > slot_count) {
        printf(COLOR_RED "Invalid Slot ID!" COLOR_RESET "\n");
        pause_terminal();
        return;
    }

    int idx = slot_id - 1;

    if (!slots[idx].is_occupied || strcmp(slots[idx].parked_by_user, current_logged_user) != 0) {
        printf(COLOR_RED "No vehicle owned by you was found in Slot #%d!" COLOR_RESET "\n", slot_id);
        pause_terminal();
        return;
    }

    int elapsed = 0, overtime = 0;
    float billed_amount = calculate_billing(slots[idx], &elapsed, &overtime);
    float payment = 0.0f;

    printf("\n Registration Number : " COLOR_BOLD "%s" COLOR_RESET "\n", slots[idx].vehicle_no);
    printf(" Time Elapsed        : %d hours (Booked: %d hours)\n", elapsed, slots[idx].expected_hours);
    if (overtime > 0) {
        printf(" Overtime Fine Added : " COLOR_RED "%d hours x $6.00/hr fine" COLOR_RESET "\n", overtime);
    }
    printf(" Total Billed Amount : " COLOR_YELLOW "$%.2f" COLOR_RESET "\n", billed_amount);
    printf(" Enter Payment Amount ($): ");
    scanf("%f", &payment);

    if (payment < billed_amount) {
        printf("\n" COLOR_RED "Insufficient payment! Required amount is $%.2f." COLOR_RESET "\n", billed_amount);
        pause_terminal();
        return;
    }

    float change = payment - billed_amount;

    // Reset slot values
    slots[idx].is_occupied = 0;
    strcpy(slots[idx].vehicle_no, "N/A");
    slots[idx].expected_hours = 0;
    slots[idx].entry_timestamp = 0;
    strcpy(slots[idx].parked_by_user, "N/A");

    save_parking_data();

    printf("\n" COLOR_GREEN "Payment Successful!" COLOR_RESET "\n");
    if (change > 0) {
        printf(" Change Returned : " COLOR_CYAN "$%.2f" COLOR_RESET "\n", change);
    }
    printf(" Vehicle checked out. Slot #%d is now free!\n", slot_id);

    pause_terminal();
}