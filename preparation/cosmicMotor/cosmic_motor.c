#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <math.h>

// #define MOCK_HARDWARE
int fd; // File descriptor for the port

#ifdef MOCK_HARDWARE
// Mock implementations

int open_port(void)
{
    printf("Mock: Open port\n");
    // Implement mock behavior
    return 1; // Mock file descriptor
}

void send_command(int fd, char *command)
{
    printf("Mock: Sending command - %s\n", command);
    // Implement mock behavior
    // Maybe simulate some delay
}

void read_response(int fd)
{
    printf("Mock: Reading response\n");
    // Implement mock response
}

#else
// Actual hardware interaction implementations

int open_port(void)
{
    // fd = open("/dev/tty.usbserial-A601VOHX", O_RDWR | O_NOCTTY | O_NDELAY);
    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    // fd = open("/dev/tty.usbserial-AI054UCW", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("open_port: Unable to open USB port - ");
    }
    else
    {
        fcntl(fd, F_SETFL, 0);
    }
    return (fd);
}

void send_command(int fd, char *command)
{
    char full_command[128];
    snprintf(full_command, sizeof(full_command), "%s\r\n", command); // Append termination characters
    write(fd, full_command, strlen(full_command));
    sleep(1); // Wait for command to execute
}

void read_response(int fd)
{
    char buffer[32];
    int n = read(fd, buffer, sizeof(buffer));
    if (n < 0)
    {
        perror("Read failed");
        return;
    }
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);
}

#endif

gboolean show_confirmation_dialog(GtkWidget *parent)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel(parent);
    if (!GTK_IS_WINDOW(toplevel))
    {
        g_warning("Parent widget is not a window");
        return FALSE;
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "Are you sure?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm Action");

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (response == GTK_RESPONSE_YES);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_window_unfullscreen(GTK_WINDOW(gtk_widget_get_toplevel(widget)));
        return TRUE; // Indicate that the event was handled
    }
    return FALSE; // Propagate the event further
}

static void start_limit_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        send_command(fd, "C, (, I3M0, I1M0,), R");
    }
}

static void end_limit_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        send_command(fd, "C, (, I3M-0, I1M-0,), R");
    }
}
static void online(void)
{
    send_command(fd, "F");
}

GtkWidget *distance_entry;
GtkWidget *position_label;
GtkWidget *status_label;

// Function to handle the "Move" button click
static void move_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        const char *distance_str = gtk_entry_get_text(GTK_ENTRY(distance_entry));
        double distance = strtod(distance_str, NULL); // Convert the string to a double
        // Convert the distance to steps by dividing by 0.0025 and rounding to the nearest integer
        int steps = (int)((distance / 0.0025 + 0.5));
        char command[128];
        snprintf(command, sizeof(command), "C, (,I3M-%d, I1M-%d,), R", steps, steps);
        // print the steps to the terminal
        printf("Steps: %d\n", steps);
        printf("Command: %s\n", command);
        send_command(fd, command);
        sleep(1);
    }
}

// Function to handle the "Move" button click
static void move_closer_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        const char *distance_str = gtk_entry_get_text(GTK_ENTRY(distance_entry));
        double distance = strtod(distance_str, NULL); // Convert the string to a double
        // Convert the distance to steps by dividing by 0.0025 and rounding to the nearest integer
        int steps = (int)((distance / 0.0025));
        char command[128];
        snprintf(command, sizeof(command), "C, (,I3M%d, I1M%d,), R", steps, steps);
        // snprintf(command, sizeof(command), "C, I1M%d, R", steps);
        // print the steps to the terminal
        printf("Steps: %d\n", steps);
        printf("Command: %s\n", command);
        send_command(fd, command);
        sleep(1);
    }
}

static void move_left_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        const char *distance_str = gtk_entry_get_text(GTK_ENTRY(distance_entry));
        double distance = strtod(distance_str, NULL); // Convert the string to a double
        // Convert the distance to steps by dividing by 0.0025 and rounding to the nearest integer
        int steps = (int)((distance / 0.005));
        char command[128];
        snprintf(command, sizeof(command), "C, I2M%d, R", steps);
        // snprintf(command, sizeof(command), "C, I1M%d, R", steps);
        // print the steps to the terminal
        printf("Steps: %d\n", steps);
        printf("Command: %s\n", command);
        send_command(fd, command);
        sleep(1);
    }
}

static void move_right_button_clicked_cb(GtkWidget *button, gpointer data)
{
    if (show_confirmation_dialog(button))
    {
        const char *distance_str = gtk_entry_get_text(GTK_ENTRY(distance_entry));
        double distance = strtod(distance_str, NULL); // Convert the string to a double
        // Convert the distance to steps by dividing by 0.0025 and rounding to the nearest integer
        int steps = (int)((distance / 0.005));
        char command[128];
        snprintf(command, sizeof(command), "C, I2M-%d, R", steps);
        // snprintf(command, sizeof(command), "C, I1M%d, R", steps);
        // print the steps to the terminal
        printf("Steps: %d\n", steps);
        printf("Command: %s\n", command);
        send_command(fd, command);
        sleep(1);
    }
}

static void set_zero_button_clicked_cb(GtkWidget *button, gpointer data)
{
    send_command(fd, "N");
}

static void status_button_clicked_cb(GtkWidget *button, gpointer data)
{
    send_command(fd, "V");
    static char buffer[32];                       // Make buffer static so it remains valid after function returns
    int n = read(fd, buffer, sizeof(buffer) - 1); // Leave space for null terminator
    if (n < 0)
    {
        perror("Read failed");
    }
    else
    {
        buffer[n] = '\0';
        // if the buffer is R, the motor is running if the buffer is B the motor is busy else its in jog mode
        if (strcmp(buffer, "R") == 0)
        {
            strcpy(buffer, "Running");
        }
        else if (strcmp(buffer, "B") == 0)
        {
            strcpy(buffer, "Busy");
        }
        else
        {
            strcpy(buffer, "Jog");
        }

        char label_text[64];                                     // Enough space for prefix and position value
        sprintf(label_text, "Motor Status: %s", buffer);         // Create the new label text with two decimal places
        gtk_label_set_text(GTK_LABEL(status_label), label_text); // Set the new label text
    }
}

static void get_position_button_clicked_cb(GtkWidget *button, gpointer data)
{
    char buffer[32];
    int n;
    int position;
    double distance_mm;
    char position_text[128] = ""; // Buffer to hold the combined position text

    // Array of commands and axis names
    const char *commands[] = {"Y", "Z"};
    const char *axis_names[] = {"X", "Y"};

    for (int i = 0; i < 2; ++i) // Loop over 2 elements
    {
        char commandCopy[10];
        strcpy(commandCopy, commands[i]);
        // Send command
        send_command(fd, commandCopy);

        // Read response
        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            perror("Read failed");
            strcat(position_text, axis_names[i]);
            strcat(position_text, " Position: Error; ");
        }
        else
        {
            buffer[n] = '\0';
            position = atoi(buffer);
            distance_mm = -1.0 * position * 0.0025; // Convert position to mm
            char single_axis_text[64];
            sprintf(single_axis_text, "%s Position: %.2f mm; ", axis_names[i], distance_mm);
            strcat(position_text, single_axis_text);
        }
    }

    // Update the position label with the combined position text
    gtk_label_set_text(GTK_LABEL(position_label), position_text);
}

static void clear_button_clicked_cb(GtkWidget *button, gpointer data)
{
    send_command(fd, "C");
}

static void kill_button_clicked_cb(GtkWidget *button, gpointer data)
{
    send_command(fd, "K");
}

static void close_port_clicked(void)
{
    send_command(fd, "Q");
}

static void set_motors(void)
{
    send_command(fd, "setM1M4, setM2M5, setM3M4");
}

static gboolean update_gui_callback(gpointer data)
{
    // Safely update your GUI here.
    // For example, if you need to update a label, progress bar, etc.
    // This runs in the main GTK thread.
    return G_SOURCE_REMOVE; // Ensures this function is only called once.
}

// Callback function for number pad buttons
void on_number_pad_button_press(GtkWidget *widget, gpointer data)
{
    const char *button_value = gtk_button_get_label(GTK_BUTTON(widget));
    static char input[100] = {0}; // Buffer for the input string

    if (strcmp(button_value, "<-") == 0)
    { // Backspace
        int len = strlen(input);
        if (len > 0)
            input[len - 1] = '\0';
    }
    else if (strlen(input) < sizeof(input) - 1)
    {
        strcat(input, button_value); // Append number
    }

    gtk_entry_set_text(GTK_ENTRY(distance_entry), input); // Update the display
}

// Function to create number pad
void create_number_pad(GtkWidget *parent)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(parent), grid);

    const char *buttons[12] = {"7", "8", "9", "4", "5", "6", "1", "2", "3", "0", ".", "<-"};
    for (int i = 0; i < 12; i++)
    {
        GtkWidget *button = gtk_button_new_with_label(buttons[i]);
        g_signal_connect(button, "clicked", G_CALLBACK(on_number_pad_button_press), NULL);
        gtk_grid_attach(GTK_GRID(grid), button, i % 3, i / 3, 1, 1);
    }
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    open_port();

#ifndef MOCK_HARDWARE
    // configure the port only if MOCK_HARDWARE is not defined
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0)
    {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, B9600); // set baud rate
    cfsetispeed(&tty, B9600);
    tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls, enable reading
    tty.c_cflag &= ~CSIZE;           // Mask the character size bits
    tty.c_cflag |= CS8;              // 8 data bit
    tty.c_cflag &= ~PARENB;          // no parity bit
    tty.c_cflag &= ~CSTOPB;          // only need 1 stop bit
    tty.c_cflag &= ~CRTSCTS;         // no hardware flowcontrol

    // apply the settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
#endif

    online();
    set_motors();

    int width = 300;
    int height = 100;

    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "button { "
        "   padding: 40px; "
        "   font-size: 20px; "
        "   background-color: #DCEEFB; "
        "}"
        "label { "
        "   font-size: 16px; " // Adjust the font size as needed
        "}";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Motor Controller");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 200);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *start_limit_button = gtk_button_new_with_label("Start Limit Switch");
    g_signal_connect(start_limit_button, "clicked", G_CALLBACK(start_limit_button_clicked_cb), NULL);
    gtk_widget_set_size_request(start_limit_button, width, height); // Width = width, Height = height

    GtkWidget *end_limit_button = gtk_button_new_with_label("End Limit Switch");
    g_signal_connect(end_limit_button, "clicked", G_CALLBACK(end_limit_button_clicked_cb), NULL);
    gtk_widget_set_size_request(end_limit_button, width, height); // Width = width, Height = height

    GtkWidget *set_zero_button = gtk_button_new_with_label("Set Position to Zero");
    g_signal_connect(set_zero_button, "clicked", G_CALLBACK(set_zero_button_clicked_cb), NULL);
    gtk_widget_set_size_request(set_zero_button, width, height); // Width = width, Height = height

    GtkWidget *move_button = gtk_button_new_with_label("Move Up (Y)");
    g_signal_connect(move_button, "clicked", G_CALLBACK(move_button_clicked_cb), NULL);
    gtk_widget_set_size_request(move_button, width, height); // Width = width, Height = height

    GtkWidget *move_closer_button = gtk_button_new_with_label("Move Down (Y)");
    g_signal_connect(move_closer_button, "clicked", G_CALLBACK(move_closer_button_clicked_cb), NULL);
    gtk_widget_set_size_request(move_closer_button, width, height); // Width = width, Height = height

    GtkWidget *get_position_button = gtk_button_new_with_label("Get Position");
    g_signal_connect(get_position_button, "clicked", G_CALLBACK(get_position_button_clicked_cb), NULL);
    gtk_widget_set_size_request(get_position_button, width, height); // Width = width, Height = height

    GtkWidget *clear_button = gtk_button_new_with_label("Clear");
    g_signal_connect(clear_button, "clicked", G_CALLBACK(clear_button_clicked_cb), NULL);
    gtk_widget_set_size_request(clear_button, width, height); // Width = width, Height = height

    GtkWidget *kill_button = gtk_button_new_with_label("Kill Op.");
    g_signal_connect(kill_button, "clicked", G_CALLBACK(kill_button_clicked_cb), NULL);
    gtk_widget_set_size_request(kill_button, width, height); // Width = width, Height = height

    GtkWidget *status_button = gtk_button_new_with_label("Status");
    g_signal_connect(status_button, "clicked", G_CALLBACK(status_button_clicked_cb), NULL);
    gtk_widget_set_size_request(status_button, width, height); // Width = width, Height = height

    GtkWidget *close_port_button = gtk_button_new_with_label("Offline");
    g_signal_connect(close_port_button, "clicked", G_CALLBACK(close_port_clicked), NULL);
    gtk_widget_set_size_request(close_port_button, width, height); // Width = width, Height = height

    GtkWidget *move_left_button = gtk_button_new_with_label("Move Left (X)");
    g_signal_connect(move_left_button, "clicked", G_CALLBACK(move_left_button_clicked_cb), NULL);
    gtk_widget_set_size_request(move_left_button, width, height); // Width = width, Height = height

    GtkWidget *move_right_button = gtk_button_new_with_label("Move Right (X)");
    g_signal_connect(move_right_button, "clicked", G_CALLBACK(move_right_button_clicked_cb), NULL);
    gtk_widget_set_size_request(move_right_button, width, height); // Width = width, Height = height

    // Create the number pad
    GtkWidget *number_pad_frame = gtk_frame_new("Number Pad");
    create_number_pad(number_pad_frame);

    distance_entry = gtk_entry_new();

    GtkWidget *operation_commands_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(operation_commands_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(operation_commands_grid), 6);

    gtk_grid_attach(GTK_GRID(operation_commands_grid), clear_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(operation_commands_grid), kill_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(operation_commands_grid), status_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(operation_commands_grid), set_zero_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(operation_commands_grid), get_position_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(operation_commands_grid), close_port_button, 1, 2, 1, 1);

    GtkWidget *operation_commands_frame = gtk_frame_new("Operation Commands");
    gtk_container_add(GTK_CONTAINER(operation_commands_frame), operation_commands_grid);

    /* Create the second group of buttons */
    GtkWidget *movement_commands_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(movement_commands_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(movement_commands_grid), 6);

    gtk_grid_attach(GTK_GRID(movement_commands_grid), start_limit_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(movement_commands_grid), end_limit_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(movement_commands_grid), move_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(movement_commands_grid), move_closer_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(movement_commands_grid), move_left_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(movement_commands_grid), move_right_button, 1, 2, 1, 1);

    GtkWidget *movement_commands_frame = gtk_frame_new("Movement Commands");
    gtk_container_add(GTK_CONTAINER(movement_commands_frame), movement_commands_grid);

    GtkWidget *combined_grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(combined_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(combined_grid), TRUE);
    gtk_grid_attach(GTK_GRID(combined_grid), operation_commands_frame, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(combined_grid), movement_commands_frame, 0, 0, 1, 1);

    /* Create the display */

    GtkWidget *display_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *distance_label = gtk_label_new("Enter the Distance (mm): ");
    gtk_box_pack_start(GTK_BOX(display_box), distance_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(display_box), distance_entry, FALSE, FALSE, 0);

    position_label = gtk_label_new("Position (mm): ");
    gtk_box_pack_start(GTK_BOX(display_box), position_label, FALSE, FALSE, 0);

    status_label = gtk_label_new("Motor Status: ");
    gtk_box_pack_start(GTK_BOX(display_box), status_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(display_box), number_pad_frame, FALSE, FALSE, 0); // Add number pad here

    GtkWidget *display_frame = gtk_frame_new("Display");
    gtk_container_add(GTK_CONTAINER(display_frame), display_box);

    // Make the window fullscreen
    gtk_window_fullscreen(GTK_WINDOW(window));

    /* Put everything together */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), combined_grid, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), display_frame, TRUE, TRUE, 0);

    // Add your main container to the scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), hbox); // Assuming hbox is your main container

    // Add to the window
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    // Show all widgets
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
